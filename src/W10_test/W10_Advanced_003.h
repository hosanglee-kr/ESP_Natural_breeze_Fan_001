// W10_Advanced_003.h

/**
 * WiFiManager 고급 데모, 고급 설정 옵션 포함
 * TRIGGEN_PIN 버튼 누름을 구현합니다. 한 번 누르면 온디맨드 설정 포털이 실행되고, 3초간 누르면 설정이 초기화됩니다.
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h> // JSON 데이터 직렬화/역직렬화를 위한 라이브러리
#include <LittleFS.h>    // ESP32/ESP8266의 플래시 파일 시스템 (LittleFS) 사용을 위한 라이브러리
#include <FS.h>          // 파일 시스템 기본 기능을 위한 라이브러리 (LittleFS에 포함됨)

#define G_W10_USEOTA
// OTA 활성화
#ifdef G_W10_USEOTA
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif

#define G_W10_ONDEMAND
#ifdef G_W10_ONDEMAND
    #include <OneButton.h>   // OneButton 라이브러리 추가

    #define G_W10_TRIGGER_PIN 0 // 설정 포털 트리거 및 설정 초기화에 사용되는 핀
    // OneButton 객체 생성
    // G_W10_TRIGGER_PIN, true (풀업 저항 사용), true (내부 풀업 저항 활성화)
    OneButton g_W10_button(G_W10_TRIGGER_PIN, true, true);
#endif

#define G_W10_WM_CONFIG_FILE  "/w10_wm_config_001.json"

// wifimanager는 블로킹 모드 또는 논블로킹 모드로 실행될 수 있습니다.
// 논블로킹 모드를 사용하는 경우 delay() 없이 루프를 처리하는 방법을 알아야 합니다.
bool                 g_W10_wm_nonblocking = false; // 논블로킹 모드 사용 여부 (초기값 false)

WiFiManager          g_W10_WifiManager;            // 전역 WiFiManager 인스턴스
WiFiManagerParameter g_W10_custom_field;    // 전역 매개변수 (논블로킹 모드에서 매개변수 사용 시)

// define your default values here, if there are different values in config.json, they are overwritten.
// length should be max size + 1
char mqtt_server[40];
char mqtt_port[6] = "8080";
char api_token[34] = "YOUR_APITOKEN";
// default custom static IP
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";

// flag for saving data
bool g_W10_shouldSaveConfig = false;

// 함수 프로토타입 선언
void W10_saveConfigCallback();   // WiFiManager에서 설정 저장 시 호출될 콜백 함수
void W10_startConfigPortal();    // 설정 포털을 시작하는 함수
void W10_resetSettings();        // 설정을 초기화하고 재시작하는 함수

String W10_getParam(String name); // 사용자 정의 매개변수 값을 가져오는 함수
void W10_saveParamCallback();     // 매개변수 저장 시 호출되는 콜백 함수


void W10_handlePreOtaUpdateCallback(){
    Update.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("CUSTOM Progress: %u%%\r", (progress / (total / 100)));
    });
}

// callback notifying us of the need to save config
void W10_saveConfigCallback() {
    Serial.println("Should save config");
    g_W10_shouldSaveConfig = true;
}

void W10_loadJson_config(){
    // read configuration from FS json
    Serial.println("mounting FS...");

    if (LittleFS.begin()) {
        Serial.println("mounted file system");
        // 파일 경로 일관성 유지: /config.json 대신 G_W10_WM_CONFIG_FILE 사용
        if (LittleFS.exists(G_W10_WM_CONFIG_FILE)) {
            // file exists, reading and loading
            Serial.println("reading config file");
            File v_configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "r");
            if (v_configFile) {
                Serial.println("opened config file");
                size_t size = v_configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size + 1]); // +1 for null terminator
                buf.get()[size] = '\0'; // ensure null termination
                v_configFile.readBytes(buf.get(), size);
                v_configFile.close();
 
                JsonDocument json;
                // deserializeJson 함수는 const char* 또는 String을 인자로 받습니다.
                auto deserializeError = deserializeJson(json, buf.get());
                serializeJson(json, Serial); // 디버깅용
                Serial.println(); // 개행

                if ( ! deserializeError ) {
                    Serial.println("\nparsed json");

                    // JSON에서 값 로드, 기본값 설정 추가 (안정성 향상)
                    strcpy(mqtt_server, json["mqtt_server"] | "");
                    strcpy(mqtt_port, json["mqtt_port"] | "8080");
                    strcpy(api_token, json["api_token"] | "YOUR_APITOKEN");

                    // IP 설정 로드
                    if (json["ip"].as<String>().length() > 0) { // IP가 유효한지 확인
                        Serial.println("setting custom ip from config");
                        strcpy(static_ip, json["ip"] | "");
                        strcpy(static_gw, json["gateway"] | "");
                        strcpy(static_sn, json["subnet"] | "");
                        Serial.println(static_ip);
                    } else {
                        Serial.println("no custom ip in config");
                    }
                    
                    // 논블로킹 모드 설정 로드
                    g_W10_wm_nonblocking = json["wm_nonblocking"] | false; // 기본값 false
                    Serial.print("논블로킹 모드 설정: ");
                    Serial.println(g_W10_wm_nonblocking ? "활성화" : "비활성화");
                
                } else {
                    Serial.println("failed to load json config: parsing error");
                    Serial.println(deserializeError.c_str()); // 파싱 오류 메시지 출력
                }
            } else {
                Serial.println("failed to open config file");
            }
        } else {
            Serial.println("config file does not exist");
        }
    } else {
        Serial.println("failed to mount FS");
    }

    Serial.println("--- 로드된 설정 ---");
    Serial.println("Static IP: " + String(static_ip));
    Serial.println("API Token: " + String(api_token));
    Serial.println("MQTT Server: " + String(mqtt_server));
    Serial.println("논블로킹 모드: " + String(g_W10_wm_nonblocking ? "활성화" : "비활성화"));
    Serial.println("--------------------");
}

void W10_saveJson_config(){
    // save the custom parameters to FS
    if (g_W10_shouldSaveConfig) {
        Serial.println("saving config");
        JsonDocument json;

        json["mqtt_server"] = mqtt_server;
        json["mqtt_port"]   = mqtt_port;
        json["api_token"]   = api_token;

        // 현재 연결된 IP 정보 저장 (고정 IP 사용 여부와 무관하게 저장)
        json["ip"]      = WiFi.localIP().toString();
        json["gateway"] = WiFi.gatewayIP().toString();
        json["subnet"]  = WiFi.subnetMask().toString();

        // 논블로킹 모드 설정 저장
        json["wm_nonblocking"] = g_W10_wm_nonblocking;
        
        File configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "w");
        if (!configFile) {
            Serial.println("failed to open config file for writing");
        }

        serializeJson(json, Serial); // 디버깅용
        Serial.println(); // 개행
        serializeJson(json, configFile);

        configFile.close();
        Serial.println("config saved.");
        g_W10_shouldSaveConfig = false; // 저장 후 플래그 초기화
    }
    Serial.println("--- 현재 네트워크 정보 ---");
    Serial.println("로컬 IP: " + WiFi.localIP().toString());
    Serial.println("게이트웨이 IP: " + WiFi.gatewayIP().toString());
    Serial.println("서브넷 마스크: " + WiFi.subnetMask().toString());
    Serial.println("-----------------------");
}


void W10_init() {

    WiFi.mode(WIFI_STA);  // 명시적으로 STA 모드로 설정합니다. ESP는 기본적으로 STA+AP 모드입니다.

    // 1. 설정 로드 및 초기 WiFiManager 모드 설정
    W10_loadJson_config();
    g_W10_WifiManager.setConfigPortalBlocking(!g_W10_wm_nonblocking); // 논블로킹 모드 설정 적용

    // 2. WiFiManager 콜백 함수 설정
    g_W10_WifiManager.setSaveConfigCallback(W10_saveConfigCallback);
    g_W10_WifiManager.setSaveParamsCallback(W10_saveParamCallback); // 매개변수 저장 콜백 함수 설정
    g_W10_WifiManager.setPreOtaUpdateCallback(W10_handlePreOtaUpdateCallback);

    // 3. WiFiManager 매개변수 정의 및 추가
    // MQTT 서버, 포트, API 토큰
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
    WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 34);

    // 논블로킹 모드 설정을 위한 체크박스 매개변수
    char checkbox_checked[10];
    if (g_W10_wm_nonblocking) strcpy(checkbox_checked, "checked"); else strcpy(checkbox_checked, "");
    char checkbox_html[100];
    sprintf(checkbox_html, "<br/><input type='checkbox' name='wm_nonblocking' value='true' %s> 논블로킹 모드 사용", checkbox_checked);
    WiFiManagerParameter v_wmp_nonblocking_checkbox(checkbox_html);

    // MAC 주소를 위한 읽기 전용 매개변수 (Param 메뉴에서 확인 가능)
    char v_macAddressStr[18]; // MAC 주소 문자열 (XX:XX:XX:XX:XX:XX) + null 종료
    String v_currentMac = WiFi.macAddress();
    v_currentMac.toCharArray(v_macAddressStr, sizeof(v_macAddressStr));
    WiFiManagerParameter v_mac_param("<p><strong>디바이스 MAC 주소:</strong></p>", "MAC Address", v_macAddressStr, sizeof(v_macAddressStr), "readonly");
    
    // 추가적인 사용자 정의 입력 필드 (예: 라디오 버튼)
    const char* custom_radio_str  = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    new (&g_W10_custom_field) WiFiManagerParameter(custom_radio_str); // 사용자 정의 HTML 입력 필드 추가

    // 모든 매개변수를 WiFiManager에 추가
    g_W10_WifiManager.addParameter(&custom_mqtt_server);
    g_W10_WifiManager.addParameter(&custom_mqtt_port);
    g_W10_WifiManager.addParameter(&custom_api_token);
    g_W10_WifiManager.addParameter(&v_wmp_nonblocking_checkbox); // 논블로킹 체크박스 추가
    g_W10_WifiManager.addParameter(&v_mac_param); // MAC 주소 파라미터 추가
    g_W10_WifiManager.addParameter(&g_W10_custom_field); // 사용자 정의 라디오 버튼 추가

    // 4. WiFiManager 고급 설정
    // 고정 IP 설정
    IPAddress _ip, _gw, _sn;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _sn.fromString(static_sn);
    g_W10_WifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

    // 메뉴 설정
    std::vector<const char*> menu = {"wifi", "info", "param", "sep", "restart", "exit"};
    g_W10_WifiManager.setMenu(menu);

    // 다크 테마 설정
    g_W10_WifiManager.setClass("invert");

    g_W10_WifiManager.setConfigPortalTimeout(30);    // n초 후 설정 포털 자동 닫기

    // 5. Wi-Fi 연결 시도
    bool res;
    res = g_W10_WifiManager.autoConnect("AutoConnectAP", "password");    // 비밀번호로 보호된 AP로 자동 연결

    if (!res) {
        Serial.println("연결 실패 또는 타임아웃 발생");
        // ESP.restart(); // 필요 시 주석 해제
    } else {
        Serial.println("연결 성공 :)");
    }

    // 6. 웹 UI에서 업데이트된 매개변수 값들을 전역 변수에 복사 및 저장
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(api_token, custom_api_token.getValue());
    
    // 논블로킹 체크박스 값 반영
    if (g_W10_WifiManager.server->hasArg("wm_nonblocking") && g_W10_WifiManager.server->arg("wm_nonblocking").equalsIgnoreCase("true")) {
        g_W10_wm_nonblocking = true;
    } else {
        g_W10_wm_nonblocking = false;
    }
    // 설정 포털 블로킹 모드 재설정 (업데이트된 값으로)
    g_W10_WifiManager.setConfigPortalBlocking(!g_W10_wm_nonblocking);

    W10_saveJson_config(); // 변경된 설정 저장

    // 7. 온디맨드 및 OTA 설정 (조건부 컴파일)
    #ifdef G_W10_ONDEMAND
        // OneButton 콜백 함수 설정
        g_W10_button.attachClick(W10_startConfigPortal);        // 짧게 눌렀을 때 (클릭)
        g_W10_button.attachLongPressStart(W10_resetSettings);   // 길게 눌렀을 때 (롱 프레스 스타트)
    #endif
    
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
        // ESP.restart(); // 필요 시 주석 해제
    } else {
        Serial.println("Wi-Fi 연결 성공 :)");
        // 설정 포털을 통해 연결 성공 시, 변경된 설정이 있을 수 있으므로 저장 플래그 설정
        g_W10_shouldSaveConfig = true; // 변경된 설정이 있을 수 있으므로 저장 플래그 설정
        W10_saveJson_config(); // 현재 상태를 JSON에 저장
    }
}
// 설정을 초기화하고 재시작하는 함수
void W10_resetSettings() {
    Serial.println("버튼이 길게 눌렸습니다. 설정을 초기화하고 재시작합니다.");
    g_W10_WifiManager.resetSettings(); // WiFiManager 설정 초기화
    // config.json 파일도 삭제하여 완전 초기화
    if (LittleFS.exists(G_W10_WM_CONFIG_FILE)) {
        LittleFS.remove(G_W10_WM_CONFIG_FILE);
        Serial.println("config.json 파일 삭제 완료.");
    }
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
    // 이 콜백은 사용자 정의 매개변수가 저장될 때 호출됩니다.
    // g_W10_shouldSaveConfig를 true로 설정하여 W10_saveJson_config()가 실행되도록 합니다.
    g_W10_shouldSaveConfig = true;
}

void W10_run() {
    if (g_W10_wm_nonblocking) {
        g_W10_WifiManager.process();  // 논블로킹 모드에서 delay()를 피하고 다른 장시간 실행 코드 처리
    }
    
    #ifdef G_W10_ONDEMAND
        g_W10_button.tick(); // OneButton 라이브러리의 상태 업데이트 함수 호출
    #endif
    
    #ifdef G_W10_USEOTA
        ArduinoOTA.handle();
    #endif
}
