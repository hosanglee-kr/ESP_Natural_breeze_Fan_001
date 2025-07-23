#pragma once

// W10_Advanced_006.h

#ifndef W10_ADVANCED_006_H
#define W10_ADVANCED_006_H


// --- 라이브러리 인클루드 ---
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>      // JSON 데이터 직렬화/역직렬화를 위한 라이브러리
#include <LittleFS.h>         // ESP32/ESP8266의 플래시 파일 시스템 (LittleFS) 사용을 위한 라이브러리
#include <FS.h>               // 파일 시스템 기본 기능을 위한 라이브러리 (LittleFS에 포함됨)
#include <FastLED.h>          // WS2812B LED 제어 라이브러리
#include <vector>             // std::vector 사용을 위한 헤더 추가

// --- 매크로 정의 ---
#define G_W10_OTA_ENABLE          // OTA 업데이트 기능 활성화
#ifdef G_W10_OTA_ENABLE
    #include <WiFiUdp.h>
    #include <ArduinoOTA.h>
#endif

#define G_W10_ONDEMAND_ENABLE     // 온디맨드 설정 포털 및 초기화 버튼 기능 활성화
#ifdef G_W10_ONDEMAND_ENABLE
    #include <OneButton.h>    // OneButton 라이브러리 추가
    #define G_W10_TRIGGER_PIN 0 // 설정 포털 트리거 및 설정 초기화에 사용되는 핀
#endif

#define G_W10_WM_CONFIG_FILE  "/w10_wm_config2_002.json" // WiFiManager 설정 파일 경로

// --- WS2812B LED 설정 ---
#define G_W10_LED_PIN         27    // WS2812B 데이터 핀 번호
#define G_W10_NUM_LEDS        1     // 사용할 WS2812B LED 개수 (단일 LED 사용)

// --- WS2812B LED 상태 정의 ---
enum LedStatus {
    LED_STATUS_OFF,                 // 꺼짐 (Black)
    LED_STATUS_INIT,                // 초기화 중 (파란색 깜빡임, 1초 간격)
    LED_STATUS_WIFI_CONNECTING,     // Wi-Fi 연결 시도 중 (파란색 깜빡임, 1초 간격)
    LED_STATUS_WIFI_CONNECTED,      // Wi-Fi 연결 성공 (녹색 고정)
    LED_STATUS_WIFI_DISCONNECTED,   // Wi-Fi 연결 끊김 (빨간색 빠르게 깜빡임, 0.2초 간격)    
    LED_STATUS_MQTT_CONNECTED,      // MQTT 연결 성공 (노란색 고정)
    LED_STATUS_MQTT_DISCONNECTED,   // MQTT 연결 실패 (주황색 깜빡임, 0.5초 간격)
    LED_STATUS_OTA_START,           // OTA 시작 (흰색 고정)
    LED_STATUS_OTA_PROGRESS,        // OTA 진행 중 (흰색 깜빡임, 0.2초 간격)
    LED_STATUS_OTA_END,             // OTA 완료 (녹색 고정)
    LED_STATUS_OTA_ERROR,           // OTA 오류 (빨간색 고정)
    LED_STATUS_CONFIG_PORTAL        // 설정 포털 활성화 (보라색 깜빡임, 0.3초 간격)
};

// --- 설정 구조체 정의 ---
// 모든 애플리케이션 관련 설정은 이 구조체에 포함됩니다.
struct AppConfig {
    char mqttServer[40] = "";               // 기본값을 빈 문자열로 초기화
    int mqttPort = 1883;                    // 기본 MQTT 포트 1883으로 초기화
    char apiToken[34] = "YOUR_APITOKEN";    // 기본 API 토큰으로 초기화

    bool use_custom_ap_ip = false;          // 기본적으로 사용자 정의 AP IP 사용 안 함
    char ap_Ip[16] = "10.0.1.1";            // 기본 AP IP 주소
    char ap_Gateway[16] = "10.0.1.1";       // 기본 AP 게이트웨이 주소
    char ap_Subnet[16] = "255.255.255.0";   // 기본 AP 서브넷 마스크
    
    // Wi-Fi STA 모드 관련 설정 (JSON 필드명과 일치하도록 변경)
    char wifiSsid[32] = "";                 // 기본값을 빈 문자열로 초기화
    char wifiPassword[64] = "";             // 기본값을 빈 문자열로 초기화
    bool wifiUseDhcp = true;                // 기본값은 DHCP 사용
    char wifiIp[16] = "";                   // 기본값을 빈 문자열로 초기화 (DHCP 사용 시 채워짐)
    char wifiGateway[16] = "";              // 기본값을 빈 문자열로 초기화 (DHCP 사용 시 채워짐)
    char wifiSubnet[16] = "";               // 기본값을 빈 문자열로 초기화 (DHCP 사용 시 채워짐)
    char wifiDns[16] = "";

    bool isWmNonBlocking = false; // WiFiManager 논블로킹 모드 사용 여부
};

// --- 전역 변수 정의 ---
AppConfig             g_W10_appConfig;                                // 애플리케이션 설정 구조체 인스턴스
// g_W10_isWmNonBlocking 변수는 g_W10_appConfig.isWmNonBlocking과 중복되므로 제거합니다.
WiFiManager           g_W10_wifiManager;                              // 전역 WiFiManager 인스턴스
// g_W10_customField 전역 변수는 W10_init()에서 지역 변수로 대체됩니다.
bool                  g_W10_shouldSaveConfig  = false;              // 데이터 저장 필요 여부를 나타내는 플래그
bool                  g_W10_shouldReconnectWifi = false;              // Wi-Fi 재연결 필요 플래그
unsigned long         g_W10_lastReconnectAttempt= 0;                  // 마지막 재연결 시도 시간
const long            G_W10_RECONNECT_INTERVAL_MS = 5000;             // 재연결 시도 간격 (5초)

