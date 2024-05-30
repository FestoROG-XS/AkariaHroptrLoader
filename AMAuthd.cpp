#include <StdInc.h>
#pragma optimize("", off)
#include <iphlpapi.h>
#include <winsock2.h>
#include <sysinfoapi.h>
#include "Utility/InitFunction.h"
#include "Functions/Global.h"
#include <string.h>

#ifndef _M_IX86
u_short HttpPort = 80;

// PrepareAMAuth
static const unsigned char AMConfigW6W[] = R"(
[AMUpdaterConfig] 
;; AMUpdater Configuration
amucfg-title=WANGAN MIDNIGHT MAXIMUM TUNE 6RR
amucfg-lang=EN
amucfg-countdown=5
amucfg-h_resol=1360
amucfg-v_resol=768
amucfg-logfile=.\amupdater.log
amucfg-game_rev=3

[AMAuthdConfig] 
amdcfg-authType=ALL.NET
amdcfg-sleepTime=50
amdcfg-resoNameTimeout=180
amdcfg-writableConfig=.\WritableConfig.ini
amdcfg-showConsole=ENABLE
amdcfg-logfile=
amdcfg-export_log=
amdcfg-offlineMode=DISABLE

[AllnetConfig] 
allcfg-gameID=SBWJ
allcfg-gameVer=5.00

[AllnetOptionRevalTime]
allopt-reval_hour=7
allopt-reval_minute=0
allopt-reval_second=0

[AllnetOptionTimeout]
allopt-timeout_connect=60000  
allopt-timeout_send=60000
allopt-timeout_recv=60000

[MuchaAppConfig]
appcfg-logfile=.\muchaapp.log
appcfg-loglevel=INFO

[MuchaSysConfig]
syscfg-daemon_exe=.\MuchaBin\muchacd.exe
syscfg-daemon_pidfile=.\muchacd.pid
syscfg-daemon_logfile=.\muchacd.log
syscfg-daemon_loglevel=INFO
syscfg-daemon_listen=tcp:0.0.0.0:12345
syscfg-client_connect=tcp:127.0.0.1:12345

[MuchaCAConfig]
cacfg-game_cd=W6W1
cacfg-game_ver=05.03
cacfg-game_board_type=0
cacfg-game_board_id=W6W
cacfg-auth_server_url=https://0.0.0.0:10082/
cacfg-auth_server_sslverify=0
cacfg-auth_server_sslcafile=.\front.mucha-prd.nbgi-amnet.jp.cacert.pem
cacfg-auth_server_timeout=300
cacfg-interval_ainfo_renew=1800
cacfg-interval_ainfo_retry=60

[MuchaDtConfig]
dtcfg-dl_product_id=0x57355031
dtcfg-dl_chunk_size=0x10000
dtcfg-dl_image_path=.\dl_image
dtcfg-dl_image_size=0
dtcfg-dl_image_type=RAW
dtcfg-dl_image_crypt_key=0x45520913
dtcfg-dl_log_level=INFO
dtcfg-dl_lan_crypt_key=0xfz26s7201m68952x
dtcfg-dl_lan_broadcast_interval=1000
dtcfg-dl_lan_udp_port=8765
dtcfg-dl_lan_bandwidth_limit=0
dtcfg-dl_lan_broadcast_address=0.0.0.0
dtcfg-dl_wan_retry_limit=
dtcfg-dl_wan_retry_interval=
dtcfg-dl_wan_send_timeout=
dtcfg-dl_wan_recv_timeout=
dtcfg-dl_lan_retry_limit=
dtcfg-dl_lan_retry_interval=
dtcfg-dl_lan_send_timeout=
dtcfg-dl_lan_recv_timeout=

[MuchaDtModeConfig]
dtmode-io_dir=E:\
dtmode-io_file=W6W10JPN
dtmode-io_conv=DECEXP
dtmode-io_passphrase=Qx8hJ1KilweAp5Xm
)";

static const unsigned char AMConfigW5P[] = R"(
[AMUpdaterConfig] 
;; AMUpdater Configuration
amucfg-title=WANGAN MIDNIGHT MAXIMUM TUNE 5DX PLUS
amucfg-lang=EN
amucfg-countdown=5
amucfg-h_resol=1360
amucfg-v_resol=768
amucfg-logfile=.\amupdater.log
amucfg-game_rev=2

