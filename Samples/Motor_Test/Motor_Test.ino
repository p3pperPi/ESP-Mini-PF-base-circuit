//  HWに適したピン定義をインクルードする
#include "../mini_PF_pins.hpp"
// #include "../mini_PF_pins_S3.hpp"

void setup(){
}

byte duty = 0;
void loop(){
    // 左右正回転
    for(duty = 0; duty < 255; duty++){
        analogWrite(PIN_MOTOR_L_1,duty); // 左モータ
        analogWrite(PIN_MOTOR_L_2,   0);
        analogWrite(PIN_MOTOR_R_1,duty); // 右モータ
        analogWrite(PIN_MOTOR_R_2,   0);
        delay(10);
    }

    // ブレーキ
    analogWrite(PIN_MOTOR_L_1,255);
    analogWrite(PIN_MOTOR_L_2,255);
    analogWrite(PIN_MOTOR_R_1,255);
    analogWrite(PIN_MOTOR_R_2,255);


    // 左逆回転
    for(duty = 0; duty < 255; duty++){
        analogWrite(PIN_MOTOR_L_1,   0); // 左モータ
        analogWrite(PIN_MOTOR_L_2,duty);
        analogWrite(PIN_MOTOR_R_1,   0); // 右モータ
        analogWrite(PIN_MOTOR_R_2,   0);
        delay(10);
    }

    // ブレーキ
    analogWrite(PIN_MOTOR_L_1,255);
    analogWrite(PIN_MOTOR_L_2,255);
    analogWrite(PIN_MOTOR_R_1,255);
    analogWrite(PIN_MOTOR_R_2,255);


    // 右逆回転
    for(duty = 0; duty < 255; duty++){
        analogWrite(PIN_MOTOR_L_1,   0); // 左モータ
        analogWrite(PIN_MOTOR_L_2,   0);
        analogWrite(PIN_MOTOR_R_1,   0); // 右モータ
        analogWrite(PIN_MOTOR_R_2,duty);
        delay(10);
    }

    // ブレーキ
    analogWrite(PIN_MOTOR_L_1,255);
    analogWrite(PIN_MOTOR_L_2,255);
    analogWrite(PIN_MOTOR_R_1,255);
    analogWrite(PIN_MOTOR_R_2,255);
}