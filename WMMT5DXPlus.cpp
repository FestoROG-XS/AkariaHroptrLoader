#include <StdInc.h>
#include "Utility/InitFunction.h"
#include "Functions/Global.h"
#include "MinHook.h"
#include <Utility/Hooking.Patterns.h>
#include <thread>
#include <iostream>
#include <Windowsx.h>
#include <Utility/TouchSerial/MT6.h>
#include <cstdarg>

#ifdef _M_AMD64
#pragma optimize("", off)
#pragma comment(lib, "Ws2_32.lib")


extern LPCSTR hookPort;
uintptr_t imageBasedxplus;
static unsigned char hasp_buffer[0xD40];
static bool isFreePlay;
static bool isEventMode2P;
static bool isEventMode4P;
const char* ipaddrdxplus;
bool isUpdate5 = true;

#define HASP_STATUS_OK 0
unsigned int dxpHook_hasp_login(int feature_id, void* vendor_code, int hasp_handle) {
	OutputDebugStringA("[Sentinel HASP] hasp_login\n");
	return HASP_STATUS_OK;
}

unsigned int dxpHook_hasp_logout(int hasp_handle) {
	OutputDebugStringA("[Sentinel HASP] hasp_logout\n");
	return HASP_STATUS_OK;
}

unsigned int dxpHook_hasp_encrypt(int hasp_handle, unsigned char* buffer, unsigned int buffer_size) {
	OutputDebugStringA("[Sentinel HASP] hasp_encrypt\n");
	return HASP_STATUS_OK;
}

unsigned int dxpHook_hasp_decrypt(int hasp_handle, unsigned char* buffer, unsigned int buffer_size) {
	OutputDebugStringA("[Sentinel HASP] hasp_decrypt\n");
	return HASP_STATUS_OK;
}

unsigned int dxpHook_hasp_get_size(int hasp_handle, int hasp_fileid, unsigned int* hasp_size) {
	OutputDebugStringA("[Sentinel HASP] hasp_get_size\n");
	*hasp_size = 0xD40; // Max addressable size by the game... absmax is 4k
	return HASP_STATUS_OK;
}

unsigned int dxpHook_hasp_read(int hasp_handle, int hasp_fileid, unsigned int offset, unsigned int length, unsigned char* buffer) {
	OutputDebugStringA("[Sentinel HASP] hasp_read\n");
	memcpy(buffer, hasp_buffer + offset, length);
	return HASP_STATUS_OK;
}

unsigned int dxpHook_hasp_write(int hasp_handle, int hasp_fileid, unsigned int offset, unsigned int length, unsigned char* buffer) {
	OutputDebugStringA("[Sentinel HASP] hasp_write\n");
	return HASP_STATUS_OK;
}

//set system date patch by pockywitch
typedef bool (WINAPI* SETSYSTEMTIME)(SYSTEMTIME* in);
SETSYSTEMTIME pSetSystemTime = NULL;

bool WINAPI Hook_SetSystemTime(SYSTEMTIME* in) {
	return TRUE;
}

static int ReturnTrue() {
	return 1;
}

void GenerateDongleDataDxp(bool isTerminal) {
	memset(hasp_buffer, 0, 0xD40);
	hasp_buffer[0] = 0x01;
	hasp_buffer[0x13] = 0x01;
	hasp_buffer[0x17] = 0x0A;
	hasp_buffer[0x1B] = 0x04;
	hasp_buffer[0x1C] = 0x3B;
	hasp_buffer[0x1D] = 0x6B;
	hasp_buffer[0x1E] = 0x40;
	hasp_buffer[0x1F] = 0x87;

	hasp_buffer[0x23] = 0x01;
	hasp_buffer[0x27] = 0x0A;
	hasp_buffer[0x2B] = 0x04;
	hasp_buffer[0x2C] = 0x3B;
	hasp_buffer[0x2D] = 0x6B;
	hasp_buffer[0x2E] = 0x40;
	hasp_buffer[0x2F] = 0x87;

	if (isTerminal) {
		memcpy(hasp_buffer + 0xD00, "278311042069", 12); //272211990002
		hasp_buffer[0xD3E] = 0x6B;
		hasp_buffer[0xD3F] = 0x94;
	}
	else {
		memcpy(hasp_buffer + 0xD00, "278313042069", 12); //272213990002
		hasp_buffer[0xD3E] = 0x6D;
		hasp_buffer[0xD3F] = 0x92;
	}
}


static HWND mt6Hwnd;

typedef BOOL(WINAPI* ShowWindow_t)(HWND, int);
static ShowWindow_t pShowWindow;


// Hello Win32 my old friend...
typedef LRESULT(WINAPI* WindowProcedure_t)(HWND, UINT, WPARAM, LPARAM);
static WindowProcedure_t pMaxituneWndProc;

static BOOL gotWindowSize = FALSE;
static unsigned displaySizeX = 0;
static unsigned displaySizeY = 0;
static float scaleFactorX = 0.0f;
static float scaleFactorY = 0.0f;

static LRESULT Hook_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (!gotWindowSize) {
		mt6SetDisplayParams(hwnd);
		gotWindowSize = TRUE;
	}

	if (msg == WM_LBUTTONDOWN ||
		msg == WM_LBUTTONUP) {
		mt6SetTouchData(lParam, msg == WM_LBUTTONDOWN, false);
		return 0;
	}

	if (msg == WM_POINTERDOWN ||
		msg == WM_POINTERUP) {
		mt6SetTouchData(lParam, msg == WM_POINTERDOWN, true);
		return 0;
	}

	return pMaxituneWndProc(hwnd, msg, wParam, lParam);
}

static BOOL Hook_ShowWindow(HWND hwnd, int nCmdShow) {
	SetWindowLongPtrW(hwnd, -4, (LONG_PTR)Hook_WndProc);
	ShowCursor(1);

	mt6Hwnd = hwnd;
	return pShowWindow(hwnd, nCmdShow);
}

typedef void (WINAPI* OutputDebugStringA_t)(LPCSTR);
static void Hook_OutputDebugStringA(LPCSTR str) {
	printf("%s", str);
}

static DWORD WINAPI SpamMulticast(LPVOID) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	int ttl = 255;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));

	int reuse = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&reuse, sizeof(reuse));

	sockaddr_in bindAddr = { 0 };
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_addr.s_addr = inet_addr(ipaddrdxplus);
	bindAddr.sin_port = htons(50765);
	bind(sock, (sockaddr*)&bindAddr, sizeof(bindAddr));


	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr("225.0.0.1");
	mreq.imr_interface.s_addr = inet_addr(ipaddrdxplus);

	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

	sockaddr_in toAddr = { 0 };
	toAddr.sin_family = AF_INET;
	toAddr.sin_addr.s_addr = inet_addr("225.0.0.1");
	toAddr.sin_port = htons(50765);
	return true;
}