const char*           g_W10_apName              = "AutoConnectAP";
const char*           g_W10_apPassword          = NULL;

CRGB                  g_W10_leds[G_W10_NUM_LEDS];                     // WS2812B LED 배열 정의

LedStatus             g_W10_currentLedStatus  = LED_STATUS_OFF;     // 현재 LED 상태
unsigned long         g_W10_lastLedUpdateTime = 0;                  // 마지막 LED 업데이트 시간

bool                  g_W10_mqttConnected     = false;              // 임시 MQTT 연결 상태 (실제 MQTT 클라이언트 통합 시 제거 또는 연동)

#ifdef G_W10_ONDEMAND_ENABLE
    OneButton g_W10_button(G_W10_TRIGGER_PIN, true, true);
#endif

// --- 함수 프로토타입 선언 ---
void W10_clearLeds();
void W10_setLedStatus(LedStatus status);
void W10_updateLedStatus();

void W10_saveConfigCallback();
void W10_loadJsonConfig();
void W10_saveJsonConfig();

void W10_handleWiFiEvent(arduino_event_id_t event);

void W10_init();
void W10_startConfigPortal();
void W10_resetSettings();

String W10_getParam(String paramName);
void W10_saveParamCallback();
void W10_handlePreOtaUpdateCallback();

// --- 함수 정의 ---

// LED 제어 함수
void W10_clearLeds() {
    FastLED.clear();
    FastLED.show();
}

// LED 상태를 설정하는 함수
// 상태 변경이 있을 때만 LED를 초기화하고 초기 색상을 설정합니다.
// 깜빡임 로직은 W10_updateLedStatus()에서 담당합니다.
void W10_setLedStatus(LedStatus status) {
    if (g_W10_currentLedStatus != status) { // 현재 상태와 다른 경우에만 업데이트
        g_W10_currentLedStatus = status;
        g_W10_lastLedUpdateTime = millis(); // 상태 변경 시 시간 업데이트
        W10_clearLeds(); // 새로운 상태 적용 전 LED 초기화

        // 고정 색상 상태는 여기서 바로 색상을 설정합니다.
        // 깜빡이는 상태는 W10_updateLedStatus에서 제어하므로, 여기서는 초기 색상을 Black으로 둡니다.
        switch (g_W10_currentLedStatus) {
            case LED_STATUS_WIFI_CONNECTED:
            case LED_STATUS_OTA_END:
                g_W10_leds[0] = CRGB::Green;
                break;
            case LED_STATUS_MQTT_CONNECTED:
                g_W10_leds[0] = CRGB::Yellow;
                break;
            case LED_STATUS_OTA_START:
                g_W10_leds[0] = CRGB::White; // OTA 시작은 고정 흰색
                break;
            case LED_STATUS_OTA_ERROR:
            case LED_STATUS_WIFI_DISCONNECTED:
                g_W10_leds[0] = CRGB::Red;
                break;
            case LED_STATUS_OFF:
                g_W10_leds[0] = CRGB::Black; // LED 끄기
                break;
            default: // 깜빡이는 상태 (INIT, WIFI_CONNECTING, MQTT_DISCONNECTED, OTA_PROGRESS, CONFIG_PORTAL)
                g_W10_leds[0] = CRGB::Black; // 깜빡임 시작 시에는 일단 꺼진 상태로 시작
                break;
        }
        FastLED.show();
    }
}

// 메인 루프에서 주기적으로 호출하여 LED 패턴 업데이트 (주로 깜빡임 처리)
void W10_updateLedStatus() {
    unsigned long v_currentTime = millis();
    unsigned int v_blinkInterval = 0;
    CRGB v_blinkColor; // 깜빡일 때 사용할 색상

    switch (g_W10_currentLedStatus) {
        case LED_STATUS_INIT:           // 파란색 깜빡임 (초기화)
        case LED_STATUS_WIFI_CONNECTING:    // 파란색 천천히 깜빡임 (Wi-Fi 연결 시도)
            v_blinkInterval = 1000; // 1초 간격
            v_blinkColor = CRGB::Blue;
            break;
        case LED_STATUS_MQTT_DISCONNECTED:  // 주황색 깜빡임 (MQTT 연결 실패)
            v_blinkInterval = 500; // 0.5초 간격
            v_blinkColor = CRGB::Orange;
            break;
        case LED_STATUS_OTA_PROGRESS:       // 흰색 깜빡임 (OTA 진행 중)
            v_blinkInterval = 200; // 0.2초 간격
            v_blinkColor = CRGB::White;
            break;
        case LED_STATUS_CONFIG_PORTAL:      // 보라색 깜빡임 (설정 포털)
            v_blinkInterval = 300; // 0.3초 간격
            v_blinkColor = CRGB::Purple;
            break;
        case LED_STATUS_WIFI_DISCONNECTED:  // 빨간색 빠르게 깜빡임 (Wi-Fi 끊김)
            v_blinkInterval = 200; // 0.2초 간격
            v_blinkColor = CRGB::Red;
            break;
        default:
            // 고정 색상 상태는 별도 업데이트 필요 없음 (W10_setLedStatus에서 이미 설정됨)
            return;
    }

    if (v_currentTime - g_W10_lastLedUpdateTime >= v_blinkInterval) {
        g_W10_lastLedUpdateTime = v_currentTime;
        if (g_W10_leds[0] == CRGB::Black) {
            g_W10_leds[0] = v_blinkColor; // LED 켜기 (깜빡임 색상으로)
        } else {
            g_W10_leds[0] = CRGB::Black; // LED 끄기
        }
        FastLED.show();
    }
}


