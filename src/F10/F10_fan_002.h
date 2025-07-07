#pragma once

//ESP_Natural_breeze_Fan_001

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <LittleFS.h> // LittleFS 라이브러리 추가
#include <ArduinoJson.h> // ArduinoJson 라이브러리 추가

// --- 전역 상수 정의 (G_F10_ 접두사) ---
// WiFi 설정 (본인의 WiFi 정보로 변경하세요)
const char* G_F10_WIFI_SSID = "YOUR_WIFI_SSID";
const char* G_F10_WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 모터 드라이버 핀 정의 (L298N 기준)
const int G_F10_MOTOR_ENABLE_PIN = 25;  // 모터 속도 제어 (PWM 핀)
const int G_F10_MOTOR_INPUT1_PIN = 26;     // 모터 방향 제어 핀 1
const int G_F10_MOTOR_INPUT2_PIN = 27;     // 모터 방향 제어 핀 2

// DHT11 센서 설정
const int G_F10_DHT_PIN = 14;       // DHT 데이터 핀 (예: GPIO 14)
#define G_F10_DHT_TYPE DHT11   // DHT11 센서 사용 (DHT22를 사용하려면 DHT22로 변경)

// PIR 센서 설정
const int G_F10_PIR_PIN = 13; // PIR 센서 OUT 핀 (예: GPIO 13)

// 자연풍 로직 내부 상수
const int G_F10_RANDOM_DEVIATION = 20; // 사인파 기본 속도에 더해질 랜덤 값의 최대 편차 (PWM 값)
const long G_F10_PIR_INACTIVE_TIMEOUT_MS = 10000; // PIR 감지 후 선풍기 유지 시간 (밀리초)
const long G_F10_DHT_READ_INTERVAL_MS = 2000; // DHT 센서 읽는 간격 (밀리초)
const int G_F10_LOOP_DELAY_MS = 10; // 메인 루프 딜레이 (밀리초)

// LittleFS 파일 경로 상수
const char* G_F10_SETTINGS_FILE_PATH = "/F10_settings_001.json"; // 설정 저장 파일
const char* G_F10_HTML_FILE_PATH = "/F10_index_001.html";       // HTML 페이지 파일

// --- 전역 변수 정의 (g_F10_ 접두사) ---
// 웹 서버 객체
AsyncWebServer g_F10_server(80);

// DHT 센서 객체
DHT g_F10_dht(G_F10_DHT_PIN, G_F10_DHT_TYPE);

// 자연풍 선풍기 설정 변수 (웹에서 제어)
int g_F10_fanMinSpeed = 80;   // 최소 바람 세기 (0-255 PWM 값)
int g_F10_fanMaxSpeed = 255;  // 최대 바람 세기 (0-255 PWM 값)

// 사인파 및 랜덤 변화 관련 변수 (웹에서 제어)
float g_F10_sineFrequency = 0.0005; // 사인파 주기 조절 (값이 작을수록 주기가 길어짐)
long g_F10_randomInterval = 3000; // 랜덤 값을 얼마나 자주 갱신할지 (ms)

bool g_F10_pirSensorEnabled = false; // PIR 센서 적용 여부
bool g_F10_dhtSensorEnabled = false; // DHT11 센서 적용 여부
float g_F10_dhtTempThreshold = 28.0; // DHT 온도 임계값 (°C, 이 온도 이상일 때 선풍기 작동)
float g_F10_dhtHumidThreshold = 70.0; // DHT 습도 임계값 (%, 이 습도 이상일 때 선풍기 작동)

// 자연풍 로직 내부 변수
float g_F10_currentMotorSpeed = 0; // 부드러운 변화를 위해 float 사용
unsigned long g_F10_previousSineMillis = 0;
float g_F10_sineAngle = 0; // 사인파의 현재 각도
unsigned long g_F10_previousRandomMillis = 0;
int g_F10_randomOffset = 0; // 현재 랜덤 오프셋

// 센서 상태 및 제어 로직 변수
unsigned long g_F10_lastPirDetectionTime = 0;
float g_F10_temperature = 0.0;
float g_F10_humidity = 0.0;
unsigned long g_F10_lastDhtReadTime = 0;