typedef int (WINAPI* BINDw5p)(SOCKET, CONST SOCKADDR*, INT);
static BINDw5p pbindw5p = NULL;
unsigned int WINAPI Hook_bind_w5p(SOCKET s, const sockaddr* addr, int namelen) {
	sockaddr_in bindAddr = { 0 };
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_addr.s_addr = inet_addr("192.168.96.20");
	bindAddr.sin_port = htons(50765);
	if (addr == (sockaddr*)&bindAddr) {
		sockaddr_in bindAddr2 = { 0 };
		bindAddr2.sin_family = AF_INET;
		bindAddr2.sin_addr.s_addr = inet_addr(ipaddrdxplus);
		bindAddr2.sin_port = htons(50765);
		return pbindw5p(s, (sockaddr*)&bindAddr2, namelen);
	}
	else {
		return pbindw5p(s, addr, namelen);

	}
}

static void PathFix() {
	auto chars = { 'F', 'G' , 'J' };

	for (auto cha : chars) {
		auto patterns = hook::pattern(va("%02X 3A 2F", cha));

		if (patterns.size() > 0) {
			for (int i = 0; i < patterns.size(); i++) {
				char* text = patterns.get(i).get<char>(0);
				std::string text_str(text);

				std::string to_replace = va("%c:/", cha);
				std::string replace_with = va("./%c", cha);

				std::string replaced = text_str.replace(0, to_replace.length(), replace_with);

				injector::WriteMemoryRaw(text, (char*)replaced.c_str(), replaced.length() + 1, true);
			}
		}
	}
}


