#pragma once

//ESP_Natural_breeze_Fan_001

// 2025-07-07: 사용자 요구사항 정리
// 1. ESP32 보드를 사용하여 아두이노 기반 자연풍 선풍기 구현
// 2. 웹 인터페이스를 통한 설정 제어 기능 추가 (ESPAsyncWebServer 라이브러리 활용)
//    - MIN_SPEED (최소 바람 세기), MAX_SPEED (최대 바람 세기) 설정
//    - 사인파 주기 (minInterval) 설정 (바람 강약 변화의 빠르기 조절)
//    - 랜덤 변화 주기 (maxInterval) 설정 (바람 불규칙성 변화의 빈도 조절)
// 3. PIR 모션 센서 연동 기능 추가
//    - PIR 센서 적용 여부 설정 (웹에서 On/Off)
//    - 움직임 감지 시 선풍기 작동, 일정 시간 움직임 없을 시 정지
// 4. DHT11 (또는 DHT22) 온습도 센서 연동 기능 추가
//    - 온습도 센서 적용 여부 설정 (웹에서 On/Off)
//    - 특정 온도 및 습도 임계값 설정 (웹에서 조절)
//    - 온습도 조건 충족 시에만 선풍기 작동 (예: 온도/습도 임계값 이상일 때)
// 5. 자연풍 구현 방식 개선: 사인파와 랜덤(Random) 요소를 결합하여 더 자연스러운 바람 구현
//    - 사인파를 기반으로 바람의 강도를 부드럽게 변화
//    - 여기에 작은 랜덤 오프셋을 더해 불규칙성 추가
// 6. 소스코드 출력 시 컴파일 오류 유발하는 유니코드 공백문자, byte order mark 등을 빠짐없이 제거
// 7. PlatformIO Arduino Core 사용 명시

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h> // 비동기 웹 서버 라이브러리
#include <DHT.h>               // DHT 센서 라이브러리

// WiFi 설정 (본인의 WiFi 정보로 변경하세요)
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// 웹 서버 객체 생성 (포트 80)
AsyncWebServer server(80);

// 모터 드라이버 핀 정의 (L298N 기준)
const int motorEnablePin = 25;  // 모터 속도 제어 (PWM 핀)
const int motorInput1 = 26;     // 모터 방향 제어 핀 1
const int motorInput2 = 27;     // 모터 방향 제어 핀 2

// DHT11 센서 설정
#define DHTPIN 14       // DHT 데이터 핀 (예: GPIO 14)
#define DHTTYPE DHT11   // DHT11 센서 사용 (DHT22를 사용하려면 DHT22로 변경)
DHT dht(DHTPIN, DHTTYPE);

// PIR 센서 설정
const int pirPin = 13; // PIR 센서 OUT 핀 (예: GPIO 13)

// 자연풍 선풍기 설정 변수 (웹에서 제어)
int fanMinSpeed = 80;   // 최소 바람 세기 (0-255 PWM 값)
int fanMaxSpeed = 255;  // 최대 바람 세기 (0-255 PWM 값)

// 사인파 및 랜덤 변화 관련 변수 (웹에서 제어)
float sineFrequency = 0.0005; // 사인파 주기 조절 (값이 작을수록 주기가 길어짐)
long randomInterval = 3000; // 랜덤 값을 얼마나 자주 갱신할지 (ms)

bool pirSensorEnabled = false; // PIR 센서 적용 여부
bool dhtSensorEnabled = false; // DHT11 센서 적용 여부
float dhtTempThreshold = 28.0; // DHT 온도 임계값 (°C, 이 온도 이상일 때 선풍기 작동)
float dhtHumidThreshold = 70.0; // DHT 습도 임계값 (%, 이 습도 이상일 때 선풍기 작동)

// 자연풍 로직 내부 변수
float currentMotorSpeed = 0; // 부드러운 변화를 위해 float 사용
unsigned long previousSineMillis = 0;
float sineAngle = 0; // 사인파의 현재 각도

const int RANDOM_DEVIATION = 20; // 사인파 기본 속도에 더해질 랜덤 값의 최대 편차 (PWM 값)
unsigned long previousRandomMillis = 0;
int randomOffset = 0; // 현재 랜덤 오프셋

