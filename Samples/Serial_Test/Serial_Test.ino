#include "../mini_PF_pins.hpp"

// テストにLEDを使用

void setLED(bool RED,bool YEL,bool GRN){
    digitalWrite(PIN_LED_RED,RED);
    digitalWrite(PIN_LED_YEL,YEL);
    digitalWrite(PIN_LED_GRN,GRN);
}

bool menu_flag = false;

void setup(){
    pinMode(PIN_LED_RED,OUTPUT);
    pinMode(PIN_LED_YEL,OUTPUT);
    pinMode(PIN_LED_GRN,OUTPUT);

    delay(100);

    // イニシャル点灯
    setLED(0,0,1);
    
    delay(200);
    setLED(0,1,0);
    delay(200);
    setLED(1,0,0);
    delay(200);
    setLED(0,0,0);
    delay(600);

    setLED(1,1,1);
    delay(200);
    setLED(0,0,0);
    delay(200);
    setLED(1,1,1);
    delay(200);
    setLED(0,0,0);
    delay(1500);

    setLED(1,1,1);
    Serial.begin(115200);
    Serial.println("start program.");
    delay(200);
    setLED(0,0,0);

    menu_flag = true;
}

void loop(){

    if(menu_flag){
        Serial.println("--------Serial Test--------");
        Serial.println("Type value 0-7 to set LED.");
        Serial.println("RED LED measn 0x04 bit.");
        Serial.println("YEL LED measn 0x02 bit.");
        Serial.println("GRN LED measn 0x01 bit.");
        Serial.println("---------------------------");

        menu_flag = false;
    }

    if(Serial.available()){
        char c = Serial.read();
        if(c != '\r' && c != '\n'){
            if(c >= '0' && c <= '7'){
                int led = c - '0';
                setLED(led & 0x04,led & 0x02,led & 0x01);
                Serial.print("LED: ");
                Serial.println(led,DEC);
                menu_flag = true;
            }else{
                Serial.println("Invalid value.");
            }
            menu_flag = true;
        }

    }
}