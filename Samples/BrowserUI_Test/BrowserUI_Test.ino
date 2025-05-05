//  HWに適したピン定義をインクルードする
#include "../mini_PF_pins.hpp"
// #include "../mini_PF_pins_S3.hpp"

// 下記ライブラリを使用
#include <WiFi.h>
#include <WebServer.h>

// テストにLEDを使用

// AP情報

#define AP_SSID     "ESP32_AP"
// コメントアウト解除でパスワードあり
// #define AP_PASSWORD "12345678" 


// LED状態配列
bool LED_state[3] = { false, false, false };

// LED制御関数
void setLED(bool RED,bool YEL,bool GRN){
    digitalWrite(PIN_LED_RED,RED);
    digitalWrite(PIN_LED_YEL,YEL);
    digitalWrite(PIN_LED_GRN,GRN);
}

// Webサーバ初期化
WebServer server(80);

// HTML生成関数
String getHTML() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <title>ESP32 IO Control</title>
      <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; }
        button { font-size: 24px; padding: 10px 20px; margin: 10px; }
        .status { font-size: 20px; margin: 10px; }
      </style>
    </head>
    <body>
      <h1>ESP32 IO Control</h1>
      <button onclick="toggleLED(0)">Toggle LED 1</button>
      <div class="status">LED 1: <span id="led1">---</span></div>
      <button onclick="toggleLED(1)">Toggle LED 2</button>
      <div class="status">LED 2: <span id="led2">---</span></div>
      <button onclick="toggleLED(2)">Toggle LED 3</button>
      <div class="status">LED 3: <span id="led3">---</span></div>

      <script>
        function toggleLED(n) {
          fetch("/toggle?led=" + n)
            .then(() => updateStatus());
        }

        function updateStatus() {
          fetch("/status")
            .then(response => response.json())
            .then(data => {
              document.getElementById("led1").textContent = data[0] ? "ON" : "OFF";
              document.getElementById("led2").textContent = data[1] ? "ON" : "OFF";
              document.getElementById("led3").textContent = data[2] ? "ON" : "OFF";
            });
        }

        setInterval(updateStatus, 500);
        window.onload = updateStatus;
      </script>
    </body>
    </html>
  )rawliteral";

  return html;
}

// HTML画面処理
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// ボタンクリック時のLED状態切り替え処理
void handleToggle() {
  if (server.hasArg("led")) {
    int led = server.arg("led").toInt();
    if (led >= 0 && led < 3) {
      LED_state[led] = !LED_state[led];
    }
  }
  server.send(200, "text/plain", "OK");
}

// JSONでLED状態を返す
void handleStatus() {
  String json = "[";
  for (int i = 0; i < 3; i++) {
    json += (LED_state[i] ? "true" : "false");
    if (i < 2) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}


void setup() {
    // LEDピン初期化
    pinMode(PIN_LED_RED,OUTPUT);
    pinMode(PIN_LED_YEL,OUTPUT);
    pinMode(PIN_LED_GRN,OUTPUT);

    // イニシャル点灯
    setLED(0,0,1);
    delay(200);
    setLED(0,1,0);
    delay(200);
    setLED(1,0,0);
    delay(200);
    setLED(0,0,0);
    delay(600);

  // IP設定とAPモード開始（パスワードなし）
    IPAddress local_ip(192, 168,   0,   1);
    IPAddress gateway (192, 168,   0,   1);
    IPAddress subnet  (255, 255, 255,   0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    #ifdef AP_PASSWORD
        WiFi.softAP(AP_SSID, AP_PASSWORD );
    #else
        WiFi.softAP(AP_SSID, "" );
    #endif
    
    
  // サーバールート登録
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  server.begin();

    setLED(1,1,1);
    delay(200);
    setLED(0,0,0);
    delay(200);
    setLED(1,1,1);
    delay(200);
    setLED(0,0,0);
}

void loop() {
    server.handleClient();

    setLED(LED_state[0],LED_state[1],LED_state[2]);

    delay(100);
}