// --- OTA 업데이트 진행률 콜백 함수 ---
void W10_handlePreOtaUpdateCallback(){
    Update.onProgress([](unsigned int progress, unsigned int total) {
        // OTA 진행 중 LED 깜빡임
        if (g_W10_currentLedStatus != LED_STATUS_OTA_PROGRESS) {
            W10_setLedStatus(LED_STATUS_OTA_PROGRESS);
        }
    });
}

// --- WiFiManager 설정 저장 알림 콜백 함수 ---
void W10_saveConfigCallback() {
    Serial.println("Should save config");
    g_W10_shouldSaveConfig = true;
}

// W10_Advanced_006.h 파일 내 W10_loadJsonConfig 함수
void W10_loadJsonConfig(){
    Serial.println("mounting FS...");

    if (LittleFS.begin()) {
        Serial.println("mounted file system");
        if (LittleFS.exists(G_W10_WM_CONFIG_FILE)) {
            Serial.println("reading config file");
            File v_configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "r");
            if (v_configFile) {
                Serial.println("opened config file");
                size_t v_fileSize = v_configFile.size();
                std::unique_ptr<char[]> v_buffer(new char[v_fileSize + 1]);
                v_configFile.readBytes(v_buffer.get(), v_fileSize);
                v_buffer.get()[v_fileSize] = '\0';
                v_configFile.close();
    
                JsonDocument v_jsonDoc;
                auto v_deserializeError = deserializeJson(v_jsonDoc, v_buffer.get());
                serializeJson(v_jsonDoc, Serial);
                Serial.println();

                if ( ! v_deserializeError ) {
                    Serial.println("\nparsed json");
                    // 설정 구조체에 값 로드 (JSON에 값이 없으면 구조체 기본값이 유지됨)
                    strlcpy(g_W10_appConfig.mqttServer, v_jsonDoc["mqtt_server"] | g_W10_appConfig.mqttServer, sizeof(g_W10_appConfig.mqttServer));
                    g_W10_appConfig.mqttPort            = v_jsonDoc["mqtt_port"]    | g_W10_appConfig.mqttPort;
                    strlcpy(g_W10_appConfig.apiToken, v_jsonDoc["api_token"]    | g_W10_appConfig.apiToken, sizeof(g_W10_appConfig.apiToken));

                    Serial.println("setting AP ip from config");    
                    g_W10_appConfig.use_custom_ap_ip    = v_jsonDoc["use_custom_ap_ip"] | g_W10_appConfig.use_custom_ap_ip;
                    strlcpy(g_W10_appConfig.ap_Ip       , v_jsonDoc["ap_ip"]        | g_W10_appConfig.ap_Ip, sizeof(g_W10_appConfig.ap_Ip));
                    strlcpy(g_W10_appConfig.ap_Gateway  , v_jsonDoc["ap_gateway"]   | g_W10_appConfig.ap_Gateway, sizeof(g_W10_appConfig.ap_Gateway));
                    strlcpy(g_W10_appConfig.ap_Subnet   , v_jsonDoc["ap_subnet"]    | g_W10_appConfig.ap_Subnet, sizeof(g_W10_appConfig.ap_Subnet));
                    
                    // Wi-Fi STA 모드 설정 로드 (JSON 필드명 반영)
                    strlcpy(g_W10_appConfig.wifiSsid    , v_jsonDoc["wifi_ssid"]    | g_W10_appConfig.wifiSsid, sizeof(g_W10_appConfig.wifiSsid));
                    strlcpy(g_W10_appConfig.wifiPassword, v_jsonDoc["wifi_password"]| g_W10_appConfig.wifiPassword, sizeof(g_W10_appConfig.wifiPassword));
                    g_W10_appConfig.wifiUseDhcp         = v_jsonDoc["wifi_use_dhcp"]| g_W10_appConfig.wifiUseDhcp;
                    strlcpy(g_W10_appConfig.wifiIp      , v_jsonDoc["wifi_ip"]      | g_W10_appConfig.wifiIp, sizeof(g_W10_appConfig.wifiIp));
                    strlcpy(g_W10_appConfig.wifiGateway , v_jsonDoc["wifi_gateway"] | g_W10_appConfig.wifiGateway, sizeof(g_W10_appConfig.wifiGateway));
                    strlcpy(g_W10_appConfig.wifiSubnet  , v_jsonDoc["wifi_subnet"]  | g_W10_appConfig.wifiSubnet, sizeof(g_W10_appConfig.wifiSubnet));
                    strlcpy(g_W10_appConfig.wifiDns     , v_jsonDoc["wifi_dns"]     | g_W10_appConfig.wifiDns, sizeof(g_W10_appConfig.wifiDns));
                                        
                    g_W10_appConfig.isWmNonBlocking = v_jsonDoc["wm_nonblocking"] | g_W10_appConfig.isWmNonBlocking;

                    Serial.print("논블로킹 모드 설정: ");
                    Serial.println(g_W10_appConfig.isWmNonBlocking ? "활성화" : "비활성화");
                
                } else {
                    Serial.print("failed to load json config: parsing error -> ");
                    Serial.println(v_deserializeError.c_str());
                }
            } else {
                Serial.println("failed to open config file");
            }
        } else {
            Serial.println("config file does not exist. Using default settings from struct.");
        }
    } else {
        Serial.println("failed to mount FS! Configuration cannot be loaded.");
    }

    Serial.println("--- 로드된 설정 요약 ---");
    Serial.println("API Token: "    + String(g_W10_appConfig.apiToken));
    Serial.println("MQTT Server: " + String(g_W10_appConfig.mqttServer));
    Serial.println("MQTT Port: "   + String(g_W10_appConfig.mqttPort));
    Serial.println("논블로킹 모드: "    + String(g_W10_appConfig.isWmNonBlocking ? "활성화" : "비활성화"));
    Serial.println("--- Wi-Fi 네트워크 설정 ---");
    Serial.println("WiFi SSID: "        + String(g_W10_appConfig.wifiSsid));
    Serial.println("WiFi DHCP: "        + String(g_W10_appConfig.wifiUseDhcp ? "Yes" : "No"));
    Serial.println("WiFi IP: "          + String(g_W10_appConfig.wifiIp));
    Serial.println("WiFi Gateway: "    + String(g_W10_appConfig.wifiGateway));
    Serial.println("WiFi Subnet: "     + String(g_W10_appConfig.wifiSubnet));
    Serial.println("WiFi DNS: "         + String(g_W10_appConfig.wifiDns));
    Serial.println("--------------------");
}

    
// --- JSON 설정 파일 저장 함수 ---
void W10_saveJsonConfig(){
    if (g_W10_shouldSaveConfig) {
        Serial.println("saving config");
        JsonDocument v_jsonDoc;

        // 설정 구조체에서 값 저장
        v_jsonDoc["mqtt_server"]    = g_W10_appConfig.mqttServer;
        v_jsonDoc["mqtt_port"]      = g_W10_appConfig.mqttPort;
        v_jsonDoc["api_token"]      = g_W10_appConfig.apiToken;

        v_jsonDoc["use_custom_ap_ip"] = g_W10_appConfig.use_custom_ap_ip;
        v_jsonDoc["ap_ip"]          = g_W10_appConfig.ap_Ip; // 저장된 AP IP 사용
        v_jsonDoc["ap_gateway"]     = g_W10_appConfig.ap_Gateway; // 저장된 AP Gateway 사용
        v_jsonDoc["ap_subnet"]      = g_W10_appConfig.ap_Subnet;

        // WiFiManager에 의해 설정된 현재 Wi-Fi 정보를 JSON에 저장 (STA 모드)
        // 이 값들은 설정 포털 제출 시 업데이트되거나, autoConnect 성공 시 최신 값으로 반영됩니다.
        v_jsonDoc["wifi_ssid"]      = WiFi.SSID();
        v_jsonDoc["wifi_password"]  = WiFi.psk(); // PSK (Pre-Shared Key) - 보안 유의
        v_jsonDoc["wifi_use_dhcp"]  = g_W10_appConfig.wifiUseDhcp; // AppConfig의 값을 직접 사용
        v_jsonDoc["wifi_ip"]        = WiFi.localIP().toString();
        v_jsonDoc["wifi_gateway"]   = WiFi.gatewayIP().toString();
        v_jsonDoc["wifi_subnet"]    = WiFi.subnetMask().toString();
        v_jsonDoc["wifi_dns"]       = WiFi.dnsIP().toString();

        v_jsonDoc["wm_nonblocking"] = g_W10_appConfig.isWmNonBlocking;
        
        File v_configFile = LittleFS.open(G_W10_WM_CONFIG_FILE, "w");
        if (!v_configFile) {
            Serial.println("failed to open config file for writing");
        }

        serializeJson(v_jsonDoc, Serial); Serial.println();

        serializeJson(v_jsonDoc, v_configFile);

        v_configFile.close();

        Serial.println("config saved successfully.");
        g_W10_shouldSaveConfig = false;
    }
    Serial.println("--- 현재 네트워크 정보 ---");
    Serial.println("로컬 IP: "            + WiFi.localIP().toString());
    Serial.println("게이트웨이 IP: "      + WiFi.gatewayIP().toString());
    Serial.println("서브넷 마스크: "      + WiFi.subnetMask().toString());
    Serial.println("-----------------------");
}