// 센서 상태 및 제어 로직 변수
unsigned long lastPirDetectionTime = 0;
const long PIR_INACTIVE_TIMEOUT = 10000; // PIR 감지 후 선풍기 유지 시간 (밀리초)

float temperature = 0.0;
float humidity = 0.0;
unsigned long lastDhtReadTime = 0;
const long DHT_READ_INTERVAL = 2000; // DHT 센서 읽는 간격 (밀리초)

// HTML 웹 페이지 내용
// 웹 페이지는 인라인으로 정의되어 있습니다. 복사하여 웹 브라우저에서 직접 열어볼 수도 있습니다.
const char* htmlPage PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>자연풍 선풍기 제어</title>
<style>
  body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
  .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 600px; margin: auto; }
  h2 { color: #333; }
  label { display: block; margin-top: 10px; font-weight: bold; }
  input[type="number"], input[type="range"] { width: calc(100% - 22px); padding: 8px; margin-top: 5px; border: 1px solid #ddd; border-radius: 4px; }
  input[type="checkbox"] { margin-right: 10px; }
  button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }
  button:hover { background-color: #45a049; }
  .status { margin-top: 20px; padding: 10px; background-color: #e7f3e7; border-left: 6px solid #4CAF50; }
  .sensor-data { margin-top: 15px; padding: 10px; background-color: #e0f7fa; border-left: 6px solid #00bcd4; }
</style>
</head>
<body>
<div class="container">
  <h2>자연풍 선풍기 설정</h2>

  <div class="status" id="status">상태: 연결됨</div>
  <div class="sensor-data">
    현재 온도: <span id="temp">N/A</span>°C, 습도: <span id="hum">N/A</span>%
    <br>PIR 감지: <span id="pirStatus">N/A</span>
  </div>

  <form id="fanSettingsForm">
    <label for="minSpeed">최소 바람 세기 (0-255):</label>
    <input type="range" id="minSpeed" name="minSpeed" min="0" max="255" oninput="this.nextElementSibling.value=this.value">
    <output>80</output><br>

    <label for="maxSpeed">최대 바람 세기 (0-255):</label>
    <input type="range" id="maxSpeed" name="maxSpeed" min="0" max="255" oninput="this.nextElementSibling.value=this.value">
    <output>255</output><br>

    <label for="minInterval">사인파 주기 (느림-빠름):</label>
    <input type="range" id="minInterval" name="minInterval" min="100" max="5000" oninput="this.nextElementSibling.value=this.value">
    <output>500</output><br>

    <label for="maxInterval">랜덤 변화 주기 (느림-빠름):</label>
    <input type="range" id="maxInterval" name="maxInterval" min="500" max="10000" oninput="this.nextElementSibling.value=this.value">
    <output>3000</output><br>

    <label><input type="checkbox" id="pirEnable" name="pirEnable"> PIR 센서 적용</label><br>
    <label><input type="checkbox" id="dhtEnable" name="dhtEnable"> 온습도 센서 적용</label><br>
    
    <div id="dhtConditions" style="display:none;">
      <label for="tempThreshold">온도 임계값 (°C):</label>
      <input type="number" id="tempThreshold" name="tempThreshold" step="0.1" value="28.0"><br>
      <label for="humidThreshold">습도 임계값 (%):</label>
      <input type="number" id="humidThreshold" name="humidThreshold" step="0.1" value="70.0"><br>
    </div>

    <button type="submit">설정 적용</button>
  </form>

</div>

<script>
  const minSpeedInput = document.getElementById('minSpeed');
  const maxSpeedInput = document.getElementById('maxSpeed');
  const minIntervalInput = document.getElementById('minInterval'); // 사인파 주기에 사용
  const maxIntervalInput = document.getElementById('maxInterval'); // 랜덤 주기에 사용
  const pirEnableInput = document.getElementById('pirEnable');
  const dhtEnableInput = document.getElementById('dhtEnable');
  const tempThresholdInput = document.getElementById('tempThreshold');
  const humidThresholdInput = document.getElementById('humidThreshold');
  const dhtConditionsDiv = document.getElementById('dhtConditions');
  const statusDiv = document.getElementById('status');
  const tempSpan = document.getElementById('temp');
  const humSpan = document.getElementById('hum');
  const pirStatusSpan = document.getElementById('pirStatus');

  // 초기 설정값 로드
  function loadSettings() {
    fetch('/settings')
      .then(response => response.json())
      .then(data => {
        minSpeedInput.value = data.minSpeed;
        minSpeedInput.nextElementSibling.value = data.minSpeed;
        maxSpeedInput.value = data.maxSpeed;
        maxSpeedInput.nextElementSibling.value = data.maxSpeed;
        minIntervalInput.value = data.minInterval;
        minIntervalInput.nextElementSibling.value = data.minInterval;
        maxIntervalInput.value = data.maxInterval;
        maxIntervalInput.nextElementSibling.value = data.maxInterval;
        pirEnableInput.checked = data.pirEnabled;
        dhtEnableInput.checked = data.dhtEnabled;
        tempThresholdInput.value = data.tempThreshold;
        humidThresholdInput.value = data.humidThreshold;
        toggleDhtConditions(); // DHT 조건 표시/숨김
        updateSensorData(); // 초기 센서 데이터 로드
      })
      .catch(error => console.error('Error loading settings:', error));
  }

  // DHT 조건 입력 필드 표시/숨김 토글
  function toggleDhtConditions() {
    dhtConditionsDiv.style.display = dhtEnableInput.checked ? 'block' : 'none';
  }
  dhtEnableInput.addEventListener('change', toggleDhtConditions);

  // 폼 제출 시 설정 저장
  document.getElementById('fanSettingsForm').addEventListener('submit', function(event) {
    event.preventDefault();
    const formData = {
      minSpeed: parseInt(minSpeedInput.value),
      maxSpeed: parseInt(maxSpeedInput.value),
      minInterval: parseInt(minIntervalInput.value), // 사인파 주기
      maxInterval: parseInt(maxIntervalInput.value), // 랜덤 주기
      pirEnable: pirEnableInput.checked,
      dhtEnable: dhtEnableInput.checked,
      tempThreshold: parseFloat(tempThresholdInput.value),
      humidThreshold: parseFloat(humidThresholdInput.value)
    };

    fetch('/settings', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(formData)
    })
    .then(response => response.text())
    .then(data => {
      statusDiv.textContent = '상태: ' + data;
      console.log(data);
    })
    .catch(error => {
      statusDiv.textContent = '상태: 설정 저장 실패!';
      console.error('Error saving settings:', error);
    });
  });

  // 센서 데이터 주기적으로 업데이트
  function updateSensorData() {
    fetch('/sensor_data')
      .then(response => response.json())
      .then(data => {
        tempSpan.textContent = data.temperature.toFixed(1);
        humSpan.textContent = data.humidity.toFixed(1);
        pirStatusSpan.textContent = data.pirDetected ? '감지됨' : '감지 안 됨';
      })
      .catch(error => console.error('Error fetching sensor data:', error));
  }

  // 페이지 로드 시 초기 설정 및 센서 데이터 로드
  window.onload = function() {
    loadSettings();
    setInterval(updateSensorData, 3000); // 3초마다 센서 데이터 업데이트
  };
</script>
</body>
</html>
)rawliteral";

// WiFi 연결 함수
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// 웹 서버 핸들러 설정
void setupWebHandlers() {
  // 루트 페이지 핸들러
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });

  // 설정값 가져오기 핸들러
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"minSpeed\":" + String(fanMinSpeed) + ",";
    json += "\"maxSpeed\":" + String(fanMaxSpeed) + ",";
    json += "\"minInterval\":" + String(map(sineFrequency * 100000.0, 1.0, 100.0, 5000.0, 100.0)) + ","; // sineFrequency 역변환
    json += "\"maxInterval\":" + String(randomInterval) + ",";
    json += "\"pirEnabled\":" + String(pirSensorEnabled ? "true" : "false") + ",";
    json += "\"dhtEnabled\":" + String(dhtSensorEnabled ? "true" : "false") + ",";
    json += "\"tempThreshold\":" + String(dhtTempThreshold) + ",";
    json += "\"humidThreshold\":" + String(dhtHumidThreshold);
    json += "}";
    request->send(200, "application/json", json);
  });

  // 설정값 업데이트 핸들러
  server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request){
    // POST 요청 본문에서 파라미터 파싱
    String requestBody = request->arg(0); // 첫 번째 POST 인자를 전체 JSON 문자열로 가정
    
    // JSON 파싱을 위한 간단한 방법 (ESPAsyncWebServer는 form data 파싱이 더 일반적)
    // 실제로는 ArduinoJson 라이브러리 사용이 권장되지만, 여기서는 간략화
    
    // minSpeed
    int newMinSpeed = request->arg("minSpeed").toInt();
    if (newMinSpeed >= 0 && newMinSpeed <= 255) fanMinSpeed = newMinSpeed;
    
    // maxSpeed
    int newMaxSpeed = request->arg("maxSpeed").toInt();
    if (newMaxSpeed >= 0 && newMaxSpeed <= 255) fanMaxSpeed = newMaxSpeed;

    // minInterval (사인파 주기) -> sineFrequency로 변환
    long newMinInterval = request->arg("minInterval").toInt();
    if (newMinInterval >= 100 && newMinInterval <= 5000) {
      sineFrequency = map(newMinInterval, 100, 5000, 100, 1) / 100000.0;
    }
    
    // maxInterval (랜덤 변화 주기)
    long newMaxInterval = request->arg("maxInterval").toInt();
    if (newMaxInterval >= 500 && newMaxInterval <= 10000) randomInterval = newMaxInterval;

    // pirEnable
    pirSensorEnabled = (request->arg("pirEnable") == "true");

    // dhtEnable
    dhtSensorEnabled = (request->arg("dhtEnable") == "true");

    // tempThreshold
    float newTempThreshold = request->arg("tempThreshold").toFloat();
    if (!isnan(newTempThreshold)) dhtTempThreshold = newTempThreshold;

    // humidThreshold
    float newHumidThreshold = request->arg("humidThreshold").toFloat();
    if (!isnan(newHumidThreshold)) dhtHumidThreshold = newHumidThreshold;
    
    // 설정 변경 후 시리얼 출력
    Serial.println("\n--- 설정 업데이트 ---");
    Serial.print("Min Speed: "); Serial.println(fanMinSpeed);
    Serial.print("Max Speed: "); Serial.println(fanMaxSpeed);
    Serial.print("Sine Frequency: "); Serial.println(sineFrequency, 6); // 소수점 6자리까지 출력
    Serial.print("Random Interval: "); Serial.println(randomInterval);
    Serial.print("PIR Enabled: "); Serial.println(pirSensorEnabled ? "True" : "False");
    Serial.print("DHT Enabled: "); Serial.println(dhtSensorEnabled ? "True" : "False");
    Serial.print("Temp Threshold: "); Serial.println(dhtTempThreshold);
    Serial.print("Humid Threshold: "); Serial.println(dhtHumidThreshold);
    Serial.println("--------------------");

    request->send(200, "text/plain", "설정 저장 완료!");
  });

  // 센서 데이터 가져오기 핸들러
  server.on("/sensor_data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"temperature\":" + String(temperature, 1) + ","; // 소수점 1자리까지
    json += "\"humidity\":" + String(humidity, 1) + ",";       // 소수점 1자리까지
    json += "\"pirDetected\":" + String(digitalRead(pirPin) == HIGH ? "true" : "false");
    json += "}";
    request->send(200, "application/json", json);
  });

  // 웹 서버 시작
  server.begin();
}