// 웹 페이지 내용 (이제 G_F10_HTML_PAGE는 사용하지 않습니다. 파일에서 불러옵니다.)
// const char* G_F10_HTML_PAGE PROGMEM = R"rawliteral(...)";


// --- 함수 정의 (F10_ 접두사) ---

/**
 * @brief 설정값을 LittleFS 파일에 JSON 형식으로 저장합니다.
 */
void F10_saveJson_Settings() {
  // 로컬 변수 정의 (v_ 접두사)
  JsonDocument v_doc; // JSON 문서 크기 (필요에 따라 조절)

  v_doc["minSpeed"] = g_F10_fanMinSpeed;
  v_doc["maxSpeed"] = g_F10_fanMaxSpeed;
  // 사인파 주파수를 웹의 minInterval 값으로 역변환하여 저장
  v_doc["minInterval"] = map(g_F10_sineFrequency * 100000.0, 1.0, 100.0, 5000.0, 100.0);
  v_doc["maxInterval"] = g_F10_randomInterval;
  v_doc["pirEnabled"] = g_F10_pirSensorEnabled;
  v_doc["dhtEnabled"] = g_F10_dhtSensorEnabled;
  v_doc["tempThreshold"] = g_F10_dhtTempThreshold;
  v_doc["humidThreshold"] = g_F10_dhtHumidThreshold;

  File v_settingsFile = LittleFS.open(G_F10_SETTINGS_FILE_PATH, "w");
  if (!v_settingsFile) {
    Serial.println("LittleFS: 설정 파일을 열 수 없습니다!");
    return;
  }

  if (serializeJson(v_doc, v_settingsFile) == 0) {
    Serial.println("LittleFS: 설정 파일 쓰기 실패!");
  } else {
    Serial.println("LittleFS: 설정 저장 완료.");
  }
  v_settingsFile.close();
}

/**
 * @brief LittleFS 파일에서 JSON 형식의 설정값을 불러옵니다.
 */
void F10_loadJson_Settings() {
  // 로컬 변수 정의 (v_ 접두사)
  File v_settingsFile = LittleFS.open(G_F10_SETTINGS_FILE_PATH, "r");
  if (!v_settingsFile) {
    Serial.println("LittleFS: 설정 파일이 없습니다. 기본값을 사용합니다.");
    F10_saveSettings(); // 파일이 없으면 기본값으로 저장
    return;
  }

  JsonDocument v_doc;
  DeserializationError v_error = deserializeJson(v_doc, v_settingsFile);

  if (v_error) {
    Serial.print("LittleFS: 설정 파일 읽기 실패: ");
    Serial.println(v_error.c_str());
    v_settingsFile.close();
    return;
  }

  g_F10_fanMinSpeed = v_doc["minSpeed"] | g_F10_fanMinSpeed; // | 연산자를 사용하여 기본값 제공
  g_F10_fanMaxSpeed = v_doc["maxSpeed"] | g_F10_fanMaxSpeed;
  
  long v_loadedMinInterval = v_doc["minInterval"] | 500; // 웹 기본값 500 사용
  g_F10_sineFrequency = map((float)v_loadedMinInterval, 100.0, 5000.0, 100.0, 1.0) / 100000.0;

  g_F10_randomInterval = v_doc["maxInterval"] | g_F10_randomInterval;
  g_F10_pirSensorEnabled = v_doc["pirEnabled"] | g_F10_pirSensorEnabled;
  g_F10_dhtSensorEnabled = v_doc["dhtEnabled"] | g_F10_dhtSensorEnabled;
  g_F10_dhtTempThreshold = v_doc["tempThreshold"] | g_F10_dhtTempThreshold;
  g_F10_dhtHumidThreshold = v_doc["humidThreshold"] | g_F10_dhtHumidThreshold;

  v_settingsFile.close();
  Serial.println("LittleFS: 설정 불러오기 완료.");
}


/**
 * @brief WiFi 네트워크에 연결합니다.
 */
void F10_connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(G_F10_WIFI_SSID, G_F10_WIFI_PASSWORD);
  int v_retries = 0;
  while (WiFi.status() != WL_CONNECTED && v_retries < 20) { // 10초 타임아웃 (20 * 500ms)
    delay(500);
    Serial.print(".");
    v_retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi 연결 실패!");
  }
}