// --- Wi-Fi 이벤트 핸들러 ---
void W10_handleWiFiEvent(arduino_event_id_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi STA Started");
            W10_setLedStatus(LED_STATUS_WIFI_CONNECTING);
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("WiFi Connected! IP address: ");
            Serial.println(WiFi.localIP());
            Serial.print("SSID: ");
            Serial.println(WiFi.SSID());
            W10_setLedStatus(LED_STATUS_WIFI_CONNECTED);
            g_W10_shouldReconnectWifi = false; // 연결 성공 시 재연결 플래그 초기화
            // WiFiManager가 저장한 현재 네트워크 정보를 AppConfig에 반영 (수동 저장 시 사용)
            strlcpy(g_W10_appConfig.wifiSsid, WiFi.SSID().c_str(), sizeof(g_W10_appConfig.wifiSsid));
            strlcpy(g_W10_appConfig.wifiPassword, WiFi.psk().c_str(), sizeof(g_W10_appConfig.wifiPassword));
            strlcpy(g_W10_appConfig.wifiIp, WiFi.localIP().toString().c_str(), sizeof(g_W10_appConfig.wifiIp));
            strlcpy(g_W10_appConfig.wifiGateway, WiFi.gatewayIP().toString().c_str(), sizeof(g_W10_appConfig.wifiGateway));
            strlcpy(g_W10_appConfig.wifiSubnet, WiFi.subnetMask().toString().c_str(), sizeof(g_W10_appConfig.wifiSubnet));
            strlcpy(g_W10_appConfig.wifiDns, WiFi.dnsIP().toString().c_str(), sizeof(g_W10_appConfig.wifiDns));
            // DHCP 사용 여부는 WiFiManager 파라미터에서 가져오는 것이 더 정확할 수 있으나,
            // WiFi.getAutoConnect()는 자동으로 연결된 경우 true를 반환합니다.
            // 여기서는 AppConfig의 값을 유지하는 것이 일관성 있습니다.
            // g_W10_appConfig.wifiUseDhcp = (WiFi.getMode() == WIFI_STA && WiFi.getAutoConnect());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WiFi STA Disconnected.");
            W10_setLedStatus(LED_STATUS_WIFI_DISCONNECTED);
            g_W10_shouldReconnectWifi = true; // 재연결 필요 플래그 설정
            g_W10_lastReconnectAttempt = millis(); // 재연결 시도 시간 기록
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("WiFi AP Started (Config Portal)");
            W10_setLedStatus(LED_STATUS_CONFIG_PORTAL);
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("WiFi AP Stopped");
            // AP 모드 종료 후 다시 STA 모드로 전환될 것이므로, 연결 상태에 따라 LED 설정
            if (WiFi.isConnected()) {
                W10_setLedStatus(LED_STATUS_WIFI_CONNECTED);
            } else {
                W10_setLedStatus(LED_STATUS_WIFI_DISCONNECTED);
            }
            break;
        default:
            break;
    }
}