void F10_init() {

  // 핀 모드 설정
  pinMode(motorEnablePin, OUTPUT);
  pinMode(motorInput1, OUTPUT);
  pinMode(motorInput2, OUTPUT);
  pinMode(pirPin, INPUT); // PIR 센서 입력 핀

  // 모터 정방향으로 설정 (필요에 따라 반대로 설정해도 됨)
  digitalWrite(motorInput1, HIGH);
  digitalWrite(motorInput2, LOW);

  // 난수 생성을 위한 시드 설정 (연결되지 않은 아날로그 핀의 노이즈 활용)
  randomSeed(analogRead(0));

  dht.begin(); // DHT 센서 시작

  connectToWiFi(); // WiFi 네트워크 연결
  setupWebHandlers(); // 웹 서버 핸들러 설정 및 시작

  // 초기 사인파 주파수 설정 (웹 컨트롤러의 minInterval 기본값 500에 해당)
  sineFrequency = map(500.0, 100.0, 5000.0, 100.0, 1.0) / 100000.0;
  // 초기 랜덤 변화 주기 설정 (웹 컨트롤러의 maxInterval 기본값 3000에 해당)
  randomInterval = 3000;

  Serial.println("아두이노 자연풍 선풍기 시작 (웹 제어 가능)!");
}