/**
 * @brief 웹 서버 핸들러를 설정합니다.
 */
void F10_setupWebHandlers() {
  // 루트 페이지 핸들러: LittleFS에서 HTML 파일 제공
  g_F10_server.on("/", HTTP_GET, [](AsyncWebServerRequest *p_request){
    p_request->send(LittleFS, G_F10_HTML_FILE_PATH, "text/html");
  });

  // 설정값 가져오기 핸들러
  g_F10_server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *p_request){
    // 로컬 변수 정의 (v_ 접두사)
    JsonDocument v_doc;
    v_doc["minSpeed"] = g_F10_fanMinSpeed;
    v_doc["maxSpeed"] = g_F10_fanMaxSpeed;
    // 사인파 주파수를 웹의 minInterval 값으로 역변환하여 전송
    v_doc["minInterval"] = map(g_F10_sineFrequency * 100000.0, 1.0, 100.0, 5000.0, 100.0);
    v_doc["maxInterval"] = g_F10_randomInterval;
    v_doc["pirEnabled"] = g_F10_pirSensorEnabled;
    v_doc["dhtEnabled"] = g_F10_dhtSensorEnabled;
    v_doc["tempThreshold"] = g_F10_dhtTempThreshold;
    v_doc["humidThreshold"] = g_F10_dhtHumidThreshold;

    String v_responseJson;
    serializeJson(v_doc, v_responseJson);
    p_request->send(200, "application/json", v_responseJson);
  });

  // 설정값 업데이트 핸들러
  g_F10_server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *p_request){
    // 로컬 변수 정의 (v_ 접두사)
    if (p_request->hasParam("plain", true)) {
        String v_requestBody = p_request->arg("plain");
        JsonDocument v_doc;
        DeserializationError v_error = deserializeJson(v_doc, v_requestBody);

        if (v_error) {
            Serial.print("JSON 파싱 실패: ");
            Serial.println(v_error.c_str());
            p_request->send(400, "text/plain", "JSON 파싱 오류!");
            return;
        }

        g_F10_fanMinSpeed = v_doc["minSpeed"] | g_F10_fanMinSpeed;
        g_F10_fanMaxSpeed = v_doc["maxSpeed"] | g_F10_fanMaxSpeed;
        
        long v_newMinInterval = v_doc["minInterval"] | 500;
        g_F10_sineFrequency = map((float)v_newMinInterval, 100.0, 5000.0, 100.0, 1.0) / 100000.0;
        
        g_F10_randomInterval = v_doc["maxInterval"] | g_F10_randomInterval;

        g_F10_pirSensorEnabled = v_doc["pirEnable"] | g_F10_pirSensorEnabled;
        g_F10_dhtSensorEnabled = v_doc["dhtEnable"] | g_F10_dhtSensorEnabled;

        g_F10_dhtTempThreshold = v_doc["tempThreshold"] | g_F10_dhtTempThreshold;
        g_F10_dhtHumidThreshold = v_doc["humidThreshold"] | g_F10_dhtHumidThreshold;
        
        // 설정 변경 후 시리얼 출력
        Serial.println("\n--- 설정 업데이트 ---");
        Serial.print("Min Speed: "); Serial.println(g_F10_fanMinSpeed);
        Serial.print("Max Speed: "); Serial.println(g_F10_fanMaxSpeed);
        Serial.print("Sine Frequency: "); Serial.println(g_F10_sineFrequency, 6);
        Serial.print("Random Interval: "); Serial.println(g_F10_randomInterval);
        Serial.print("PIR Enabled: "); Serial.println(g_F10_pirSensorEnabled ? "True" : "False");
        Serial.print("DHT Enabled: "); Serial.println(g_F10_dhtSensorEnabled ? "True" : "False");
        Serial.print("Temp Threshold: "); Serial.println(g_F10_dhtTempThreshold);
        Serial.print("Humid Threshold: "); Serial.println(g_F10_dhtHumidThreshold);
        Serial.println("--------------------");

        F10_saveJson_Settings(); // 설정 변경 후 LittleFS에 저장
        p_request->send(200, "text/plain", "설정 저장 완료!");
    } else {
        p_request->send(400, "text/plain", "잘못된 요청!");
    }
  });

  // 센서 데이터 가져오기 핸들러
  g_F10_server.on("/sensor_data", HTTP_GET, [](AsyncWebServerRequest *p_request){
    // 로컬 변수 정의 (v_ 접두사)
    JsonDocument v_doc;
    v_doc["temperature"] = g_F10_temperature;
    v_doc["humidity"] = g_F10_humidity;
    v_doc["pirDetected"] = (digitalRead(G_F10_PIR_PIN) == HIGH);

    String v_responseJson;
    serializeJson(v_doc, v_responseJson);
    p_request->send(200, "application/json", v_responseJson);
  });

  // 웹 서버 시작
  g_F10_server.begin();
}