[AMAuthdConfig] 
amdcfg-authType=ALL.NET
amdcfg-sleepTime=50
amdcfg-resoNameTimeout=180
amdcfg-writableConfig=.\WritableConfig.ini
amdcfg-showConsole=ENABLE
amdcfg-logfile=
amdcfg-export_log=
amdcfg-offlineMode=DISABLE

[AllnetConfig] 
allcfg-gameID=SBWJ
allcfg-gameVer=5.00

[AllnetOptionRevalTime]
allopt-reval_hour=7
allopt-reval_minute=0
allopt-reval_second=0

[AllnetOptionTimeout]
allopt-timeout_connect=60000  
allopt-timeout_send=60000
allopt-timeout_recv=60000

[MuchaAppConfig]
appcfg-logfile=.\muchaapp.log
appcfg-loglevel=INFO

[MuchaSysConfig]
syscfg-daemon_exe=.\MuchaBin\muchacd.exe
syscfg-daemon_pidfile=.\muchacd.pid
syscfg-daemon_logfile=.\muchacd.log
syscfg-daemon_loglevel=INFO
syscfg-daemon_listen=tcp:0.0.0.0:12345
syscfg-client_connect=tcp:127.0.0.1:12345

[MuchaCAConfig]
cacfg-game_cd=W5X2
cacfg-game_ver=12.07
cacfg-game_board_type=0
cacfg-game_board_id=WM5
cacfg-auth_server_url=https://0.0.0.0:10082/
cacfg-auth_server_sslverify=0
cacfg-auth_server_sslcafile=.\front.mucha-prd.nbgi-amnet.jp.cacert.pem
cacfg-auth_server_timeout=300
cacfg-interval_ainfo_renew=1800
cacfg-interval_ainfo_retry=60

[MuchaDtConfig]
dtcfg-dl_product_id=0x57355031
dtcfg-dl_chunk_size=0x10000
dtcfg-dl_image_path=.\dl_image
dtcfg-dl_image_size=0
dtcfg-dl_image_type=RAW
dtcfg-dl_image_crypt_key=0x45520913
dtcfg-dl_log_level=INFO
dtcfg-dl_lan_crypt_key=0xfz26s7201m68952x
dtcfg-dl_lan_broadcast_interval=1000
dtcfg-dl_lan_udp_port=8765
dtcfg-dl_lan_bandwidth_limit=0
dtcfg-dl_lan_broadcast_address=0.0.0.0
dtcfg-dl_wan_retry_limit=
dtcfg-dl_wan_retry_interval=
dtcfg-dl_wan_send_timeout=
dtcfg-dl_wan_recv_timeout=
dtcfg-dl_lan_retry_limit=
dtcfg-dl_lan_retry_interval=
dtcfg-dl_lan_send_timeout=
dtcfg-dl_lan_recv_timeout=

[MuchaDtModeConfig]
dtmode-io_dir=E:\
dtmode-io_file=W5X10JPN
dtmode-io_conv=DECEXP
dtmode-io_passphrase=Qx8hJ1KilweAp5Xm
)";

static const unsigned char AMConfigW6R[] = R"(
[AMUpdaterConfig] 
;; AMUpdater Configuration
amucfg-title=WANGAN MIDNIGHT MAXIMUM TUNE 6R
amucfg-lang=EN
amucfg-countdown=5
amucfg-h_resol=1360
amucfg-v_resol=768
amucfg-logfile=.\amupdater.log
amucfg-game_rev=2

[AMAuthdConfig] 
amdcfg-authType=ALL.NET
amdcfg-sleepTime=50
amdcfg-resoNameTimeout=180
amdcfg-writableConfig=.\WritableConfig.ini
amdcfg-showConsole=ENABLE
amdcfg-logfile=
amdcfg-export_log=
amdcfg-offlineMode=DISABLE

[AllnetConfig] 
allcfg-gameID=SBWJ
allcfg-gameVer=5.00