// array size is 1107
static const unsigned char terminal_cert_v388[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49,
  0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x44,
  0x42, 0x44, 0x43, 0x43, 0x41, 0x6d, 0x32, 0x67, 0x41, 0x77, 0x49, 0x42, 0x41, 0x67, 0x49, 0x55,
  0x47, 0x73, 0x75, 0x41, 0x30, 0x53, 0x50, 0x37, 0x66, 0x63, 0x45, 0x73, 0x53, 0x31, 0x4e, 0x5a,
  0x63, 0x46, 0x70, 0x4f, 0x6b, 0x30, 0x59, 0x72, 0x50, 0x7a, 0x67, 0x77, 0x44, 0x51, 0x59, 0x4a,
  0x4b, 0x6f, 0x5a, 0x49, 0x68, 0x76, 0x63, 0x4e, 0x41, 0x51, 0x45, 0x46, 0x0a, 0x42, 0x51, 0x41,
  0x77, 0x67, 0x5a, 0x49, 0x78, 0x43, 0x7a, 0x41, 0x4a, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x59,
  0x54, 0x41, 0x6b, 0x70, 0x51, 0x4d, 0x51, 0x34, 0x77, 0x44, 0x41, 0x59, 0x44, 0x56, 0x51, 0x51,
  0x49, 0x44, 0x41, 0x56, 0x55, 0x62, 0x32, 0x74, 0x35, 0x62, 0x7a, 0x45, 0x67, 0x4d, 0x42, 0x34,
  0x47, 0x41, 0x31, 0x55, 0x45, 0x43, 0x67, 0x77, 0x58, 0x54, 0x6b, 0x46, 0x4e, 0x0a, 0x51, 0x30,
  0x38, 0x67, 0x51, 0x6b, 0x46, 0x4f, 0x52, 0x45, 0x46, 0x4a, 0x49, 0x45, 0x64, 0x68, 0x62, 0x57,
  0x56, 0x7a, 0x49, 0x45, 0x6c, 0x75, 0x59, 0x79, 0x34, 0x78, 0x49, 0x6a, 0x41, 0x67, 0x42, 0x67,
  0x4e, 0x56, 0x42, 0x41, 0x73, 0x4d, 0x47, 0x55, 0x35, 0x6c, 0x64, 0x48, 0x64, 0x76, 0x63, 0x6d,
  0x73, 0x67, 0x55, 0x32, 0x56, 0x79, 0x64, 0x6d, 0x56, 0x79, 0x49, 0x45, 0x52, 0x6c, 0x0a, 0x63,
  0x47, 0x46, 0x79, 0x64, 0x47, 0x31, 0x6c, 0x62, 0x6e, 0x51, 0x78, 0x4c, 0x54, 0x41, 0x72, 0x42,
  0x67, 0x4e, 0x56, 0x42, 0x41, 0x4d, 0x4d, 0x4a, 0x45, 0x35, 0x42, 0x54, 0x55, 0x4e, 0x50, 0x49,
  0x45, 0x4a, 0x42, 0x54, 0x6b, 0x52, 0x42, 0x53, 0x53, 0x42, 0x48, 0x59, 0x57, 0x31, 0x6c, 0x63,
  0x79, 0x42, 0x4a, 0x62, 0x6d, 0x4d, 0x67, 0x4c, 0x53, 0x42, 0x4f, 0x55, 0x30, 0x51, 0x67, 0x0a,
  0x51, 0x30, 0x45, 0x67, 0x55, 0x6d, 0x39, 0x76, 0x64, 0x44, 0x41, 0x67, 0x46, 0x77, 0x30, 0x79,
  0x4d, 0x54, 0x41, 0x33, 0x4d, 0x54, 0x67, 0x78, 0x4f, 0x44, 0x55, 0x35, 0x4d, 0x54, 0x42, 0x61,
  0x47, 0x41, 0x38, 0x7a, 0x4d, 0x44, 0x41, 0x78, 0x4d, 0x44, 0x6b, 0x78, 0x4f, 0x54, 0x45, 0x34,
  0x4e, 0x54, 0x6b, 0x78, 0x4d, 0x46, 0x6f, 0x77, 0x67, 0x5a, 0x49, 0x78, 0x43, 0x7a, 0x41, 0x4a,
  0x0a, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x59, 0x54, 0x41, 0x6b, 0x70, 0x51, 0x4d, 0x51, 0x34,
  0x77, 0x44, 0x41, 0x59, 0x44, 0x56, 0x51, 0x51, 0x49, 0x44, 0x41, 0x56, 0x55, 0x62, 0x32, 0x74,
  0x35, 0x62, 0x7a, 0x45, 0x67, 0x4d, 0x42, 0x34, 0x47, 0x41, 0x31, 0x55, 0x45, 0x43, 0x67, 0x77,
  0x58, 0x54, 0x6b, 0x46, 0x4e, 0x51, 0x30, 0x38, 0x67, 0x51, 0x6b, 0x46, 0x4f, 0x52, 0x45, 0x46,
  0x4a, 0x0a, 0x49, 0x45, 0x64, 0x68, 0x62, 0x57, 0x56, 0x7a, 0x49, 0x45, 0x6c, 0x75, 0x59, 0x79,
  0x34, 0x78, 0x49, 0x6a, 0x41, 0x67, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x73, 0x4d, 0x47, 0x55,
  0x35, 0x6c, 0x64, 0x48, 0x64, 0x76, 0x63, 0x6d, 0x73, 0x67, 0x55, 0x32, 0x56, 0x79, 0x64, 0x6d,
  0x56, 0x79, 0x49, 0x45, 0x52, 0x6c, 0x63, 0x47, 0x46, 0x79, 0x64, 0x47, 0x31, 0x6c, 0x62, 0x6e,
  0x51, 0x78, 0x0a, 0x4c, 0x54, 0x41, 0x72, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x4d, 0x4d, 0x4a,
  0x45, 0x35, 0x42, 0x54, 0x55, 0x4e, 0x50, 0x49, 0x45, 0x4a, 0x42, 0x54, 0x6b, 0x52, 0x42, 0x53,
  0x53, 0x42, 0x48, 0x59, 0x57, 0x31, 0x6c, 0x63, 0x79, 0x42, 0x4a, 0x62, 0x6d, 0x4d, 0x67, 0x4c,
  0x53, 0x42, 0x4f, 0x55, 0x30, 0x51, 0x67, 0x51, 0x30, 0x45, 0x67, 0x55, 0x6d, 0x39, 0x76, 0x64,
  0x44, 0x43, 0x42, 0x0a, 0x6e, 0x7a, 0x41, 0x4e, 0x42, 0x67, 0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47,
  0x39, 0x77, 0x30, 0x42, 0x41, 0x51, 0x45, 0x46, 0x41, 0x41, 0x4f, 0x42, 0x6a, 0x51, 0x41, 0x77,
  0x67, 0x59, 0x6b, 0x43, 0x67, 0x59, 0x45, 0x41, 0x33, 0x47, 0x54, 0x4f, 0x41, 0x2f, 0x4a, 0x31,
  0x62, 0x69, 0x41, 0x6e, 0x71, 0x6c, 0x79, 0x70, 0x33, 0x36, 0x43, 0x55, 0x77, 0x55, 0x35, 0x42,
  0x63, 0x64, 0x63, 0x71, 0x0a, 0x39, 0x37, 0x4d, 0x61, 0x54, 0x77, 0x37, 0x68, 0x4d, 0x55, 0x66,
  0x54, 0x71, 0x58, 0x57, 0x33, 0x32, 0x35, 0x33, 0x69, 0x71, 0x48, 0x4b, 0x33, 0x50, 0x72, 0x68,
  0x58, 0x78, 0x45, 0x78, 0x7a, 0x39, 0x4a, 0x37, 0x6e, 0x70, 0x79, 0x2b, 0x31, 0x34, 0x36, 0x76,
  0x31, 0x75, 0x38, 0x4c, 0x33, 0x4e, 0x70, 0x66, 0x41, 0x48, 0x44, 0x58, 0x37, 0x45, 0x57, 0x6c,
  0x4c, 0x48, 0x51, 0x58, 0x57, 0x0a, 0x6b, 0x4e, 0x6f, 0x49, 0x37, 0x4a, 0x6b, 0x56, 0x56, 0x59,
  0x73, 0x32, 0x35, 0x66, 0x71, 0x58, 0x4c, 0x41, 0x4b, 0x79, 0x49, 0x4a, 0x4a, 0x4f, 0x42, 0x48,
  0x52, 0x43, 0x66, 0x46, 0x4f, 0x38, 0x50, 0x46, 0x79, 0x35, 0x32, 0x51, 0x53, 0x4c, 0x5a, 0x53,
  0x70, 0x7a, 0x55, 0x63, 0x68, 0x58, 0x4b, 0x61, 0x75, 0x73, 0x79, 0x79, 0x45, 0x6b, 0x32, 0x44,
  0x78, 0x70, 0x2f, 0x78, 0x61, 0x69, 0x0a, 0x65, 0x49, 0x64, 0x69, 0x68, 0x50, 0x76, 0x37, 0x49,
  0x55, 0x50, 0x6e, 0x70, 0x64, 0x30, 0x43, 0x41, 0x77, 0x45, 0x41, 0x41, 0x61, 0x4e, 0x54, 0x4d,
  0x46, 0x45, 0x77, 0x48, 0x51, 0x59, 0x44, 0x56, 0x52, 0x30, 0x4f, 0x42, 0x42, 0x59, 0x45, 0x46,
  0x45, 0x77, 0x37, 0x67, 0x4b, 0x32, 0x4c, 0x34, 0x48, 0x2f, 0x79, 0x31, 0x52, 0x4a, 0x67, 0x35,
  0x32, 0x64, 0x33, 0x66, 0x64, 0x70, 0x62, 0x0a, 0x78, 0x6c, 0x71, 0x4a, 0x4d, 0x42, 0x38, 0x47,
  0x41, 0x31, 0x55, 0x64, 0x49, 0x77, 0x51, 0x59, 0x4d, 0x42, 0x61, 0x41, 0x46, 0x45, 0x77, 0x37,
  0x67, 0x4b, 0x32, 0x4c, 0x34, 0x48, 0x2f, 0x79, 0x31, 0x52, 0x4a, 0x67, 0x35, 0x32, 0x64, 0x33,
  0x66, 0x64, 0x70, 0x62, 0x78, 0x6c, 0x71, 0x4a, 0x4d, 0x41, 0x38, 0x47, 0x41, 0x31, 0x55, 0x64,
  0x45, 0x77, 0x51, 0x49, 0x4d, 0x41, 0x59, 0x42, 0x0a, 0x41, 0x66, 0x38, 0x43, 0x41, 0x51, 0x41,
  0x77, 0x44, 0x51, 0x59, 0x4a, 0x4b, 0x6f, 0x5a, 0x49, 0x68, 0x76, 0x63, 0x4e, 0x41, 0x51, 0x45,
  0x46, 0x42, 0x51, 0x41, 0x44, 0x67, 0x59, 0x45, 0x41, 0x58, 0x46, 0x4c, 0x73, 0x70, 0x52, 0x47,
  0x67, 0x2b, 0x53, 0x4b, 0x73, 0x6f, 0x44, 0x33, 0x72, 0x48, 0x4c, 0x68, 0x47, 0x32, 0x31, 0x2f,
  0x74, 0x31, 0x64, 0x64, 0x6b, 0x6b, 0x4e, 0x70, 0x54, 0x0a, 0x78, 0x56, 0x4d, 0x31, 0x66, 0x4b,
  0x61, 0x33, 0x6a, 0x45, 0x45, 0x35, 0x7a, 0x62, 0x6d, 0x6d, 0x4a, 0x74, 0x67, 0x4e, 0x51, 0x47,
  0x33, 0x68, 0x59, 0x41, 0x77, 0x69, 0x2f, 0x71, 0x6e, 0x77, 0x57, 0x72, 0x34, 0x39, 0x55, 0x6d,
  0x41, 0x57, 0x49, 0x68, 0x5a, 0x7a, 0x43, 0x43, 0x44, 0x51, 0x53, 0x2f, 0x62, 0x30, 0x65, 0x31,
  0x32, 0x6c, 0x5a, 0x2b, 0x6e, 0x65, 0x33, 0x32, 0x77, 0x4b, 0x0a, 0x4e, 0x45, 0x2b, 0x73, 0x35,
  0x6c, 0x31, 0x53, 0x71, 0x52, 0x41, 0x51, 0x46, 0x36, 0x4e, 0x68, 0x75, 0x4f, 0x35, 0x4d, 0x37,
  0x6d, 0x51, 0x32, 0x4a, 0x47, 0x38, 0x69, 0x6e, 0x48, 0x6e, 0x4e, 0x34, 0x59, 0x77, 0x70, 0x72,
  0x66, 0x74, 0x45, 0x6f, 0x67, 0x76, 0x52, 0x2f, 0x69, 0x7a, 0x65, 0x61, 0x6b, 0x35, 0x4d, 0x4d,
  0x7a, 0x43, 0x77, 0x36, 0x34, 0x44, 0x66, 0x44, 0x43, 0x79, 0x41, 0x0a, 0x6d, 0x72, 0x4a, 0x6f,
  0x71, 0x77, 0x75, 0x71, 0x69, 0x74, 0x34, 0x3d, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45, 0x4e,
  0x44, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d, 0x2d,
  0x2d, 0x2d, 0x0a
};