/**
 * @brief 센서 데이터를 읽고 전역 변수에 업데이트합니다.
 * @param p_currentMillis 현재 밀리초 값
 */
void F10_readSensorData(unsigned long p_currentMillis) {
  if (g_F10_dhtSensorEnabled && p_currentMillis - g_F10_lastDhtReadTime >= G_F10_DHT_READ_INTERVAL_MS) {
    g_F10_lastDhtReadTime = p_currentMillis;
    float v_h = g_F10_dht.readHumidity();
    float v_t = g_F10_dht.readTemperature();

    if (isnan(v_h) || isnan(v_t)) {
      Serial.println("DHT 센서 읽기 실패!");
      // 오류 발생 시 이전 값 유지 또는 기본값 설정
      g_F10_temperature = 0.0;
      g_F10_humidity = 0.0;
    } else {
      g_F10_temperature = v_t;
      g_F10_humidity = v_h;
      // Serial.print("온도: "); Serial.print(g_F10_temperature); Serial.print("°C, 습도: "); Serial.print(g_F10_humidity); Serial.println("%");
    }
  }
}

/**
 * @brief 선풍기 작동 조건을 결정합니다.
 * @param p_currentMillis 현재 밀리초 값
 * @return 선풍기가 작동해야 하는지 여부 (true/false)
 */
bool F10_determineFanRunCondition(unsigned long p_currentMillis) {
  bool v_fanShouldRun = false;

  // 1. PIR 센서 적용 여부 확인
  if (g_F10_pirSensorEnabled) {
    if (digitalRead(G_F10_PIR_PIN) == HIGH) { // PIR 센서에서 움직임 감지
      g_F10_lastPirDetectionTime = p_currentMillis; // 마지막 감지 시간 갱신
      v_fanShouldRun = true; // 선풍기 작동 허용
      // Serial.println("PIR 감지! 선풍기 작동.");
    } else if (p_currentMillis - g_F10_lastPirDetectionTime < G_F10_PIR_INACTIVE_TIMEOUT_MS) {
      // 움직임이 없더라도 'G_F10_PIR_INACTIVE_TIMEOUT_MS' 동안은 작동 유지
      v_fanShouldRun = true;
      // Serial.println("PIR 감지 후 유지 시간.");
    }
  } else {
    // PIR 센서가 비활성화된 경우, 항상 선풍기 작동을 허용
    v_fanShouldRun = true;
  }

  // 2. DHT 센서 적용 여부 및 조건 확인 (PIR이 작동을 허용했을 때만 추가 검토)
  if (g_F10_dhtSensorEnabled && v_fanShouldRun) {
    // 현재 온도와 습도가 임계값 미만이면 선풍기 정지
    if (g_F10_temperature < g_F10_dhtTempThreshold || g_F10_humidity < g_F10_dhtHumidThreshold) {
      v_fanShouldRun = false; // 온도 또는 습도 조건 미달 시 선풍기 정지
      // Serial.println("DHT 조건 미달. 선풍기 정지.");
    }
  }
  return v_fanShouldRun;
}

/**
 * @brief 자연풍 로직에 따라 모터 속도를 제어합니다.
 * 사인파와 랜덤 요소를 결합하여 자연스러운 바람을 생성합니다.
 * @param p_currentMillis 현재 밀리초 값
 */