void F10_run() {
  unsigned long currentMillis = millis();

  // DHT 센서 데이터 읽기 (일정 간격마다)
  if (dhtSensorEnabled && currentMillis - lastDhtReadTime >= DHT_READ_INTERVAL) {
    lastDhtReadTime = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("DHT 센서 읽기 실패!");
      // 오류 발생 시 이전 값 유지 또는 기본값 설정
      temperature = 0.0;
      humidity = 0.0;
    } else {
      temperature = t;
      humidity = h;
      // Serial.print("온도: "); Serial.print(temperature); Serial.print("°C, 습도: "); Serial.print(humidity); Serial.println("%");
    }
  }

  // 선풍기 작동 조건 결정
  bool fanShouldRun = false;

  // 1. PIR 센서 적용 여부 확인
  if (pirSensorEnabled) {
    if (digitalRead(pirPin) == HIGH) { // PIR 센서에서 움직임 감지
      lastPirDetectionTime = currentMillis; // 마지막 감지 시간 갱신
      fanShouldRun = true; // 선풍기 작동 허용
      // Serial.println("PIR 감지! 선풍기 작동.");
    } else if (currentMillis - lastPirDetectionTime < PIR_INACTIVE_TIMEOUT) {
      // 움직임이 없더라도 'PIR_INACTIVE_TIMEOUT' 동안은 작동 유지
      fanShouldRun = true;
      // Serial.println("PIR 감지 후 유지 시간.");
    }
  } else {
    // PIR 센서가 비활성화된 경우, 항상 선풍기 작동을 허용
    fanShouldRun = true;
  }

  // 2. DHT 센서 적용 여부 및 조건 확인 (PIR이 작동을 허용했을 때만 추가 검토)
  if (dhtSensorEnabled && fanShouldRun) {
    // 현재 온도와 습도가 임계값 미만이면 선풍기 정지
    if (temperature < dhtTempThreshold || humidity < dhtHumidThreshold) {
      fanShouldRun = false; // 온도 또는 습도 조건 미달 시 선풍기 정지
      // Serial.println("DHT 조건 미달. 선풍기 정지.");
    }
  }

  // 최종 결정에 따라 선풍기 작동 로직 실행
  if (fanShouldRun) {
    // 자연풍 로직 (사인파 + 랜덤 결합)
    // 1. 사인파 기반 기본 속도 계산
    // sineAngle은 시간에 따라 연속적으로 증가하여 sin() 함수의 입력으로 사용
    sineAngle += sineFrequency * (currentMillis - previousSineMillis);
    previousSineMillis = currentMillis;

    // sin() 함수는 -1에서 1 사이의 값을 반환하므로, 이를 0에서 255 (또는 fanMinSpeed ~ fanMaxSpeed) 범위로 매핑
    // (sin(angle) + 1.0) 은 0 ~ 2 범위
    float baseSpeed = (fanMaxSpeed - fanMinSpeed) / 2.0 * (sin(sineAngle) + 1.0) + fanMinSpeed;

    // 2. 랜덤 오프셋 추가 (일정 간격마다 새로운 랜덤 값 갱신)
    if (currentMillis - previousRandomMillis >= randomInterval) {
      previousRandomMillis = currentMillis;
      // -RANDOM_DEVIATION 부터 +RANDOM_DEVIATION 까지의 랜덤 값 생성
      randomOffset = random(-RANDOM_DEVIATION, RANDOM_DEVIATION + 1);
    }
    
    // 최종 모터 속도 계산 (사인파 기반 속도 + 랜덤 오프셋)
    currentMotorSpeed = baseSpeed + randomOffset;
    
    // 계산된 속도 값이 설정된 최소/최대 속도 범위를 벗어나지 않도록 제한
    currentMotorSpeed = constrain(currentMotorSpeed, fanMinSpeed, fanMaxSpeed);

    // 모터 속도 적용 (PWM 값은 정수여야 함)
    analogWrite(motorEnablePin, (int)currentMotorSpeed);
  } else {
    // 선풍기 정지
    currentMotorSpeed = 0;
    analogWrite(motorEnablePin, 0);
    // Serial.println("선풍기 정지.");
  }
  
  // 다음 루프 실행까지 짧은 딜레이를 주어 안정적인 작동 및 속도 변화의 부드러움을 유지
  delay(10);
}