// array size is 1107
static const unsigned char v388_ca_cert[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49,
  0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x44,
  0x42, 0x44, 0x43, 0x43, 0x41, 0x6d, 0x32, 0x67, 0x41, 0x77, 0x49, 0x42, 0x41, 0x67, 0x49, 0x55,
  0x47, 0x73, 0x75, 0x41, 0x30, 0x53, 0x50, 0x37, 0x66, 0x63, 0x45, 0x73, 0x53, 0x31, 0x4e, 0x5a,
  0x63, 0x46, 0x70, 0x4f, 0x6b, 0x30, 0x59, 0x72, 0x50, 0x7a, 0x67, 0x77, 0x44, 0x51, 0x59, 0x4a,
  0x4b, 0x6f, 0x5a, 0x49, 0x68, 0x76, 0x63, 0x4e, 0x41, 0x51, 0x45, 0x46, 0x0a, 0x42, 0x51, 0x41,
  0x77, 0x67, 0x5a, 0x49, 0x78, 0x43, 0x7a, 0x41, 0x4a, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x59,
  0x54, 0x41, 0x6b, 0x70, 0x51, 0x4d, 0x51, 0x34, 0x77, 0x44, 0x41, 0x59, 0x44, 0x56, 0x51, 0x51,
  0x49, 0x44, 0x41, 0x56, 0x55, 0x62, 0x32, 0x74, 0x35, 0x62, 0x7a, 0x45, 0x67, 0x4d, 0x42, 0x34,
  0x47, 0x41, 0x31, 0x55, 0x45, 0x43, 0x67, 0x77, 0x58, 0x54, 0x6b, 0x46, 0x4e, 0x0a, 0x51, 0x30,
  0x38, 0x67, 0x51, 0x6b, 0x46, 0x4f, 0x52, 0x45, 0x46, 0x4a, 0x49, 0x45, 0x64, 0x68, 0x62, 0x57,
  0x56, 0x7a, 0x49, 0x45, 0x6c, 0x75, 0x59, 0x79, 0x34, 0x78, 0x49, 0x6a, 0x41, 0x67, 0x42, 0x67,
  0x4e, 0x56, 0x42, 0x41, 0x73, 0x4d, 0x47, 0x55, 0x35, 0x6c, 0x64, 0x48, 0x64, 0x76, 0x63, 0x6d,
  0x73, 0x67, 0x55, 0x32, 0x56, 0x79, 0x64, 0x6d, 0x56, 0x79, 0x49, 0x45, 0x52, 0x6c, 0x0a, 0x63,
  0x47, 0x46, 0x79, 0x64, 0x47, 0x31, 0x6c, 0x62, 0x6e, 0x51, 0x78, 0x4c, 0x54, 0x41, 0x72, 0x42,
  0x67, 0x4e, 0x56, 0x42, 0x41, 0x4d, 0x4d, 0x4a, 0x45, 0x35, 0x42, 0x54, 0x55, 0x4e, 0x50, 0x49,
  0x45, 0x4a, 0x42, 0x54, 0x6b, 0x52, 0x42, 0x53, 0x53, 0x42, 0x48, 0x59, 0x57, 0x31, 0x6c, 0x63,
  0x79, 0x42, 0x4a, 0x62, 0x6d, 0x4d, 0x67, 0x4c, 0x53, 0x42, 0x4f, 0x55, 0x30, 0x51, 0x67, 0x0a,
  0x51, 0x30, 0x45, 0x67, 0x55, 0x6d, 0x39, 0x76, 0x64, 0x44, 0x41, 0x67, 0x46, 0x77, 0x30, 0x79,
  0x4d, 0x54, 0x41, 0x33, 0x4d, 0x54, 0x67, 0x78, 0x4f, 0x44, 0x55, 0x35, 0x4d, 0x54, 0x42, 0x61,
  0x47, 0x41, 0x38, 0x7a, 0x4d, 0x44, 0x41, 0x78, 0x4d, 0x44, 0x6b, 0x78, 0x4f, 0x54, 0x45, 0x34,
  0x4e, 0x54, 0x6b, 0x78, 0x4d, 0x46, 0x6f, 0x77, 0x67, 0x5a, 0x49, 0x78, 0x43, 0x7a, 0x41, 0x4a,
  0x0a, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x59, 0x54, 0x41, 0x6b, 0x70, 0x51, 0x4d, 0x51, 0x34,
  0x77, 0x44, 0x41, 0x59, 0x44, 0x56, 0x51, 0x51, 0x49, 0x44, 0x41, 0x56, 0x55, 0x62, 0x32, 0x74,
  0x35, 0x62, 0x7a, 0x45, 0x67, 0x4d, 0x42, 0x34, 0x47, 0x41, 0x31, 0x55, 0x45, 0x43, 0x67, 0x77,
  0x58, 0x54, 0x6b, 0x46, 0x4e, 0x51, 0x30, 0x38, 0x67, 0x51, 0x6b, 0x46, 0x4f, 0x52, 0x45, 0x46,
  0x4a, 0x0a, 0x49, 0x45, 0x64, 0x68, 0x62, 0x57, 0x56, 0x7a, 0x49, 0x45, 0x6c, 0x75, 0x59, 0x79,
  0x34, 0x78, 0x49, 0x6a, 0x41, 0x67, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x73, 0x4d, 0x47, 0x55,
  0x35, 0x6c, 0x64, 0x48, 0x64, 0x76, 0x63, 0x6d, 0x73, 0x67, 0x55, 0x32, 0x56, 0x79, 0x64, 0x6d,
  0x56, 0x79, 0x49, 0x45, 0x52, 0x6c, 0x63, 0x47, 0x46, 0x79, 0x64, 0x47, 0x31, 0x6c, 0x62, 0x6e,
  0x51, 0x78, 0x0a, 0x4c, 0x54, 0x41, 0x72, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x4d, 0x4d, 0x4a,
  0x45, 0x35, 0x42, 0x54, 0x55, 0x4e, 0x50, 0x49, 0x45, 0x4a, 0x42, 0x54, 0x6b, 0x52, 0x42, 0x53,
  0x53, 0x42, 0x48, 0x59, 0x57, 0x31, 0x6c, 0x63, 0x79, 0x42, 0x4a, 0x62, 0x6d, 0x4d, 0x67, 0x4c,
  0x53, 0x42, 0x4f, 0x55, 0x30, 0x51, 0x67, 0x51, 0x30, 0x45, 0x67, 0x55, 0x6d, 0x39, 0x76, 0x64,
  0x44, 0x43, 0x42, 0x0a, 0x6e, 0x7a, 0x41, 0x4e, 0x42, 0x67, 0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47,
  0x39, 0x77, 0x30, 0x42, 0x41, 0x51, 0x45, 0x46, 0x41, 0x41, 0x4f, 0x42, 0x6a, 0x51, 0x41, 0x77,
  0x67, 0x59, 0x6b, 0x43, 0x67, 0x59, 0x45, 0x41, 0x33, 0x47, 0x54, 0x4f, 0x41, 0x2f, 0x4a, 0x31,
  0x62, 0x69, 0x41, 0x6e, 0x71, 0x6c, 0x79, 0x70, 0x33, 0x36, 0x43, 0x55, 0x77, 0x55, 0x35, 0x42,
  0x63, 0x64, 0x63, 0x71, 0x0a, 0x39, 0x37, 0x4d, 0x61, 0x54, 0x77, 0x37, 0x68, 0x4d, 0x55, 0x66,
  0x54, 0x71, 0x58, 0x57, 0x33, 0x32, 0x35, 0x33, 0x69, 0x71, 0x48, 0x4b, 0x33, 0x50, 0x72, 0x68,
  0x58, 0x78, 0x45, 0x78, 0x7a, 0x39, 0x4a, 0x37, 0x6e, 0x70, 0x79, 0x2b, 0x31, 0x34, 0x36, 0x76,
  0x31, 0x75, 0x38, 0x4c, 0x33, 0x4e, 0x70, 0x66, 0x41, 0x48, 0x44, 0x58, 0x37, 0x45, 0x57, 0x6c,
  0x4c, 0x48, 0x51, 0x58, 0x57, 0x0a, 0x6b, 0x4e, 0x6f, 0x49, 0x37, 0x4a, 0x6b, 0x56, 0x56, 0x59,
  0x73, 0x32, 0x35, 0x66, 0x71, 0x58, 0x4c, 0x41, 0x4b, 0x79, 0x49, 0x4a, 0x4a, 0x4f, 0x42, 0x48,
  0x52, 0x43, 0x66, 0x46, 0x4f, 0x38, 0x50, 0x46, 0x79, 0x35, 0x32, 0x51, 0x53, 0x4c, 0x5a, 0x53,
  0x70, 0x7a, 0x55, 0x63, 0x68, 0x58, 0x4b, 0x61, 0x75, 0x73, 0x79, 0x79, 0x45, 0x6b, 0x32, 0x44,
  0x78, 0x70, 0x2f, 0x78, 0x61, 0x69, 0x0a, 0x65, 0x49, 0x64, 0x69, 0x68, 0x50, 0x76, 0x37, 0x49,
  0x55, 0x50, 0x6e, 0x70, 0x64, 0x30, 0x43, 0x41, 0x77, 0x45, 0x41, 0x41, 0x61, 0x4e, 0x54, 0x4d,
  0x46, 0x45, 0x77, 0x48, 0x51, 0x59, 0x44, 0x56, 0x52, 0x30, 0x4f, 0x42, 0x42, 0x59, 0x45, 0x46,
  0x45, 0x77, 0x37, 0x67, 0x4b, 0x32, 0x4c, 0x34, 0x48, 0x2f, 0x79, 0x31, 0x52, 0x4a, 0x67, 0x35,
  0x32, 0x64, 0x33, 0x66, 0x64, 0x70, 0x62, 0x0a, 0x78, 0x6c, 0x71, 0x4a, 0x4d, 0x42, 0x38, 0x47,
  0x41, 0x31, 0x55, 0x64, 0x49, 0x77, 0x51, 0x59, 0x4d, 0x42, 0x61, 0x41, 0x46, 0x45, 0x77, 0x37,
  0x67, 0x4b, 0x32, 0x4c, 0x34, 0x48, 0x2f, 0x79, 0x31, 0x52, 0x4a, 0x67, 0x35, 0x32, 0x64, 0x33,
  0x66, 0x64, 0x70, 0x62, 0x78, 0x6c, 0x71, 0x4a, 0x4d, 0x41, 0x38, 0x47, 0x41, 0x31, 0x55, 0x64,
  0x45, 0x77, 0x51, 0x49, 0x4d, 0x41, 0x59, 0x42, 0x0a, 0x41, 0x66, 0x38, 0x43, 0x41, 0x51, 0x41,
  0x77, 0x44, 0x51, 0x59, 0x4a, 0x4b, 0x6f, 0x5a, 0x49, 0x68, 0x76, 0x63, 0x4e, 0x41, 0x51, 0x45,
  0x46, 0x42, 0x51, 0x41, 0x44, 0x67, 0x59, 0x45, 0x41, 0x58, 0x46, 0x4c, 0x73, 0x70, 0x52, 0x47,
  0x67, 0x2b, 0x53, 0x4b, 0x73, 0x6f, 0x44, 0x33, 0x72, 0x48, 0x4c, 0x68, 0x47, 0x32, 0x31, 0x2f,
  0x74, 0x31, 0x64, 0x64, 0x6b, 0x6b, 0x4e, 0x70, 0x54, 0x0a, 0x78, 0x56, 0x4d, 0x31, 0x66, 0x4b,
  0x61, 0x33, 0x6a, 0x45, 0x45, 0x35, 0x7a, 0x62, 0x6d, 0x6d, 0x4a, 0x74, 0x67, 0x4e, 0x51, 0x47,
  0x33, 0x68, 0x59, 0x41, 0x77, 0x69, 0x2f, 0x71, 0x6e, 0x77, 0x57, 0x72, 0x34, 0x39, 0x55, 0x6d,
  0x41, 0x57, 0x49, 0x68, 0x5a, 0x7a, 0x43, 0x43, 0x44, 0x51, 0x53, 0x2f, 0x62, 0x30, 0x65, 0x31,
  0x32, 0x6c, 0x5a, 0x2b, 0x6e, 0x65, 0x33, 0x32, 0x77, 0x4b, 0x0a, 0x4e, 0x45, 0x2b, 0x73, 0x35,
  0x6c, 0x31, 0x53, 0x71, 0x52, 0x41, 0x51, 0x46, 0x36, 0x4e, 0x68, 0x75, 0x4f, 0x35, 0x4d, 0x37,
  0x6d, 0x51, 0x32, 0x4a, 0x47, 0x38, 0x69, 0x6e, 0x48, 0x6e, 0x4e, 0x34, 0x59, 0x77, 0x70, 0x72,
  0x66, 0x74, 0x45, 0x6f, 0x67, 0x76, 0x52, 0x2f, 0x69, 0x7a, 0x65, 0x61, 0x6b, 0x35, 0x4d, 0x4d,
  0x7a, 0x43, 0x77, 0x36, 0x34, 0x44, 0x66, 0x44, 0x43, 0x79, 0x41, 0x0a, 0x6d, 0x72, 0x4a, 0x6f,
  0x71, 0x77, 0x75, 0x71, 0x69, 0x74, 0x34, 0x3d, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45, 0x4e,
  0x44, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d, 0x2d,
  0x2d, 0x2d, 0x0a
};