void F10_controlNaturalFan(unsigned long p_currentMillis) {
  // 1. 사인파 기반 기본 속도 계산
  g_F10_sineAngle += g_F10_sineFrequency * (p_currentMillis - g_F10_previousSineMillis);
  g_F10_previousSineMillis = p_currentMillis;

  float v_baseSpeed = (g_F10_fanMaxSpeed - g_F10_fanMinSpeed) / 2.0 * (sin(g_F10_sineAngle) + 1.0) + g_F10_fanMinSpeed;

  // 2. 랜덤 오프셋 추가 (일정 간격마다 새로운 랜덤 값 갱신)
  if (p_currentMillis - g_F10_previousRandomMillis >= g_F10_randomInterval) {
    g_F10_previousRandomMillis = p_currentMillis;
    g_F10_randomOffset = random(-G_F10_RANDOM_DEVIATION, G_F10_RANDOM_DEVIATION + 1);
  }
  
  // 최종 모터 속도 계산 (사인파 기반 속도 + 랜덤 오프셋)
  g_F10_currentMotorSpeed = v_baseSpeed + g_F10_randomOffset;
  
  // 계산된 속도 값이 설정된 최소/최대 속도 범위를 벗어나지 않도록 제한
  g_F10_currentMotorSpeed = constrain(g_F10_currentMotorSpeed, g_F10_fanMinSpeed, g_F10_fanMaxSpeed);

  // 모터 속도 적용 (PWM 값은 정수여야 함)
  analogWrite(G_F10_MOTOR_ENABLE_PIN, (int)g_F10_currentMotorSpeed);
}

/**
 * @brief 선풍기를 완전히 정지시킵니다.
 */
void F10_stopFan() {
  g_F10_currentMotorSpeed = 0;
  analogWrite(G_F10_MOTOR_ENABLE_PIN, 0);
  // Serial.println("선풍기 정지.");
}

/**
 * @brief 시스템 초기화 함수. setup()에서 호출됩니다.
 */
void F10_init() {
  
  // 핀 모드 설정
  pinMode(G_F10_MOTOR_ENABLE_PIN, OUTPUT);
  pinMode(G_F10_MOTOR_INPUT1_PIN, OUTPUT);
  pinMode(G_F10_MOTOR_INPUT2_PIN, OUTPUT);
  pinMode(G_F10_PIR_PIN, INPUT); // PIR 센서 입력 핀

  // 모터 정방향으로 설정 (필요에 따라 반대로 설정해도 됨)
  digitalWrite(G_F10_MOTOR_INPUT1_PIN, HIGH);
  digitalWrite(G_F10_MOTOR_INPUT2_PIN, LOW);

  // 난수 생성을 위한 시드 설정 (연결되지 않은 아날로그 핀의 노이즈 활용)
  randomSeed(analogRead(0));

  g_F10_dht.begin(); // DHT 센서 시작

  // LittleFS 초기화
  if (!LittleFS.begin()) {
    Serial.println("LittleFS 마운트 실패!");
    return;
  }
  Serial.println("LittleFS 마운트 성공.");

  F10_loadJson_Settings(); // LittleFS에서 설정값 불러오기

  F10_connectToWiFi(); // WiFi 네트워크 연결
  F10_setupWebHandlers(); // 웹 서버 핸들러 설정 및 시작

  Serial.println("아두이노 자연풍 선풍기 시스템 초기화 완료!");
}

/**
 * @brief 시스템 메인 루프 함수. loop()에서 호출됩니다.
 */
void F10_run() {
  unsigned long v_currentMillis = millis();

  F10_readSensorData(v_currentMillis); // 센서 데이터 읽기

  bool v_fanShouldRun = F10_determineFanRunCondition(v_currentMillis); // 선풍기 작동 조건 결정

  // 최종 결정에 따라 선풍기 작동 로직 실행
  if (v_fanShouldRun) {
    F10_controlNaturalFan(v_currentMillis); // 자연풍 로직에 따라 모터 제어
  } else {
    F10_stopFan(); // 선풍기 정지
  }
  
  // 다음 루프 실행까지 짧은 딜레이를 주어 안정적인 작동 및 속도 변화의 부드러움을 유지
  delay(G_F10_LOOP_DELAY_MS);
}