[AllnetOptionRevalTime]
allopt-reval_hour=7
allopt-reval_minute=0
allopt-reval_second=0

[AllnetOptionTimeout]
allopt-timeout_connect=60000  
allopt-timeout_send=60000
allopt-timeout_recv=60000

[MuchaAppConfig]
appcfg-logfile=.\muchaapp.log
appcfg-loglevel=INFO

[MuchaSysConfig]
syscfg-daemon_exe=.\MuchaBin\muchacd.exe
syscfg-daemon_pidfile=.\muchacd.pid
syscfg-daemon_logfile=.\muchacd.log
syscfg-daemon_loglevel=INFO
syscfg-daemon_listen=tcp:0.0.0.0:12345
syscfg-client_connect=tcp:127.0.0.1:12345

[MuchaCAConfig]
cacfg-game_cd=W6R1
cacfg-game_ver=00.08
cacfg-game_board_type=0
cacfg-game_board_id=W6R
cacfg-auth_server_url=https://0.0.0.0:10082/
cacfg-auth_server_sslverify=0
cacfg-auth_server_sslcafile=.\front.mucha-prd.nbgi-amnet.jp.cacert.pem
cacfg-auth_server_timeout=300
cacfg-interval_ainfo_renew=1800
cacfg-interval_ainfo_retry=60

[MuchaDtConfig]
dtcfg-dl_product_id=0x57355031
dtcfg-dl_chunk_size=0x10000
dtcfg-dl_image_path=.\dl_image
dtcfg-dl_image_size=0
dtcfg-dl_image_type=RAW
dtcfg-dl_image_crypt_key=0x45520913
dtcfg-dl_log_level=INFO
dtcfg-dl_lan_crypt_key=0xfz26s7201m68952x
dtcfg-dl_lan_broadcast_interval=1000
dtcfg-dl_lan_udp_port=8765
dtcfg-dl_lan_bandwidth_limit=0
dtcfg-dl_lan_broadcast_address=0.0.0.0
dtcfg-dl_wan_retry_limit=
dtcfg-dl_wan_retry_interval=
dtcfg-dl_wan_send_timeout=
dtcfg-dl_wan_recv_timeout=
dtcfg-dl_lan_retry_limit=
dtcfg-dl_lan_retry_interval=
dtcfg-dl_lan_send_timeout=
dtcfg-dl_lan_recv_timeout=

[MuchaDtModeConfig]
dtmode-io_dir=E:\
dtmode-io_file=W6R10JPN
dtmode-io_conv=DECEXP
dtmode-io_passphrase=Qx8hJ1KilweAp5Xm
)";


static void PrepareAMAuth() {

	// Prepare For AMAuth
	FILE* AMConfigWrite = fopen("AMConfig.ini", "w");
	std::string GameVersion = config["Authentication"]["GameVersion"];
	
	enum GameVer {
		W5P=0,
		W6R=1,
		W6W=2
	};

	GameVer actualGameVer = W5P;
	if ((GameVersion.compare("W5P10JPN05") == 0) || (GameVersion.compare("W5X10JPN12") == 0)) actualGameVer = W5P;
	else if ((GameVersion.compare("W6R10JPN00") == 0)) actualGameVer = W6R;
	else if ((GameVersion.compare("W6W10JPN05") == 0)) actualGameVer = W6W;
	printf("GameVer is %d", actualGameVer);

	switch (actualGameVer) {
	case W5P:
		fwrite(AMConfigW5P, 1, sizeof(AMConfigW5P), AMConfigWrite);
		fclose(AMConfigWrite);
		break;
	case W6R:
		fwrite(AMConfigW6R, 1, sizeof(AMConfigW6R), AMConfigWrite);
		fclose(AMConfigWrite);
		break;
	case W6W:
		fwrite(AMConfigW6W, 1, sizeof(AMConfigW6W), AMConfigWrite);
		fclose(AMConfigWrite);
		break;
	default:
		fwrite(AMConfigW5P, 1, sizeof(AMConfigW5P), AMConfigWrite);
		fclose(AMConfigWrite);
		break;
	}


	char WritableConfigFinal[1024] = "";
	std::string NETID = config["Authentication"]["NetID"];
	strcat(WritableConfigFinal, "[RuntimeConfig]\nmode=\nnetID=");
	strcat(WritableConfigFinal, NETID.c_str());
	strcat(WritableConfigFinal, "\nserialID=");

	FILE* WritableConfigWrite = fopen("WritableConfig.ini", "w");
	fwrite(WritableConfigFinal, 1, strlen(WritableConfigFinal), WritableConfigWrite);
	fclose(WritableConfigWrite);
}