// array size is 912
static const unsigned char terminal_key_v388[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x50, 0x52, 0x49, 0x56, 0x41,
  0x54, 0x45, 0x20, 0x4b, 0x45, 0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x43,
  0x64, 0x51, 0x49, 0x42, 0x41, 0x44, 0x41, 0x4e, 0x42, 0x67, 0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47,
  0x39, 0x77, 0x30, 0x42, 0x41, 0x51, 0x45, 0x46, 0x41, 0x41, 0x53, 0x43, 0x41, 0x6c, 0x38, 0x77,
  0x67, 0x67, 0x4a, 0x62, 0x41, 0x67, 0x45, 0x41, 0x41, 0x6f, 0x47, 0x42, 0x41, 0x4e, 0x78, 0x6b,
  0x7a, 0x67, 0x50, 0x79, 0x64, 0x57, 0x34, 0x67, 0x4a, 0x36, 0x70, 0x63, 0x0a, 0x71, 0x64, 0x2b,
  0x67, 0x6c, 0x4d, 0x46, 0x4f, 0x51, 0x58, 0x48, 0x58, 0x4b, 0x76, 0x65, 0x7a, 0x47, 0x6b, 0x38,
  0x4f, 0x34, 0x54, 0x46, 0x48, 0x30, 0x36, 0x6c, 0x31, 0x74, 0x39, 0x75, 0x64, 0x34, 0x71, 0x68,
  0x79, 0x74, 0x7a, 0x36, 0x34, 0x56, 0x38, 0x52, 0x4d, 0x63, 0x2f, 0x53, 0x65, 0x35, 0x36, 0x63,
  0x76, 0x74, 0x65, 0x4f, 0x72, 0x39, 0x62, 0x76, 0x43, 0x39, 0x7a, 0x61, 0x58, 0x0a, 0x77, 0x42,
  0x77, 0x31, 0x2b, 0x78, 0x46, 0x70, 0x53, 0x78, 0x30, 0x46, 0x31, 0x70, 0x44, 0x61, 0x43, 0x4f,
  0x79, 0x5a, 0x46, 0x56, 0x57, 0x4c, 0x4e, 0x75, 0x58, 0x36, 0x6c, 0x79, 0x77, 0x43, 0x73, 0x69,
  0x43, 0x53, 0x54, 0x67, 0x52, 0x30, 0x51, 0x6e, 0x78, 0x54, 0x76, 0x44, 0x78, 0x63, 0x75, 0x64,
  0x6b, 0x45, 0x69, 0x32, 0x55, 0x71, 0x63, 0x31, 0x48, 0x49, 0x56, 0x79, 0x6d, 0x72, 0x0a, 0x72,
  0x4d, 0x73, 0x68, 0x4a, 0x4e, 0x67, 0x38, 0x61, 0x66, 0x38, 0x57, 0x6f, 0x6e, 0x69, 0x48, 0x59,
  0x6f, 0x54, 0x37, 0x2b, 0x79, 0x46, 0x44, 0x35, 0x36, 0x58, 0x64, 0x41, 0x67, 0x4d, 0x42, 0x41,
  0x41, 0x45, 0x43, 0x67, 0x59, 0x42, 0x32, 0x38, 0x53, 0x78, 0x42, 0x38, 0x78, 0x4d, 0x76, 0x47,
  0x4d, 0x6c, 0x76, 0x5a, 0x73, 0x30, 0x43, 0x39, 0x46, 0x33, 0x7a, 0x71, 0x54, 0x45, 0x6d, 0x0a,
  0x71, 0x42, 0x48, 0x33, 0x56, 0x6b, 0x43, 0x48, 0x6c, 0x43, 0x63, 0x79, 0x65, 0x6d, 0x6f, 0x66,
  0x31, 0x58, 0x6b, 0x68, 0x58, 0x43, 0x63, 0x62, 0x38, 0x37, 0x55, 0x7a, 0x63, 0x7a, 0x64, 0x69,
  0x71, 0x45, 0x51, 0x59, 0x4b, 0x37, 0x34, 0x6e, 0x65, 0x31, 0x63, 0x31, 0x68, 0x50, 0x4e, 0x38,
  0x6e, 0x71, 0x37, 0x65, 0x56, 0x71, 0x32, 0x47, 0x54, 0x42, 0x65, 0x76, 0x67, 0x41, 0x31, 0x7a,
  0x0a, 0x79, 0x38, 0x4f, 0x49, 0x31, 0x33, 0x64, 0x6c, 0x51, 0x75, 0x30, 0x6f, 0x71, 0x5a, 0x4c,
  0x41, 0x65, 0x70, 0x43, 0x6f, 0x2b, 0x78, 0x4d, 0x38, 0x6a, 0x66, 0x56, 0x66, 0x55, 0x4a, 0x6b,
  0x30, 0x35, 0x74, 0x64, 0x42, 0x76, 0x65, 0x64, 0x4e, 0x70, 0x4c, 0x58, 0x6d, 0x55, 0x6f, 0x6a,
  0x36, 0x7a, 0x4b, 0x74, 0x63, 0x45, 0x75, 0x57, 0x6d, 0x4c, 0x75, 0x55, 0x53, 0x2f, 0x31, 0x4f,
  0x45, 0x0a, 0x72, 0x59, 0x56, 0x58, 0x62, 0x71, 0x35, 0x58, 0x70, 0x4a, 0x65, 0x4d, 0x62, 0x59,
  0x4f, 0x54, 0x34, 0x51, 0x4a, 0x42, 0x41, 0x50, 0x75, 0x63, 0x59, 0x63, 0x43, 0x52, 0x39, 0x32,
  0x46, 0x45, 0x71, 0x46, 0x4e, 0x74, 0x78, 0x56, 0x78, 0x4f, 0x50, 0x79, 0x59, 0x53, 0x63, 0x53,
  0x2f, 0x7a, 0x44, 0x70, 0x6e, 0x54, 0x41, 0x6c, 0x49, 0x59, 0x38, 0x44, 0x45, 0x6b, 0x51, 0x4d,
  0x74, 0x39, 0x0a, 0x79, 0x5a, 0x39, 0x32, 0x6c, 0x79, 0x2f, 0x78, 0x66, 0x72, 0x6a, 0x42, 0x31,
  0x4e, 0x52, 0x73, 0x57, 0x6c, 0x7a, 0x79, 0x4d, 0x75, 0x69, 0x66, 0x6c, 0x2b, 0x53, 0x68, 0x5a,
  0x32, 0x30, 0x36, 0x2f, 0x63, 0x4b, 0x5a, 0x79, 0x67, 0x38, 0x4e, 0x75, 0x4c, 0x6b, 0x43, 0x51,
  0x51, 0x44, 0x67, 0x50, 0x51, 0x52, 0x4b, 0x2f, 0x34, 0x42, 0x62, 0x49, 0x42, 0x79, 0x42, 0x47,
  0x71, 0x2b, 0x72, 0x0a, 0x6e, 0x62, 0x73, 0x6a, 0x4f, 0x76, 0x2f, 0x79, 0x70, 0x48, 0x36, 0x36,
  0x49, 0x44, 0x68, 0x73, 0x2f, 0x42, 0x62, 0x62, 0x6e, 0x42, 0x55, 0x59, 0x4d, 0x59, 0x2b, 0x6b,
  0x2b, 0x7a, 0x38, 0x2f, 0x37, 0x6e, 0x34, 0x32, 0x62, 0x61, 0x74, 0x4e, 0x79, 0x47, 0x66, 0x43,
  0x4a, 0x36, 0x55, 0x56, 0x35, 0x76, 0x62, 0x52, 0x64, 0x73, 0x47, 0x7a, 0x6a, 0x73, 0x61, 0x4f,
  0x6b, 0x65, 0x44, 0x64, 0x0a, 0x64, 0x62, 0x78, 0x46, 0x41, 0x6b, 0x41, 0x65, 0x77, 0x59, 0x66,
  0x6c, 0x62, 0x54, 0x4a, 0x4c, 0x44, 0x6f, 0x52, 0x77, 0x35, 0x6b, 0x73, 0x6f, 0x74, 0x46, 0x76,
  0x64, 0x77, 0x49, 0x56, 0x62, 0x63, 0x68, 0x2b, 0x48, 0x79, 0x42, 0x5a, 0x52, 0x69, 0x4d, 0x44,
  0x62, 0x6b, 0x4f, 0x33, 0x6d, 0x73, 0x66, 0x4b, 0x53, 0x62, 0x6f, 0x47, 0x65, 0x6c, 0x36, 0x75,
  0x41, 0x31, 0x42, 0x69, 0x36, 0x0a, 0x2b, 0x70, 0x2b, 0x31, 0x47, 0x74, 0x6f, 0x45, 0x4f, 0x33,
  0x58, 0x71, 0x75, 0x5a, 0x77, 0x56, 0x36, 0x77, 0x38, 0x58, 0x32, 0x59, 0x71, 0x73, 0x65, 0x6d,
  0x58, 0x70, 0x41, 0x6b, 0x41, 0x4c, 0x39, 0x78, 0x57, 0x49, 0x6a, 0x76, 0x4f, 0x68, 0x61, 0x53,
  0x67, 0x38, 0x4e, 0x31, 0x6c, 0x53, 0x56, 0x66, 0x74, 0x4c, 0x57, 0x50, 0x57, 0x5a, 0x55, 0x2b,
  0x2b, 0x7a, 0x4c, 0x4d, 0x37, 0x31, 0x0a, 0x2b, 0x63, 0x6d, 0x61, 0x64, 0x45, 0x79, 0x6e, 0x32,
  0x74, 0x55, 0x6f, 0x58, 0x34, 0x4f, 0x7a, 0x66, 0x4a, 0x31, 0x64, 0x65, 0x43, 0x4b, 0x67, 0x35,
  0x75, 0x57, 0x71, 0x64, 0x59, 0x55, 0x59, 0x6b, 0x78, 0x64, 0x75, 0x65, 0x2b, 0x44, 0x41, 0x48,
  0x51, 0x6b, 0x35, 0x53, 0x45, 0x31, 0x77, 0x59, 0x37, 0x7a, 0x6c, 0x41, 0x6b, 0x42, 0x47, 0x37,
  0x67, 0x4e, 0x71, 0x74, 0x78, 0x35, 0x30, 0x0a, 0x38, 0x4a, 0x4d, 0x61, 0x44, 0x4b, 0x71, 0x6d,
  0x46, 0x6c, 0x62, 0x33, 0x56, 0x46, 0x42, 0x47, 0x52, 0x34, 0x58, 0x54, 0x53, 0x77, 0x70, 0x75,
  0x48, 0x31, 0x4f, 0x38, 0x66, 0x37, 0x74, 0x62, 0x5a, 0x4a, 0x76, 0x74, 0x4b, 0x5a, 0x36, 0x56,
  0x56, 0x65, 0x4b, 0x58, 0x2b, 0x2b, 0x6b, 0x41, 0x2b, 0x62, 0x70, 0x41, 0x59, 0x4b, 0x76, 0x41,
  0x58, 0x45, 0x43, 0x34, 0x38, 0x6b, 0x39, 0x77, 0x0a, 0x58, 0x6f, 0x59, 0x64, 0x57, 0x48, 0x56,
  0x79, 0x62, 0x4e, 0x68, 0x38, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45, 0x4e, 0x44, 0x20, 0x50,
  0x52, 0x49, 0x56, 0x41, 0x54, 0x45, 0x20, 0x4b, 0x45, 0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a
};