// --- 초기화 함수 ---
void W10_init() {
    
    // 1. LED 초기화
    FastLED.addLeds<WS2812B, G_W10_LED_PIN, GRB>(g_W10_leds, G_W10_NUM_LEDS);
    FastLED.setBrightness(50); // LED 밝기 설정 (0-255)
    W10_setLedStatus(LED_STATUS_INIT); // 초기화 중 LED 상태 표시

    // 2. Wi-Fi 모드 설정 및 초기 설정 로드
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(W10_handleWiFiEvent); // Wi-Fi 이벤트 핸들러 등록

    W10_loadJsonConfig(); // 저장된 JSON 설정 파일 로드

    // WiFiManager 논블로킹 모드 설정
    g_W10_wifiManager.setConfigPortalBlocking(!g_W10_appConfig.isWmNonBlocking);

    // 3. WiFiManager 콜백 함수 등록
    g_W10_wifiManager.setSaveConfigCallback(W10_saveConfigCallback);
    g_W10_wifiManager.setSaveParamsCallback(W10_saveParamCallback);
    // OTA 업데이트 진행 콜백은 ArduinoOTA에 직접 등록하며, WiFiManager의 콜백은 아님.
    // W10_handlePreOtaUpdateCallback() 함수 내에서 ArduinoOTA.onProgress를 등록합니다.

    // 4. WiFiManager 사용자 정의 매개변수 정의
    // 설정 구조체에서 값 가져와서 매개변수 생성
    WiFiManagerParameter v_customMqttServer(    "server"    , "mqtt server" , g_W10_appConfig.mqttServer    , sizeof(g_W10_appConfig.mqttServer));
    
    // mqttPort는 int형이므로 char 배열로 변환하여 WiFiManagerParameter에 전달합니다.
    char v_mqttPortStr[8]; // 포트 번호 (최대 65535) 및 NULL 종료 문자 포함
    snprintf(v_mqttPortStr, sizeof(v_mqttPortStr), "%d", g_W10_appConfig.mqttPort); // itoa 대신 snprintf 사용
    WiFiManagerParameter v_customMqttPort(      "port"      , "mqtt port"   , v_mqttPortStr                 , sizeof(v_mqttPortStr));
    
    WiFiManagerParameter v_customApiToken(      "apikey"    , "API token"   , g_W10_appConfig.apiToken      , sizeof(g_W10_appConfig.apiToken));

    char v_nonBlock_checkboxChecked[10];
    if (g_W10_appConfig.isWmNonBlocking) {
        strcpy(v_nonBlock_checkboxChecked, "checked");
    } else {
        strcpy(v_nonBlock_checkboxChecked, "");
    }
    char v_nonBlock_checkboxHtml[100];
    sprintf(v_nonBlock_checkboxHtml, "<br/><input type='checkbox' name='wm_nonblocking' value='true' %s> 논블로킹 모드 사용", v_nonBlock_checkboxChecked);
    WiFiManagerParameter v_wmNonBlockingCheckbox(v_nonBlock_checkboxHtml);

    // DHCP 사용 여부 체크박스 추가
    char v_dhcpChecked[10];
    if (g_W10_appConfig.wifiUseDhcp) {
        strcpy(v_dhcpChecked, "checked");
    } else {
        strcpy(v_dhcpChecked, "");
    }
    char v_dhcpHtml[100];
    sprintf(v_dhcpHtml, "<br/><input type='checkbox' name='wifi_use_dhcp' value='true' %s> DHCP 사용", v_dhcpChecked);
    WiFiManagerParameter v_wifiUseDhcpCheckbox(v_dhcpHtml);


    char v_macAddressStr[18];
    String v_currentMac = WiFi.macAddress();
    v_currentMac.toCharArray(v_macAddressStr, sizeof(v_macAddressStr));
    WiFiManagerParameter v_macParam("<p><strong>디바이스 MAC 주소:</strong></p>", "MAC Address", v_macAddressStr, sizeof(v_macAddressStr), "readonly");
    
    // 사용자 정의 라디오 버튼 (지역 변수로 선언)
    const char* v_customRadioStr  = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    WiFiManagerParameter v_customField(v_customRadioStr); // 전역 변수 g_W10_customField 제거 후 지역 변수로 사용

    // 5. 모든 매개변수를 WiFiManager에 추가
    g_W10_wifiManager.addParameter(&v_customMqttServer);
    g_W10_wifiManager.addParameter(&v_customMqttPort);
    g_W10_wifiManager.addParameter(&v_customApiToken);
    g_W10_wifiManager.addParameter(&v_wmNonBlockingCheckbox);
    g_W10_wifiManager.addParameter(&v_wifiUseDhcpCheckbox); // DHCP 체크박스 추가
    g_W10_wifiManager.addParameter(&v_macParam);
    g_W10_wifiManager.addParameter(&v_customField); // 지역 변수로 선언된 매개변수 추가

    // 6. WiFiManager의 고급 동작 설정 (STA 모드 정적 IP)
    // DHCP 사용 여부에 따라 정적 IP 필드 표시 여부 결정
    g_W10_wifiManager.setShowStaticFields(!g_W10_appConfig.wifiUseDhcp); // DHCP 사용 안 하면 정적 IP 필드 표시
    g_W10_wifiManager.setShowDnsFields(!g_W10_appConfig.wifiUseDhcp);    // DHCP 사용 안 하면 DNS 필드 표시
    
    // JSON에서 로드된 고정 IP 정보로 설정
    IPAddress _ip, _gw, _sn, _dns;
    _ip.fromString(g_W10_appConfig.wifiIp);
    _gw.fromString(g_W10_appConfig.wifiGateway);
    _sn.fromString(g_W10_appConfig.wifiSubnet);
    _dns.fromString(g_W10_appConfig.wifiDns);
    
    g_W10_wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn, _dns);

    // AP 정적 IP 설정 (사용자 정의 AP IP 사용 시)
    if(g_W10_appConfig.use_custom_ap_ip){
        IPAddress _ap_ip, _ap_gw, _ap_sn;
        _ap_ip.fromString(g_W10_appConfig.ap_Ip);
        _ap_gw.fromString(g_W10_appConfig.ap_Gateway);
        _ap_sn.fromString(g_W10_appConfig.ap_Subnet);
    
        g_W10_wifiManager.setAPStaticIPConfig(_ap_ip, _ap_gw, _ap_sn);
    }
    
    // "wifi","wifinoscan","info","param","close","sep","erase","restart","exit"
    std::vector<const char*> v_menu = { "wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
    g_W10_wifiManager.setMenu(v_menu);
    g_W10_wifiManager.setClass("invert");
    g_W10_wifiManager.setConfigPortalTimeout(30); // 설정 포털 타임아웃 30초

    // 7. Wi-Fi 네트워크에 자동 연결 시도
    Serial.println("Attempting Wi-Fi autoConnect...");
    W10_setLedStatus(LED_STATUS_WIFI_CONNECTING); // 연결 시도 중 LED
    bool v_connectResult;

    // WiFiManager는 이전에 성공적으로 연결된 네트워크 정보를 내부적으로 관리합니다.
    // JSON에 특정 SSID/패스워드가 있더라도, WiFiManager의 autoConnect는 내부 저장된 자격 증명을 우선 시도합니다.
    // 여기서는 단순히 autoConnect를 호출하여 WiFiManager가 최적의 연결을 시도하도록 합니다.
    v_connectResult = g_W10_wifiManager.autoConnect(g_W10_apName, g_W10_apPassword);

    if (!v_connectResult) {
        Serial.println("Wi-Fi 연결 실패 또는 타임아웃 발생. 설정 포털 대기 중...");
        W10_setLedStatus(LED_STATUS_WIFI_DISCONNECTED); // 연결 실패 LED
        g_W10_shouldReconnectWifi = true; // 재연결 플래그 설정
        g_W10_lastReconnectAttempt = millis();
    } else {
        Serial.println("Wi-Fi 연결 성공 :)");
        // LED 상태는 W10_handleWiFiEvent에서 ARDUINO_EVENT_WIFI_STA_GOT_IP 이벤트로 설정됩니다.
    }

    // 8. 웹 UI에서 업데이트된 매개변수 값들을 전역 변수(구조체)에 반영하고 설정 저장
    // 이 부분은 saveParamCallback에서 처리되므로 여기서는 필요 없을 수 있습니다.
    // 하지만 autoConnect가 성공한 경우 최신 Wi-Fi 정보를 AppConfig에 반영하는 것은 유용합니다.
    strlcpy(g_W10_appConfig.mqttServer, v_customMqttServer.getValue(), sizeof(g_W10_appConfig.mqttServer));
    g_W10_appConfig.mqttPort = atoi(v_customMqttPort.getValue()); // string to int
    strlcpy(g_W10_appConfig.apiToken, v_customApiToken.getValue(), sizeof(g_W10_appConfig.apiToken));
    
    // WiFiManager에 의해 설정된 IP/DNS 정보 가져오기
    // autoConnect 성공 후 WiFiManager가 저장한 값을 AppConfig에 반영합니다.
    strlcpy(g_W10_appConfig.wifiIp, WiFi.localIP().toString().c_str(), sizeof(g_W10_appConfig.wifiIp));
    strlcpy(g_W10_appConfig.wifiGateway, WiFi.gatewayIP().toString().c_str(), sizeof(g_W10_appConfig.wifiGateway));
    strlcpy(g_W10_appConfig.wifiSubnet, WiFi.subnetMask().toString().c_str(), sizeof(g_W10_appConfig.wifiSubnet));
    strlcpy(g_W10_appConfig.wifiDns, WiFi.dnsIP().toString().c_str(), sizeof(g_W10_appConfig.wifiDns));
    strlcpy(g_W10_appConfig.wifiSsid, WiFi.SSID().c_str(), sizeof(g_W10_appConfig.wifiSsid));
    strlcpy(g_W10_appConfig.wifiPassword, WiFi.psk().c_str(), sizeof(g_W10_appConfig.wifiPassword));
    // DHCP 사용 여부는 saveParamCallback에서 이미 처리되거나, 초기 로드된 값을 유지합니다.
    // g_W10_appConfig.wifiUseDhcp = (WiFi.getMode() == WIFI_STA && WiFi.getAutoConnect()); // 이 로직은 불확실하므로 유지하지 않음

    // 논블로킹 모드 설정 반영
    // WiFiManagerParameter의 값은 saveParamCallback에서 AppConfig에 반영됩니다.
    // 이 부분은 초기화 시점에 설정 포털에 진입하지 않는 경우에도 AppConfig 값을 동기화하기 위함입니다.
    // 그러나, saveParamCallback이 호출되지 않았다면 이 값은 초기 로드된 값과 동일할 것입니다.
    // 따라서 이 부분은 주로 설정 포털 진입 후 saveParamCallback이 호출되었을 때 AppConfig를 최신화하는 역할을 합니다.
    // (WiFiManager.server->hasArg를 확인하는 대신, g_W10_appConfig 값을 직접 사용)
    g_W10_wifiManager.setConfigPortalBlocking(!g_W10_appConfig.isWmNonBlocking);

    W10_saveJsonConfig(); // 모든 변경된 설정들을 JSON 파일에 저장

    // 9. 온디맨드 설정 포털 및 OTA 업데이트 기능 초기화 (조건부 컴파일)
    #ifdef G_W10_ONDEMAND_ENABLE
        g_W10_button.attachClick(W10_startConfigPortal);        // 짧게 클릭 시 설정 포털 시작
        g_W10_button.attachLongPressStart(W10_resetSettings);   // 길게 누름 시 설정 초기화
    #endif
    
    #ifdef G_W10_OTA_ENABLE
        ArduinoOTA.onStart([]() {
            Serial.println("OTA Start");
            W10_setLedStatus(LED_STATUS_OTA_START);
        });
        // OTA 진행률 콜백 함수는 W10_handlePreOtaUpdateCallback에서 설정됩니다.
        // ArduinoOTA.onProgress는 W10_handlePreOtaUpdateCallback 내부에서 람다 함수로 정의되어 있습니다.
        W10_handlePreOtaUpdateCallback(); // OTA 진행률 콜백 등록
        
        ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA End");
            W10_setLedStatus(LED_STATUS_OTA_END);
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)            Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)      Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)    Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)    Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR)        Serial.println("End Failed");
            W10_setLedStatus(LED_STATUS_OTA_ERROR);
        });
        ArduinoOTA.begin();
    #endif
}

