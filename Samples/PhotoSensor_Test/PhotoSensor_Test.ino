//  HWに適したピン定義をインクルードする
#include "../mini_PF_pins.hpp"
// #include "../mini_PF_pins_S3.hpp"

// テストにシリアル通信を使用

void setup(){
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("start program.");
    delay(100);
}

unsigned int sensor_value[3];
char buffer[8];

void loop(){
    sensor_value[0] = analogRead(PIN_LINE_SENSOR_1);
    sensor_value[1] = analogRead(PIN_LINE_SENSOR_2);
    sensor_value[2] = analogRead(PIN_LINE_SENSOR_3);



    Serial.print("Sensor Data :");
    for(int i = 0;i < 3;i++){
        sprintf(buffer,"%05d",sensor_value[i]);
        Serial.print(buffer);
        Serial.print(",");
    }

    Serial.println();

    delay(10);
}