static void prepareCerts() {
	//data_jp/network/certs
	FILE* file2 = fopen("./data_jp/network/certs/terminal-cert_v388.pem", "wb");
	fwrite(terminal_cert_v388, 1, sizeof(terminal_cert_v388), file2);
	fclose(file2);

	FILE* cacert = fopen("./data_jp/network/certs/v388-ca-cert.pem", "wb");
	fwrite(v388_ca_cert, 1, sizeof(v388_ca_cert), cacert);
	fclose(cacert);

	FILE* termkey = fopen("./data_jp/network/private/terminal-key_v388.pem", "wb");
	fwrite(terminal_key_v388, 1, sizeof(terminal_key_v388), termkey);
	fclose(termkey);

	return;
}

static InitFunction Wmmt5Func([]() {

	prepareCerts();

	// Alloc debug console
	FreeConsole();
	AllocConsole();

	SetConsoleTitle(L"W5X Console | Modified by Kiana");
	
	FILE* pNewStdout = nullptr;
	FILE* pNewStderr = nullptr;
	FILE* pNewStdin = nullptr;

	::freopen_s(&pNewStdout, "CONOUT$", "w", stdout);
	::freopen_s(&pNewStderr, "CONOUT$", "w", stderr);
	::freopen_s(&pNewStdin, "CONIN$", "r", stdin);
	std::cout.clear();
	std::cerr.clear();
	std::cin.clear();
	std::wcout.clear();
	std::wcerr.clear();
	std::wcin.clear();

	isUpdate5 = true;

	// enable all game output 1408F1200
	// For AMAuthd, CRC32 = 7E804704
	puts("W5X Rev 2.12.07 OpenParrotLoader");
	puts("24/05/12 02:35:27 CST By KianaKaslana");
	puts("Powered By KianaKaslana");
	puts("Cleared Recent Cache to prevent system crash after filelist...");
	system("del /f /s /q .\\Fcontents\\*.* && del /f /q .\\*.db && del /f /q .\\hints.dat && del /f /q .\\*.bin && del /f /q .\\*.bin.gz");

	// Records if terminal mode is enabled
	bool isTerminal = false;

	// If terminal mode is set in the general settings
	if (ToBool(config["General"]["TerminalMode"])) {
		// Terminal mode is set
		isTerminal = true;
	}

	// Get the network adapter ip address from the general settings
	std::string networkip = config["General"]["NetworkAdapterIP"];

	// If the ip address is not blank
	if (!networkip.empty()) {
		// Overwrite the default ip address
		ipaddrdxplus = networkip.c_str();
	}

	hookPort = "COM3";
	imageBasedxplus = (uintptr_t)GetModuleHandleA(0);

	MH_Initialize();

	// Hook dongle funcs
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_write", dxpHook_hasp_write, NULL);
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_read", dxpHook_hasp_read, NULL);
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_get_size", dxpHook_hasp_get_size, NULL);
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_decrypt", dxpHook_hasp_decrypt, NULL);
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_encrypt", dxpHook_hasp_encrypt, NULL);
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_logout", dxpHook_hasp_logout, NULL);
	MH_CreateHookApi(L"hasp_windows_x64_106482.dll", "hasp_login", dxpHook_hasp_login, NULL);
	MH_CreateHookApi(L"WS2_32", "bind", Hook_bind_w5p, reinterpret_cast<LPVOID*>(&pbindw5p));
	MH_CreateHookApi(L"kernel32", "OutputDebugStringA", Hook_OutputDebugStringA, NULL);

	// Give me the HWND please maxitune
	MH_CreateHookApi(L"user32", "ShowWindow", Hook_ShowWindow, reinterpret_cast<LPVOID*>(&pShowWindow));
	pMaxituneWndProc = (WindowProcedure_t)(imageBasedxplus + 0xBD0890);// (same as below)
	//pMaxituneWndProc = (WindowProcedure_t)(hook::get_pattern("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 30 8B EA BA EB FF FF FF 49 8B F9 49 8B F0 48 8B D9 FF 15 ? ? ? 00 48 85 C0 74 1D 4C", 0));

	//load banapass emu
	if (ToBool(config["General"]["EnableOpenBanapass"])) {
		LoadLibraryA(".\\OpenBanaInjector.dll");
	}

	// Prevents game from setting time, thanks pockywitch!
	MH_CreateHookApi(L"KERNEL32", "SetSystemTime", Hook_SetSystemTime, reinterpret_cast<LPVOID*>(&pSetSystemTime));

	GenerateDongleDataDxp(isTerminal);

	injector::WriteMemory<uint8_t>(hook::get_pattern("85 C9 0F 94 C0 84 C0 0F 94 C0 84 C0 75 ? 40 32 F6 EB ?", 0x15), 0, true); //patches out dongle error2 (doomer)

	injector::MakeNOP(hook::get_pattern("83 C0 FD 83 F8 01 76 ? 49 8D ? ? ? ? 00 00"), 6);

	if (ToBool(config["General"]["WhiteScreenFix"])) {
		injector::WriteMemory<DWORD>(hook::get_pattern("48 8B C4 55 57 41 54 41 55 41 56 48 8D 68 A1 48 81 EC 90 00 00 00 48 C7 45 D7 FE FF FF FF 48 89 58 08 48 89 70 18 45 33 F6 4C 89 75 DF 33 C0 48 89 45 E7", 0), 0x90C3C032, true);
	}

	{
		auto location = hook::get_pattern<char>("41 3B C7 74 0E 48 8D 8F B8 00 00 00 BA F6 01 00 00 EB 6E 48 8D 8F A0 00 00 00");

		injector::WriteMemory<uint8_t>(location + 3, 0xEB, true); //patches content router (doomer)

		// Skip some jnz
		injector::MakeNOP(location + 0x22, 2); //patches ip addr error again (doomer)

		// Skip some jnz
		injector::MakeNOP(location + 0x33, 2); //patches ip aaddr error(doomer)
	}

	// Terminal mode is off
	if (!isTerminal) {
		injector::MakeNOP(hook::get_pattern("74 ? 80 7B 31 00 75 ? 48 8B 43 10 80 78 31 00 75 1A 48 8B D8 48 8B 00 80 78 31 00 75 ? 48 8B D8"), 2); //terminal on same machine patch
	}
	else {	//terminal mode patches
		safeJMP(hook::get_pattern("0F B6 41 05 2C 30 3C 09 77 04 0F BE C0 C3 83 C8 FF C3"), ReturnTrue);
		safeJMP(hook::get_pattern("8B 01 0F B6 40 78 C3 CC CC CC CC"), ReturnTrue);
	}

	PathFix();

	

	// Enable all print
	injector::MakeNOP(imageBasedxplus + 0x8F15A3, 6);

	//Fix crash when saving story mode and Time attack
	injector::MakeNOP(imageBasedxplus + 0xE8DE7, 5);

	MH_EnableHook(MH_ALL_HOOKS);

	}, GameID::WMMT5DXPlus);
#endif
#pragma optimize("", on)