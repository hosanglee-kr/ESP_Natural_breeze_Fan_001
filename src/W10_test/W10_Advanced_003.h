// W10_Advanced_003.h

/**
 * WiFiManager 고급 데모, 고급 설정 옵션 포함
 * TRIGGEN_PIN 버튼 누름을 구현합니다. 한 번 누르면 온디맨드 설정 포털이 실행되고, 3초간 누르면 설정이 초기화됩니다.
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h> // JSON 데이터 직렬화/역직렬화를 위한 라이브러리
#include <LittleFS.h>    // ESP32/ESP8266의 플래시 파일 시스템 (LittleFS) 사용을 위한 라이브러리
#include <FS.h>          // 파일 시스템 기본 기능을 위한 라이브러리 (LittleFS에 포함됨)

// --- 매크로 정의 ---
#define G_W10_USEOTA
#ifdef G_W10_USEOTA
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif

#define G_W10_ONDEMAND
#ifdef G_W10_ONDEMAND
    #include <OneButton.h>   // OneButton 라이브러리 추가

    #define G_W10_TRIGGER_PIN 0 // 설정 포털 트리거 및 설정 초기화에 사용되는 핀
    // OneButton 객체 생성: 핀 번호, 풀업 저항 사용 여부 (true), 내부 풀업 저항 활성화 여부 (true)
    OneButton g_W10_button(G_W10_TRIGGER_PIN, true, true);
#endif

#define G_W10_WM_CONFIG_FILE  "/w10_wm_config_001.json" // WiFiManager 설정 파일 경로

// --- 전역 변수 선언 ---
// wifimanager는 블로킹 모드 또는 논블로킹 모드로 실행될 수 있습니다.
// 논블로킹 모드를 사용하는 경우 delay() 없이 루프를 처리하는 방법을 알아야 합니다.
bool                 g_W10_isWmNonBlocking = false; // 논블로킹 모드 사용 여부 (초기값 false)

WiFiManager          g_W10_wifiManager;            // 전역 WiFiManager 인스턴스
WiFiManagerParameter g_W10_customField;            // 전역 매개변수 (논블로킹 모드에서 매개변수 사용 시)

// 사용자 정의 설정 값들을 위한 버퍼 및 기본값 정의
char g_W10_mqttServer[40]   = "";        // MQTT 서버 주소
char g_W10_mqttPort[6]      = "8080";    // MQTT 포트
char g_W10_apiToken[34]     = "YOUR_APITOKEN"; // API 토큰
char g_W10_staticIp[16]     = "10.0.1.56"; // 고정 IP 주소
char g_W10_staticGateway[16] = "10.0.1.1";  // 고정 게이트웨이 주소
char g_W10_staticSubnet[16] = "255.255.255.0"; // 고정 서브넷 마스크

// 데이터 저장 필요 여부를 나타내는 플래그
bool g_W10_shouldSaveConfig = false;

// --- 함수 프로토타입 선언 ---
void W10_saveConfigCallback();   // WiFiManager에서 설정 저장 시 호출될 콜백 함수
void W10_startConfigPortal();    // 설정 포털을 시작하는 함수
void W10_resetSettings();        // 설정을 초기화하고 재시작하는 함수

String W10_getParam(String paramName); // 사용자 정의 매개변수 값을 가져오는 함수
void W10_saveParamCallback();     // 매개변수 저장 시 호출되는 콜백 함수
void W10_handlePreOtaUpdateCallback(); // OTA 업데이트 진행률 콜백 함수

// --- WiFiManager 설정 저장 알림 콜백 함수 ---
void W10_saveConfigCallback() {
    Serial.println("Should save config");
    g_W10_shouldSaveConfig = true;
}

// --- JSON 설정 파일 로드 함수 ---
void W10_loadJsonConfig(){
    Serial.println("mounting FS...");

    if (LittleFS.begin()) {
        Serial.println("mounted file system");
        // 설정 파일 존재 여부 확인 (매크로 경로 사용)
        if (LittleFS.exists(G_W10_WM_CONFIG_FILE)) {
            Serial.println("reading config file");
            File v_configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "r");
            if (v_configFile) {
                Serial.println("opened config file");
                size_t v_fileSize = v_configFile.size();
                // 버퍼에 파일 내용 로드 (널 종료 문자 공간 확보)
                std::unique_ptr<char[]> v_buffer(new char[v_fileSize + 1]);
                v_configFile.readBytes(v_buffer.get(), v_fileSize);
                v_buffer.get()[v_fileSize] = '\0'; // 널 종료 문자 추가
                v_configFile.close();
 
                JsonDocument v_jsonDoc;
                // JSON 파싱 시도
                auto v_deserializeError = deserializeJson(v_jsonDoc, v_buffer.get());
                serializeJson(v_jsonDoc, Serial); // 디버깅용: 파싱된 JSON 내용을 시리얼 모니터에 출력
                Serial.println(); // 개행

                if ( ! v_deserializeError ) {
                    Serial.println("\nparsed json");

                    // JSON에서 설정 값들을 로드하고, 값이 없을 경우 기본값 사용
                    // (json["key"] | "default_value") 구문은 ArduinoJson 6 이상에서 사용 가능
                    strcpy(g_W10_mqttServer, v_jsonDoc["mqtt_server"] | "");
                    strcpy(g_W10_mqttPort, v_jsonDoc["mqtt_port"] | "8080");
                    strcpy(g_W10_apiToken, v_jsonDoc["api_token"] | "YOUR_APITOKEN");

                    // IP 설정 로드 (IP 주소가 유효한지 확인)
                    if (v_jsonDoc["ip"].as<String>().length() > 0) {
                        Serial.println("setting custom ip from config");
                        strcpy(g_W10_staticIp, v_jsonDoc["ip"] | "");
                        strcpy(g_W10_staticGateway, v_jsonDoc["gateway"] | "");
                        strcpy(g_W10_staticSubnet, v_jsonDoc["subnet"] | "");
                        Serial.println(g_W10_staticIp);
                    } else {
                        Serial.println("no custom ip in config");
                    }
                    
                    // 논블로킹 모드 설정 로드
                    g_W10_isWmNonBlocking = v_jsonDoc["wm_nonblocking"] | false;
                    Serial.print("논블로킹 모드 설정: ");
                    Serial.println(g_W10_isWmNonBlocking ? "활성화" : "비활성화");
                
                } else {
                    Serial.print("failed to load json config: parsing error -> ");
                    Serial.println(v_deserializeError.c_str()); // 파싱 오류 메시지 출력
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

    Serial.println("--- 로드된 설정 요약 ---");
    Serial.println("Static IP: " + String(g_W10_staticIp));
    Serial.println("API Token: " + String(g_W10_apiToken));
    Serial.println("MQTT Server: " + String(g_W10_mqttServer));
    Serial.println("논블로킹 모드: " + String(g_W10_isWmNonBlocking ? "활성화" : "비활성화"));
    Serial.println("--------------------");
}

// --- JSON 설정 파일 저장 함수 ---
void W10_saveJsonConfig(){
    if (g_W10_shouldSaveConfig) {
        Serial.println("saving config");
        JsonDocument v_jsonDoc;

        v_jsonDoc["mqtt_server"] = g_W10_mqttServer;
        v_jsonDoc["mqtt_port"]   = g_W10_mqttPort;
        v_jsonDoc["api_token"]   = g_W10_apiToken;

        // 현재 장치에 할당된 IP 정보 저장 (설정 포털 사용 후 갱신될 수 있음)
        v_jsonDoc["ip"]      = WiFi.localIP().toString();
        v_jsonDoc["gateway"] = WiFi.gatewayIP().toString();
        v_jsonDoc["subnet"]  = WiFi.subnetMask().toString();

        // 논블로킹 모드 설정 저장
        v_jsonDoc["wm_nonblocking"] = g_W10_isWmNonBlocking;
        
        File v_configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "w");
        if (!v_configFile) {
            Serial.println("failed to open config file for writing");
        }

        serializeJson(v_jsonDoc, Serial); // 디버깅용: 저장될 JSON 내용을 시리얼 모니터에 출력
        Serial.println(); // 개행
        serializeJson(v_jsonDoc, v_configFile); // 파일에 JSON 내용 쓰기

        v_configFile.close();
        Serial.println("config saved successfully.");
        g_W10_shouldSaveConfig = false; // 저장 완료 후 플래그 초기화
    }
    Serial.println("--- 현재 네트워크 정보 ---");
    Serial.println("로컬 IP: " + WiFi.localIP().toString());
    Serial.println("게이트웨이 IP: " + WiFi.gatewayIP().toString());
    Serial.println("서브넷 마스크: " + WiFi.subnetMask().toString());
    Serial.println("-----------------------");
}

// --- 초기화 함수 ---
void W10_init() {
    // 1. Wi-Fi 모드 설정 및 초기 설정 로드
    WiFi.mode(WIFI_STA);  // 명시적으로 STA(Station) 모드로 설정합니다.
    W10_loadJsonConfig(); // 저장된 JSON 설정 파일 로드
    // 로드된 논블로킹 모드 설정에 따라 WiFiManager의 블로킹 모드 설정
    g_W10_wifiManager.setConfigPortalBlocking(!g_W10_isWmNonBlocking);

    // 2. WiFiManager 콜백 함수 등록
    g_W10_wifiManager.setSaveConfigCallback(W10_saveConfigCallback); // Wi-Fi 설정 저장 시 호출
    g_W10_wifiManager.setSaveParamsCallback(W10_saveParamCallback); // 사용자 정의 매개변수 저장 시 호출
    g_W10_wifiManager.setPreOtaUpdateCallback(W10_handlePreOtaUpdateCallback); // OTA 전 호출

    // 3. WiFiManager 사용자 정의 매개변수 정의
    // MQTT 서버, 포트, API 토큰 입력 필드
    WiFiManagerParameter v_customMqttServer("server", "mqtt server", g_W10_mqttServer, 40);
    WiFiManagerParameter v_customMqttPort("port", "mqtt port", g_W10_mqttPort, 5);
    WiFiManagerParameter v_customApiToken("apikey", "API token", g_W10_apiToken, 34);

    // 논블로킹 모드 활성화/비활성화 체크박스
    char v_checkboxChecked[10]; // "checked" 또는 "" 저장
    if (g_W10_isWmNonBlocking) {
        strcpy(v_checkboxChecked, "checked");
    } else {
        strcpy(v_checkboxChecked, "");
    }
    // HTML 형태의 체크박스 매개변수
    char v_checkboxHtml[100];
    sprintf(v_checkboxHtml, "<br/><input type='checkbox' name='wm_nonblocking' value='true' %s> 논블로킹 모드 사용", v_checkboxChecked);
    WiFiManagerParameter v_wmNonBlockingCheckbox(v_checkboxHtml);

    // 디바이스 MAC 주소 표시 (읽기 전용)
    char v_macAddressStr[18]; // MAC 주소 문자열 (XX:XX:XX:XX:XX:XX) + null 종료
    String v_currentMac = WiFi.macAddress();
    v_currentMac.toCharArray(v_macAddressStr, sizeof(v_macAddressStr));
    // 이 파라미터는 "param" 메뉴에 표시됩니다.
    WiFiManagerParameter v_macParam("<p><strong>디바이스 MAC 주소:</strong></p>", "MAC Address", v_macAddressStr, sizeof(v_macAddressStr), "readonly");
    
    // 테스트용 사용자 정의 HTML 입력 (예: 라디오 버튼)
    const char* v_customRadioStr  = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    new (&g_W10_customField) WiFiManagerParameter(v_customRadioStr);

    // 4. 모든 매개변수를 WiFiManager에 추가
    g_W10_wifiManager.addParameter(&v_customMqttServer);
    g_W10_wifiManager.addParameter(&v_customMqttPort);
    g_W10_wifiManager.addParameter(&v_customApiToken);
    g_W10_wifiManager.addParameter(&v_wmNonBlockingCheckbox); // 논블로킹 체크박스 추가
    g_W10_wifiManager.addParameter(&v_macParam);               // MAC 주소 파라미터 추가
    g_W10_wifiManager.addParameter(&g_W10_customField);        // 사용자 정의 라디오 버튼 추가

    // 5. WiFiManager의 고급 동작 설정
    // 고정 IP 주소, 게이트웨이, 서브넷 마스크 설정
    IPAddress _ip, _gw, _sn;
    _ip.fromString(g_W10_staticIp);
    _gw.fromString(g_W10_staticGateway);
    _sn.fromString(g_W10_staticSubnet);
    g_W10_wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

    // 설정 포털 메뉴 항목 설정
    std::vector<const char*> v_menu = {"wifi", "info", "param", "sep", "restart", "exit"};
    g_W10_wifiManager.setMenu(v_menu);

    // 설정 포털 테마를 다크 모드로 설정
    g_W10_wifiManager.setClass("invert");

    // 설정 포털이 자동으로 닫히는 시간 (초) 설정
    g_W10_wifiManager.setConfigPortalTimeout(30);

    // 6. Wi-Fi 네트워크에 자동 연결 시도
    bool v_connectResult;
    // "AutoConnectAP"라는 SSID와 "password"로 보호된 AP로 연결 시도
    v_connectResult = g_W10_wifiManager.autoConnect("AutoConnectAP", "password");

    if (!v_connectResult) {
        Serial.println("Wi-Fi 연결 실패 또는 타임아웃 발생");
        // ESP.restart(); // 필요시 주석 해제하여 연결 실패시 장치 재시작
    } else {
        Serial.println("Wi-Fi 연결 성공 :)");
    }

    // 7. 웹 UI에서 업데이트된 매개변수 값들을 전역 변수에 반영하고 설정 저장
    strcpy(g_W10_mqttServer, v_customMqttServer.getValue());
    strcpy(g_W10_mqttPort, v_customMqttPort.getValue());
    strcpy(g_W10_apiToken, v_customApiToken.getValue());
    
    // 웹 UI의 논블로킹 체크박스 상태를 전역 변수에 반영
    if (g_W10_wifiManager.server->hasArg("wm_nonblocking") && g_W10_wifiManager.server->arg("wm_nonblocking").equalsIgnoreCase("true")) {
        g_W10_isWmNonBlocking = true;
    } else {
        g_W10_isWmNonBlocking = false;
    }
    // 업데이트된 논블로킹 모드 설정으로 WiFiManager의 블로킹 모드 재설정
    g_W10_wifiManager.setConfigPortalBlocking(!g_W10_isWmNonBlocking);

    W10_saveJsonConfig(); // 모든 변경된 설정들을 JSON 파일에 저장

    // 8. 온디맨드 설정 포털 및 OTA 업데이트 기능 초기화 (조건부 컴파일)
    #ifdef G_W10_ONDEMAND
        // OneButton 라이브러리 콜백 함수 설정
        g_W10_button.attachClick(W10_startConfigPortal);        // 버튼 짧게 눌렀을 때 설정 포털 시작
        g_W10_button.attachLongPressStart(W10_resetSettings);   // 버튼 길게 눌렀을 때 설정 초기화
    #endif
    
    #ifdef G_W10_USEOTA
        ArduinoOTA.begin(); // OTA 업데이트 기능 시작
    #endif
}

// --- 설정 포털을 시작하는 함수 (버튼 클릭 시 호출) ---
void W10_startConfigPortal() {
    Serial.println("버튼이 클릭되었습니다. 설정 포털을 시작합니다.");
    g_W10_wifiManager.setConfigPortalTimeout(120); // 설정 포털 타임아웃을 120초로 설정

    // 설정 포털 시작 및 결과 확인
    if (!g_W10_wifiManager.startConfigPortal("OnDemandAP", "password")) {
        Serial.println("설정 포털 연결 실패 또는 타임아웃 발생");
        delay(3000);
        // ESP.restart(); // 필요시 주석 해제
    } else {
        Serial.println("Wi-Fi 연결 성공 :)");
        // 설정 포털을 통해 Wi-Fi 정보가 변경되었을 수 있으므로 저장 플래그 설정 및 저장
        g_W10_shouldSaveConfig = true;
        W10_saveJsonConfig();
    }
}

// --- 설정을 초기화하고 재시작하는 함수 (버튼 길게 누름 시 호출) ---
void W10_resetSettings() {
    Serial.println("버튼이 길게 눌렸습니다. 설정을 초기화하고 재시작합니다.");
    g_W10_wifiManager.resetSettings(); // WiFiManager의 저장된 Wi-Fi 자격 증명 초기화
    // LittleFS에 저장된 커스텀 설정 파일도 삭제하여 완전 초기화
    if (LittleFS.exists(G_W10_WM_CONFIG_FILE)) {
        LittleFS.remove(G_W10_WM_CONFIG_FILE);
        Serial.println("config.json 파일 삭제 완료.");
    }
    ESP.restart(); // 장치 재시작
}

// --- 웹 서버에서 매개변수 값을 가져오는 함수 ---
String W10_getParam(String paramName) {
    String v_value;
    if (g_W10_wifiManager.server->hasArg(paramName)) {
        v_value = g_W10_wifiManager.server->arg(paramName);
    }
    return v_value;
}

// --- 사용자 정의 매개변수 저장 시 호출되는 콜백 함수 ---
void W10_saveParamCallback() {
    Serial.println("[콜백] W10_saveParamCallback이 호출되었습니다.");
    Serial.println("매개변수 customfieldid = " + W10_getParam("customfieldid"));
    // 사용자 정의 매개변수가 변경되었으므로 설정을 저장해야 함을 알림
    g_W10_shouldSaveConfig = true;
}

// --- 메인 루프에서 지속적으로 실행될 함수 ---
void W10_run() {
    // 논블로킹 모드 활성화 시 WiFiManager의 백그라운드 작업을 처리
    if (g_W10_isWmNonBlocking) {
        g_W10_wifiManager.process();
    }
    
    // 온디맨드 기능 활성화 시 버튼 상태 업데이트
    #ifdef G_W10_ONDEMAND
        g_W10_button.tick();
    #endif
    
    // OTA 업데이트 기능 활성화 시 OTA 이벤트 처리
    #ifdef G_W10_USEOTA
        ArduinoOTA.handle();
    #endif
}