// --- 설정 포털을 시작하는 함수 (버튼 클릭 시 호출) ---
void W10_startConfigPortal() {
    Serial.println("버튼이 클릭되었습니다. 설정 포털을 시작합니다.");
    W10_setLedStatus(LED_STATUS_CONFIG_PORTAL); // 설정 포털 진입 LED
    g_W10_wifiManager.setConfigPortalTimeout(120); // 설정 포털 타임아웃 120초

    if (!g_W10_wifiManager.startConfigPortal(g_W10_apName, g_W10_apPassword)) {
        Serial.println("설정 포털 연결 실패 또는 타임아웃 발생");
        delay(3000);
        // ESP.restart(); // 필요시 주석 해제하여 디바이스 재시작
    } else {
        Serial.println("Wi-Fi 연결 성공 또는 설정이 업데이트되었습니다.");
        g_W10_shouldSaveConfig = true; // 설정이 변경되었으므로 저장 플래그 설정
        W10_saveJsonConfig();          // 변경된 설정 즉시 저장
        // LED 상태는 W10_handleWiFiEvent에서 Wi-Fi 연결 상태에 따라 자동으로 설정됩니다.
    }
}

// --- 설정을 초기화하고 재시작하는 함수 (버튼 길게 누름 시 호출) ---
void W10_resetSettings() {
    Serial.println("버튼이 길게 눌렸습니다. 설정을 초기화하고 재시작합니다.");
    g_W10_wifiManager.resetSettings(); // WiFiManager의 Wi-Fi 자격 증명 삭제
    if (LittleFS.exists(G_W10_WM_CONFIG_FILE)) {
        LittleFS.remove(G_W10_WM_CONFIG_FILE); // 사용자 정의 설정 파일 삭제
        Serial.println("config.json 파일 삭제 완료.");
    }
    ESP.restart(); // 디바이스 재시작
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
    // "customfieldid" 매개변수 값 가져오기 (예시)
    Serial.println("매개변수 customfieldid = " + W10_getParam("customfieldid")); 
    
    g_W10_shouldSaveConfig = true; // 매개변수 변경이 있었으므로 저장 플래그 설정
    
    // 매개변수 변경 시 AppConfig 구조체 값 업데이트
    if (g_W10_wifiManager.server->hasArg("server")) strlcpy(g_W10_appConfig.mqttServer, g_W10_wifiManager.server->arg("server").c_str(), sizeof(g_W10_appConfig.mqttServer));
    if (g_W10_wifiManager.server->hasArg("port")) g_W10_appConfig.mqttPort = atoi(g_W10_wifiManager.server->arg("port").c_str());
    if (g_W10_wifiManager.server->hasArg("apikey")) strlcpy(g_W10_appConfig.apiToken, g_W10_wifiManager.server->arg("apikey").c_str(), sizeof(g_W10_appConfig.apiToken));

    // WiFiManager가 제공하는 정적 IP 필드 값 가져오기
    // 'ip', 'gateway', 'subnet', 'dns' 필드는 WiFiManager가 내부적으로 관리합니다.
    // 여기서 AppConfig에 반영하는 것은 다음 재부팅 시에도 이 설정이 유지되도록 JSON에 저장하기 위함입니다.
    if (g_W10_wifiManager.server->hasArg("ip")) strlcpy(g_W10_appConfig.wifiIp, g_W10_wifiManager.server->arg("ip").c_str(), sizeof(g_W10_appConfig.wifiIp));
    if (g_W10_wifiManager.server->hasArg("gateway")) strlcpy(g_W10_appConfig.wifiGateway, g_W10_wifiManager.server->arg("gateway").c_str(), sizeof(g_W10_appConfig.wifiGateway));
    if (g_W10_wifiManager.server->hasArg("subnet")) strlcpy(g_W10_appConfig.wifiSubnet, g_W10_wifiManager.server->arg("subnet").c_str(), sizeof(g_W10_appConfig.wifiSubnet));
    if (g_W10_wifiManager.server->hasArg("dns")) strlcpy(g_W10_appConfig.wifiDns, g_W10_wifiManager.server->arg("dns").c_str(), sizeof(g_W10_appConfig.wifiDns));

    // DHCP 사용 체크박스 값 가져오기 및 WiFiManager 설정에 반영
    if (g_W10_wifiManager.server->hasArg("wifi_use_dhcp") && g_W10_wifiManager.server->arg("wifi_use_dhcp").equalsIgnoreCase("true")) {
        g_W10_appConfig.wifiUseDhcp = true;
    } else {
        g_W10_appConfig.wifiUseDhcp = false;
    }
    // DHCP 사용 여부에 따라 정적 IP/DNS 필드 표시 여부 업데이트
    g_W10_wifiManager.setShowStaticFields(!g_W10_appConfig.wifiUseDhcp);
    g_W10_wifiManager.setShowDnsFields(!g_W10_appConfig.wifiUseDhcp);

    // 논블로킹 모드 체크박스 값 가져오기 및 WiFiManager 설정에 반영
    if (g_W10_wifiManager.server->hasArg("wm_nonblocking") && g_W10_wifiManager.server->arg("wm_nonblocking").equalsIgnoreCase("true")) {
        g_W10_appConfig.isWmNonBlocking = true;
    } else {
        g_W10_appConfig.isWmNonBlocking = false;
    }
    g_W10_wifiManager.setConfigPortalBlocking(!g_W10_appConfig.isWmNonBlocking); // WiFiManager 동작 모드 변경
}

