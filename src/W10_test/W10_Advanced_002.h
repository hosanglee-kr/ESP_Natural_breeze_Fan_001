// W10_Advanced_002.h

/**
 * WiFiManager 고급 데모, 고급 설정 옵션 포함
 * TRIGGEN_PIN 버튼 누름을 구현합니다. 한 번 누르면 온디맨드 설정 포털이 실행되고, 3초간 누르면 설정이 초기화됩니다.
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <OneButton.h>   // OneButton 라이브러리 추가

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <FS.h>

#define G_W10_USEOTA
// enable OTA
#ifdef G_W10_USEOTA
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif


#define G_W10_TRIGGER_PIN 0 // 설정 포털 트리거 및 설정 초기화에 사용되는 핀
// OneButton 객체 생성
// G_W10_TRIGGER_PIN, true (풀업 저항 사용), true (내부 풀업 저항 활성화)
OneButton g_W10_button(G_W10_TRIGGER_PIN, true, true);

#define G_W10_WM_CONFIG_FILE  "/w10_wm_config_001.json"

// wifimanager는 블로킹 모드 또는 논블로킹 모드로 실행될 수 있습니다.
// 논블로킹 모드를 사용하는 경우 delay() 없이 루프를 처리하는 방법을 알아야 합니다.
bool                 g_W10_wm_nonblocking = false; // true로 변경하면 논블로킹 모드를 사용합니다.

WiFiManager          g_W10_WifiManager;            // 전역 WiFiManager 인스턴스
WiFiManagerParameter g_W10_custom_field;    // 전역 매개변수 (논블로킹 모드에서 매개변수 사용 시)


//define your default values here, if there are different values in config.json, they are overwritten.
//length should be max size + 1
char mqtt_server[40];
char mqtt_port[6] = "8080";
char api_token[34] = "YOUR_APITOKEN";
//default custom static IP
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";

//flag for saving data
bool g_W10_shouldSaveConfig = false;



void W10_saveConfigCallback() ;
void W10_startConfigPortal();
void W10_resetSettings();

String    W10_getParam(String name); // 사용자 정의 매개변수 값을 가져오는 함수
void    W10_saveParamCallback(); // 매개변수 저장 시 호출되는 콜백 함수



void W10_handlePreOtaUpdateCallback(){
  Update.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("CUSTOM Progress: %u%%\r", (progress / (total / 100)));
  });
}

//callback notifying us of the need to save config
void W10_saveConfigCallback() {
  Serial.println("Should save config");
  g_W10_shouldSaveConfig = true;
}

void W10_loadJson_config(){
	//read configuration from FS json
  Serial.println("mounting FS...");

  if (LittleFS.begin()) {
    Serial.println("mounted file system");
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File v_configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "r");
      if (v_configFile) {
        Serial.println("opened config file");
        size_t size = v_configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        v_configFile.readBytes(buf.get(), size);
 
        JsonDocument json;
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(api_token, json["api_token"]);

          if (json["ip"]) {
            Serial.println("setting custom ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            Serial.println(static_ip);
          } else {
            Serial.println("no custom ip in config");
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
   } else {
      Serial.println("failed to mount FS");
   }

	Serial.println(static_ip);
    Serial.println(api_token);
    Serial.println(mqtt_server);
}

void W10_saveJson_config(){
	//save the custom parameters to FS
  if (g_W10_shouldSaveConfig) {
    Serial.println("saving config");
    JsonDocument json;

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"]   = mqtt_port;
    json["api_token"]   = api_token;

    json["ip"]      = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"]  = WiFi.subnetMask().toString();

    File configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}


void W10_init() {

    WiFi.mode(WIFI_STA);  // 명시적으로 STA 모드로 설정합니다. ESP는 기본적으로 STA+AP 모드입니다.

    // Serial.begin(115200); // 시리얼 통신 시작 (디버깅용)
    // Serial.setDebugOutput(true); // 시리얼 디버그 출력 활성화 (디버깅용)
    // delay(3000); // 초기화 지연 (디버깅용)
    // Serial.println("\n Starting"); // 시작 메시지 출력 (디버깅용)

    // g_W10_WifiManager.resetSettings(); // 설정을 초기화합니다. (주석 처리됨: 필요 시 주석 해제)

    if (g_W10_wm_nonblocking) {
        g_W10_WifiManager.setConfigPortalBlocking(false); // 논블로킹 모드로 설정 포털을 실행합니다.
    }

	////////////////
	// The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
    WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 34);
	

	// 사용자에게 'param' 메뉴에서 MAC 주소를 확인하도록 유도하는 방법이 더 현실적입니다.
    // 읽기 전용 매개변수로 MAC 주소를 추가합니다.
    char v_macAddressStr[18]; // MAC 주소 문자열 (XX:XX:XX:XX:XX:XX) + null 종료
    String v_currentMac = WiFi.macAddress();
    v_currentMac.toCharArray(v_macAddressStr, sizeof(v_macAddressStr));

    // 이 파라미터는 "param" 메뉴에 표시됩니다.
    // info 페이지에 직접 표시하는 것은 WiFiManager의 유연성 한계로 어렵습니다.
    // 따라서 사용자에게 "매개변수" 메뉴를 확인하도록 안내해야 합니다.
    WiFiManagerParameter v_mac_param("<p><strong>디바이스 MAC 주소:</strong></p>", "MAC Address", v_macAddressStr, sizeof(v_macAddressStr), "readonly");
    g_W10_WifiManager.addParameter(&v_mac_param);

	

	////////////////////
    // 사용자 정의 입력 필드 추가
    int            customFieldLength = 40;

    // new (&g_W10_custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\""); // 일반 텍스트 입력 필드 예시 (주석 처리됨)

    // 테스트용 사용자 정의 HTML 입력 타입 (체크박스)
    // new (&g_W10_custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // 사용자 정의 HTML 타입 (주석 처리됨)

    // 테스트용 사용자 정의 HTML (라디오 버튼)
    const char* custom_radio_str  = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    new (&g_W10_custom_field) WiFiManagerParameter(custom_radio_str);     // 사용자 정의 HTML 입력 필드 추가

    g_W10_WifiManager.addParameter(&g_W10_custom_field); // WiFiManager에 사용자 정의 매개변수 추가
    g_W10_WifiManager.setSaveParamsCallback(W10_saveParamCallback); // 매개변수 저장 콜백 함수 설정

	//////////////////////
    // 배열 또는 벡터를 통한 사용자 정의 메뉴
    //
    // 메뉴 토큰: "wifi", "wifinoscan", "info", "param", "close", "sep", "erase", "restart", "exit" (sep는 구분선)
    // (param이 메뉴에 있으면 매개변수는 wifi 페이지에 표시되지 않습니다!)
    // const char* menu[] = {"wifi","info","param","sep","restart","exit"}; // 배열로 메뉴 설정 예시 (주석 처리됨)
    // g_W10_WifiManager.setMenu(menu,6);
    std::vector<const char*> menu = {"wifi", "info", "param", "sep", "restart", "exit"}; // 벡터로 메뉴 설정
    g_W10_WifiManager.setMenu(menu); // WiFiManager 메뉴 설정

    // 다크 테마 설정
    g_W10_WifiManager.setClass("invert");

	///////
	W10_loadJson_config();
	//set config save notify callback
    g_W10_WifiManager.setSaveConfigCallback(W10_saveConfigCallback);
    //set static ip
    IPAddress _ip, _gw, _sn;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _sn.fromString(static_sn);

    g_W10_WifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

    //add all your parameters here
    g_W10_WifiManager.addParameter(&custom_mqtt_server);
    g_W10_WifiManager.addParameter(&custom_mqtt_port);
    g_W10_WifiManager.addParameter(&custom_api_token);


	
    // 고정 IP 설정
    //  g_W10_WifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // 고정 IP, 게이트웨이, 서브넷 마스크 설정 (주석 처리됨)
    //  g_W10_WifiManager.setShowStaticFields(true); // 고정 IP 필드를 항상 표시하도록 강제 (주석 처리됨)
    //  g_W10_WifiManager.setShowDnsFields(true);    // DNS 필드를 항상 표시하도록 강제 (주석 처리됨)

    // g_W10_WifiManager.setConnectTimeout(20); // 연결 시도 시간 (초) 설정 (주석 처리됨)
    g_W10_WifiManager.setConfigPortalTimeout(30);    // n초 후 설정 포털 자동 닫기
    // g_W10_WifiManager.setCaptivePortalEnable(false); // 캡티브 포털 리디렉션 비활성화 (주석 처리됨)
    // g_W10_WifiManager.setAPClientCheck(true); // 소프트 AP에 클라이언트가 연결되면 타임아웃 방지 (주석 처리됨)

    // Wi-Fi 스캔 설정
    // g_W10_WifiManager.setRemoveDuplicateAPs(false); // 중복 AP 이름 제거 안 함 (true로 설정 시 제거) (주석 처리됨)
    // g_W10_WifiManager.setMinimumSignalQuality(20);  // 스캔에 표시될 최소 RSSI (백분율), null = 8% (주석 처리됨)
    // g_W10_WifiManager.setShowInfoErase(false);      // 정보 페이지에 지우기 버튼 표시 안 함 (주석 처리됨)
    // g_W10_WifiManager.setScanDispPerc(true);        // RSSI를 그래프 아이콘이 아닌 백분율로 표시 (주석 처리됨)

    // g_W10_WifiManager.setBreakAfterConfig(true);    // Wi-Fi 저장 실패 시에도 항상 설정 포털 종료 (주석 처리됨)
    g_W10_WifiManager.setPreOtaUpdateCallback(W10_handlePreOtaUpdateCallback);

    bool res;
    // res = g_W10_WifiManager.autoConnect(); // 칩 ID로부터 자동 생성된 AP 이름으로 자동 연결 (주석 처리됨)
    // res = g_W10_WifiManager.autoConnect("AutoConnectAP"); // 익명 AP로 자동 연결 (주석 처리됨)
    res = g_W10_WifiManager.autoConnect("AutoConnectAP", "password");    // 비밀번호로 보호된 AP로 자동 연결

    if (!res) {
        Serial.println("연결 실패 또는 타임아웃 발생");
        // ESP.restart(); // ESP 재시작 (필요 시 주석 해제)
    } else {
        // 여기에 도달했다면 Wi-Fi에 연결되었습니다.
        Serial.println("연결 성공 :)");
    }

	//read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(api_token, custom_api_token.getValue());
	
	W10_saveJson_config();
	
	// OneButton 콜백 함수 설정
    // 짧게 눌렀을 때 (클릭)
    g_W10_button.attachClick(W10_startConfigPortal);
    // 길게 눌렀을 때 (롱 프레스 스타트) - 여기서는 롱 프레스가 시작되는 시점에 바로 처리
    g_W10_button.attachLongPressStart(W10_resetSettings);

	#ifdef G_W10_USEOTA
        ArduinoOTA.begin();
    #endif

}

// 설정 포털을 시작하는 함수
void W10_startConfigPortal() {
    Serial.println("버튼이 클릭되었습니다. 설정 포털을 시작합니다.");
    g_W10_WifiManager.setConfigPortalTimeout(120); // 설정 포털 타임아웃을 120초로 설정

    if (!g_W10_WifiManager.startConfigPortal("OnDemandAP", "password")) {
        Serial.println("설정 포털 연결 실패 또는 타임아웃 발생");
        delay(3000);
        // ESP.restart(); // ESP 재시작 (필요 시 주석 해제)
    } else {
        Serial.println("Wi-Fi 연결 성공 :)");
    }
}
// 설정을 초기화하고 재시작하는 함수
void W10_resetSettings() {
    Serial.println("버튼이 길게 눌렸습니다. 설정을 초기화하고 재시작합니다.");
    g_W10_WifiManager.resetSettings(); // WiFiManager 설정 초기화
    ESP.restart(); // ESP 재시작
}

String W10_getParam(String name) {
    // 서버에서 매개변수 읽기 (사용자 정의 HTML 입력용)
    String value;
    if (g_W10_WifiManager.server->hasArg(name)) {
        value = g_W10_WifiManager.server->arg(name);
    }
    return value;
}


void W10_saveParamCallback() {
    Serial.println("[콜백] W10_saveParamCallback이 호출되었습니다.");
    Serial.println("매개변수 customfieldid = " + W10_getParam("customfieldid"));
}

void W10_run() {
    if (g_W10_wm_nonblocking) {
		g_W10_WifiManager.process();  // 논블로킹 모드에서 delay()를 피하고 다른 장시간 실행 코드 처리
	}
    g_W10_button.tick(); // OneButton 라이브러리의 상태 업데이트 함수 호출

	#ifdef G_W10_USEOTA
      ArduinoOTA.handle();
    #endif
}