linb::ini myconfig;

static char gatewayAddressStr[256];

bool (WINAPI* orig_SetSystemTime)(const SYSTEMTIME* lpSystemTime);

static bool SetSystemTimeHook(const SYSTEMTIME* lpSystemTime) {
	return 1;
}

u_short(PASCAL FAR* htons_orig)(u_short hostshort);

static u_short htonsHook(u_short hostshort) {
#if _DEBUG
	info("htons: %i", hostshort);
#endif

	if (hostshort == 80) {
#ifdef _DEBUG
		info("replacing port...");
#endif
		return htons_orig(HttpPort);
	}
	else {
		return htons_orig(hostshort);
	}
}

int(WSAAPI* g_origgetaddrinfoo)(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult);

int WSAAPI getaddrinfoHookAMAuth(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult) {
#if _DEBUG
	info("getaddrinfo: %s, %s", pNodeName, pServiceName);
#endif
	if (strcmp(pNodeName, "tenporouter.loc") == 0) {
		return g_origgetaddrinfoo(config["General"]["NetworkAdapterIP"].c_str(), pServiceName, pHints, ppResult);
	}
	else if (strcmp(pNodeName, "bbrouter.loc") == 0) {
		return g_origgetaddrinfoo(config["General"]["NetworkAdapterIP"].c_str(), pServiceName, pHints, ppResult);
	}
	else if (strcmp(pNodeName, "naominet.jp") == 0) {
		return g_origgetaddrinfoo(config["General"]["ServerHost"].c_str(), pServiceName, pHints, ppResult);
	}
	else {
		return g_origgetaddrinfoo(pNodeName, pServiceName, pHints, ppResult);
	}
}

static int WINAPI GetRTTAndHopCountStubAM(_In_ uint32_t DestIpAddress, _Out_ PULONG HopCount, _In_ ULONG MaxHops, _Out_ PULONG RTT) {
	return 1;
}

typedef HRESULT(__stdcall* DllRegisterServerFunc)();

static void dllreg() {
	//iauthdll.dll
	HMODULE hModule = LoadLibrary(L"iauthdll.dll");
	DllRegisterServerFunc DllRegisterServer = (DllRegisterServerFunc)GetProcAddress(hModule, "DllRegisterServer");
	HRESULT hr = DllRegisterServer();
	if (SUCCEEDED(hr)) {
#ifdef DEBUG
		īnfo(true, "iauthdll.dll registered successfully!");
#endif
	}
	else {
		int msgboxID = MessageBox(
			NULL,
			(LPCWSTR)L"iauthdll.dll register failure. \n 请检查是否管理员运行，或者使用DX修复工具增强版修复运行库。",
			(LPCWSTR)L"ALL.Net Online Auth Service Error",
			MB_ICONWARNING | MB_OK
		);
		// There was an error
	}
}

static InitFunction HookAmAuthD64([]() {
	// write config files for mt6
	PrepareAMAuth();
	dllreg();
	MH_Initialize();
	MH_CreateHookApi(L"kernel32.dll", "SetSystemTime", SetSystemTimeHook, (void**)&orig_SetSystemTime);
	MH_CreateHookApi(L"ws2_32.dll", "getaddrinfo", getaddrinfoHookAMAuth, (void**)&g_origgetaddrinfoo);
	MH_CreateHookApi(L"ws2_32.dll", "htons", htonsHook, (void**)&htons_orig);
	MH_CreateHookApi(L"iphlpapi.dll", "GetRTTAndHopCount", GetRTTAndHopCountStubAM, NULL);
	MH_EnableHook(MH_ALL_HOOKS);
	}, GameID::AMAuthd);
#pragma optimize("", on)
#endif