// --- 메인 루프에서 지속적으로 실행될 함수 ---
void W10_run() {
    // 논블로킹 모드 활성화 시 WiFiManager의 백그라운드 작업을 처리
    if (g_W10_appConfig.isWmNonBlocking) { // 변경: g_W10_isWmNonBlocking 대신 g_W10_appConfig.isWmNonBlocking 사용
        g_W10_wifiManager.process();
    }
    
    // 온디맨드 기능 활성화 시 버튼 상태 업데이트
    #ifdef G_W10_ONDEMAND_ENABLE
        g_W10_button.tick();
    #endif
    
    // OTA 업데이트 기능 활성화 시 OTA 이벤트 처리
    #ifdef G_W10_OTA_ENABLE
        ArduinoOTA.handle();
    #endif

    // Wi-Fi 재연결 로직
    // Wi-Fi 연결이 끊어졌고, 재연결 시도 간격이 지났다면 재연결 시도
    if (g_W10_shouldReconnectWifi && (millis() - g_W10_lastReconnectAttempt >= G_W10_RECONNECT_INTERVAL_MS)) {
        Serial.println("Attempting Wi-Fi reconnection...");
        W10_setLedStatus(LED_STATUS_WIFI_CONNECTING); // 재연결 시도 중 LED

        // 저장된 자격 증명으로 Wi-Fi 재연결 시도 (WiFi.reconnect() 사용)
        // WiFiManager의 autoConnect는 설정 포털을 시작하는 역할이 강하므로,
        // 단순 재연결에는 WiFi.reconnect()가 더 적합합니다.
        if (WiFi.reconnect()) {
            Serial.println("Wi-Fi reconnect successful!");
            g_W10_shouldReconnectWifi = false; // 연결 성공 시 플래그 해제
            // LED 상태는 ARDUINO_EVENT_WIFI_STA_GOT_IP 이벤트에서 W10_handleWiFiEvent가 처리합니다.
        } else {
            Serial.println("Wi-Fi reconnect failed. Retrying...");
            g_W10_lastReconnectAttempt = millis(); // 재연결 시도 시간 업데이트
        }
    }

    // LED 상태 업데이트 (깜빡임 패턴 등)
    W10_updateLedStatus();

    // 임시 MQTT 연결 상태 시뮬레이션 (실제 MQTT 클라이언트 통합 시 제거)
    // 이 부분은 실제 MQTT 연결 로직으로 대체되어야 합니다.
    static unsigned long lastMqttCheck = 0;
    if (millis() - lastMqttCheck > 10000) { // 10초마다 MQTT 상태 변경 시뮬레이션
        g_W10_mqttConnected = !g_W10_mqttConnected;
        if (WiFi.isConnected()) { // Wi-Fi 연결 상태일 때만 MQTT LED 상태 반영
            if (g_W10_mqttConnected) {
                W10_setLedStatus(LED_STATUS_MQTT_CONNECTED);
            } else {
                W10_setLedStatus(LED_STATUS_MQTT_DISCONNECTED);
            }
        }
        lastMqttCheck = millis();
    }
}

#endif // W10_ADVANCED_006_H
