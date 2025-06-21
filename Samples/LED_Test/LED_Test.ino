#include "../mini_PF_pins.hpp"

void setLED(bool RED,bool YEL,bool GRN){
    digitalWrite(PIN_LED_RED,RED);
    digitalWrite(PIN_LED_YEL,YEL);
    digitalWrite(PIN_LED_GRN,GRN);
}

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
    delay(200);


}

void loop(){
    setLED(0,0,1);
    delay(1000);
    setLED(0,1,0);
    delay(1000);
    setLED(1,0,0);
    delay(1000);
    setLED(0,0,0);
    delay(1000);
}