#include "../mini_PF_pins.hpp"

// 下記ライブラリを使用
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h> 

// AP情報
#define AP_SSID     "ESP32_AP"
// コメントアウト解除でパスワードあり
// #define AP_PASSWORD "12345678" 


// define
#define MAX_LOG 32

#define FORWARD_DUTY 255
#define BACKWARD_DUTY 127
#define TURN_DUTY_FW1  63
#define TURN_DUTY_FW2   0
#define TURN_DUTY_BW1   0
#define TURN_DUTY_BW2   0

#define MOVING_DURATION 500 // ms

#define STATUS_STOP       0
#define STATUS_FORWARD    1
#define STATUS_BACKWARD   2
#define STATUS_TURN_RIGHT 3 
#define STATUS_TURN_LEFT  4

// --------------------------------
// webサーバー定義
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// led変数
byte LED_state[3];

// 仮センサ変数
uint16_t sensorVal[3] = {0, 0, 0};
float    position     = 0.0;
int      status       = 0;

unsigned long lastSend = 0;
unsigned long stopTime = 0;

// ログ
String  logBuffer[MAX_LOG];
int     logIndex = 0;

// ログ関数
void GUI_log(const char* format, ...) {
    char buf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    logBuffer[logIndex] = String(buf);
    logIndex = (logIndex + 1) % MAX_LOG;
}

void LED_Nomal(){
    LED_state[0] = 0;
    LED_state[1] = 0;
    LED_state[2] = 1;
}


void LED_warn(){
    LED_state[0] = 0;
    LED_state[1] = 1;
    LED_state[2] = 0;
}

void LED_emg(){
    LED_state[0] = 1;
    LED_state[1] = 0;
    LED_state[2] = 0;
}


void setLED(bool RED,bool YEL,bool GRN){
    digitalWrite(PIN_LED_RED,RED);
    digitalWrite(PIN_LED_YEL,YEL);
    digitalWrite(PIN_LED_GRN,GRN);
}

// WebSocketイベント
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        if (msg.startsWith("CMD:")) {
            String cmd = msg.substring(4);
            if      (cmd == "F") { GUI_log("前進");   status = STATUS_FORWARD   ; stopTime = millis() + MOVING_DURATION; LED_Nomal();}
            else if (cmd == "B") { GUI_log("後退");   status = STATUS_BACKWARD  ; stopTime = millis() + MOVING_DURATION; LED_Nomal();}
            else if (cmd == "R") { GUI_log("右旋回"); status = STATUS_TURN_RIGHT; stopTime = millis() + MOVING_DURATION; LED_Nomal();}
            else if (cmd == "L") { GUI_log("左旋回"); status = STATUS_TURN_LEFT ; stopTime = millis() + MOVING_DURATION; LED_Nomal();}
            else if (cmd == "S") { GUI_log("停止");   status = STATUS_STOP;                                              LED_warn ();}
            else { GUI_log("無効なコマンド"); status = STATUS_STOP; }
        }
    }
}

// JSON送信
void sendData() {
    sensorVal[0] = analogRead(PIN_LINE_SENSOR_1);
    sensorVal[1] = analogRead(PIN_LINE_SENSOR_2);
    sensorVal[2] = analogRead(PIN_LINE_SENSOR_3);

    // position = 0.0;;

    StaticJsonDocument<512> doc;
    doc["s1"] = sensorVal[0];
    doc["s2"] = sensorVal[1];
    doc["s3"] = sensorVal[2];
    doc["position"] = position;
    doc["status"] = status;

    JsonArray log = doc.createNestedArray("log");
		for (int i = MAX_LOG - 1; i >= 0; --i) {
    		int idx = (logIndex + MAX_LOG - 1 - i) % MAX_LOG;
    		if (logBuffer[idx].length() > 0) log.add(logBuffer[idx]);
		}
    String out;
    serializeJson(doc, out);
    webSocket.broadcastTXT(out);
}

void setup() {
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_YEL, OUTPUT);
    pinMode(PIN_LED_GRN, OUTPUT);
    setLED(0, 0, 1); delay(200);
    setLED(0, 1, 0); delay(200);
    setLED(1, 0, 0); delay(200);
    setLED(0, 0, 0); delay(600);

    IPAddress local_ip(192, 168,   0,   1);
    IPAddress gateway (192, 168,   0,   1);
    IPAddress subnet  (255, 255, 255,   0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    #ifdef AP_PASSWORD
        WiFi.softAP(AP_SSID, AP_PASSWORD );
    #else
        WiFi.softAP(AP_SSID, "" );
    #endif
    

    // LittleFS 初期化
    if (!LittleFS.begin(true)) { //初回のみフォーマットが必要
//    if (!LittleFS.begin()) {
        GUI_log("LittleFS init failed!");
        setLED(1, 0, 0);
        return;
    }
    
    // HTMLをファイルから読み込む
    server.on("/", []() {
        File file = LittleFS.open("/index.html", "r");
        if (file) {
            server.streamFile(file, "text/html");
            file.close();
        } else {
            server.send(500, "text/plain", "index.html not found");
        }
    });


    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    setLED(1,1,1);
    delay(200);
    setLED(0,0,0);
    delay(200);
    setLED(1,1,1);
    delay(200);
    setLED(0,0,0);

    GUI_log("システム開始");
}

void loop() {
    server.handleClient();
    webSocket.loop();

    // LED状態反映
    setLED(LED_state[0], LED_state[1], LED_state[2]);

    if (millis() - lastSend >= 100) {  // 約10Hz
        sendData();
        lastSend = millis();
    }

    if(status != STATUS_STOP) {
        if (status == STATUS_FORWARD) {
            analogWrite(PIN_MOTOR_L_1, FORWARD_DUTY);
            analogWrite(PIN_MOTOR_L_2, 0);
            analogWrite(PIN_MOTOR_R_1, FORWARD_DUTY);
            analogWrite(PIN_MOTOR_R_2, 0);
        } else if (status == STATUS_BACKWARD) {
            analogWrite(PIN_MOTOR_L_1, 0);
            analogWrite(PIN_MOTOR_L_2, BACKWARD_DUTY);
            analogWrite(PIN_MOTOR_R_1, 0);
            analogWrite(PIN_MOTOR_R_2, BACKWARD_DUTY);
        } else if (status == STATUS_TURN_RIGHT) {
            analogWrite(PIN_MOTOR_L_1, TURN_DUTY_FW1);
            analogWrite(PIN_MOTOR_L_2, TURN_DUTY_FW2);
            analogWrite(PIN_MOTOR_R_1, TURN_DUTY_BW1);
            analogWrite(PIN_MOTOR_R_2, TURN_DUTY_BW2);
        } else if (status == STATUS_TURN_LEFT) {
            analogWrite(PIN_MOTOR_L_1, TURN_DUTY_BW1);
            analogWrite(PIN_MOTOR_L_2, TURN_DUTY_BW2);
            analogWrite(PIN_MOTOR_R_1, TURN_DUTY_FW1);
            analogWrite(PIN_MOTOR_R_2, TURN_DUTY_FW2);
        }

        if(millis() > stopTime) {
            status = STATUS_STOP;
            GUI_log("停止");
            LED_warn();
        }
    } else {
        // 停止
        analogWrite(PIN_MOTOR_L_1, 0);
        analogWrite(PIN_MOTOR_L_2, 0);
        analogWrite(PIN_MOTOR_R_1, 0);
        analogWrite(PIN_MOTOR_R_2, 0);
    }
}
