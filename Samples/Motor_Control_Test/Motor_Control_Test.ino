#include "../mini_PF_pins.hpp"
#include "driver/pcnt.h"

// pcnt : ESP32 の パルスカウンタ
// ライブラリの詳細：
// https://github.com/m5stack/azure_iothub_arduino_lib_esp32/blob/master/hardware/espressif/esp32/tools/sdk/include/driver/driver/pcnt.h
// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/peripherals/pcnt.html

// タイマ割り込み：Arduino-ESP32 の TImer
// 機能の詳細
// https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html

// エンコーダ周りの定義
volatile int64_t pulse_R = 0;
volatile int64_t pulse_L = 0;
volatile int32_t pulse_diff_R = 0;
volatile int32_t pulse_diff_L = 0;

volatile uint32_t time_diff = 0xFFFFFFFF;// 意図しない0割りを防ぐため大きな値を初期値とする
volatile uint64_t last_read_time = 0;



// カウンタオーバーフロー処理
#define COUNTER_SKIP_THRESHOLD    3000
// #define COUNTER_MAX_TO_MIN       10000
#define COUNTER_MAX               5000
#define COUNTER_MIN              -5000
// #define COUNTER_MAX_TO_MIN       65535
// #define COUNTER_MAX              32767
// #define COUNTER_MIN             -32768

int16_t cnt_crnt[8] = {0, 0, 0, 0, 0, 0, 0, 0}; 
int16_t cnt_prv [8] = {0, 0, 0, 0, 0, 0, 0, 0}; 

int32_t getDiff(pcnt_unit_t unit,byte cnt_no = 0){
    // カウンタの値を読み取る
    pcnt_get_counter_value(unit, &cnt_crnt[cnt_no]);
    int32_t cnt_diff = cnt_crnt[cnt_no] - cnt_prv[cnt_no];
    cnt_prv[cnt_no] = cnt_crnt[cnt_no];

    // オーバーフローの処理
    // カウンタはオーバーフローすると0に戻る
    if (cnt_diff > COUNTER_SKIP_THRESHOLD) {
        // カウンタが最大値から0に戻った場合
        cnt_diff += COUNTER_MAX;
    } else if (cnt_diff < -COUNTER_SKIP_THRESHOLD) {
        // カウンタが最小値から0に戻った場合
        cnt_diff += COUNTER_MIN;
    }

    return cnt_diff;
}


// タイマ割り込み周りの定義
// エンコーダ速度計測
#define SPEED_TIMER_FREQ  100000
#define SPEED_TIMER_COUNT   1000

hw_timer_t * speed_timer = NULL;
volatile SemaphoreHandle_t speed_timer_semaphore;
portMUX_TYPE speed_timer_mux = portMUX_INITIALIZER_UNLOCKED;

// 速度制御
#define CONTROL_TIMER_FREQ  10000
#define CONTROL_TIMER_COUNT  1000
hw_timer_t * control_timer = NULL;
volatile SemaphoreHandle_t control_timer_semaphore;
portMUX_TYPE control_timer_mux = portMUX_INITIALIZER_UNLOCKED;
volatile bool exec_control = false;


// 速度計測用のタイマ割り込みハンドラ
void ARDUINO_ISR_ATTR speedTimerInterrupt(){
    portENTER_CRITICAL_ISR(&speed_timer_mux);

    time_diff = millis() - last_read_time;
    last_read_time += time_diff;
    pulse_diff_L = getDiff(PCNT_UNIT_0, 0);
    pulse_diff_R = getDiff(PCNT_UNIT_1, 1);

    pulse_L += pulse_diff_L;
    pulse_R += pulse_diff_R;

    portEXIT_CRITICAL_ISR(&speed_timer_mux);
    xSemaphoreGiveFromISR(speed_timer_semaphore, NULL);
}

// 速度制御用のタイマ割り込みハンドラ
void ARDUINO_ISR_ATTR controlTimerInterrupt(){
    portENTER_CRITICAL_ISR(&control_timer_mux);

    exec_control = true;

    portEXIT_CRITICAL_ISR(&control_timer_mux);
    xSemaphoreGiveFromISR(control_timer_semaphore, NULL);
}



// 通信周り
char serialBuff[64];
long nextPrint = 0;
#define PRINT_INTERVAL 200



void setup(){
    // シリアル通孫
    Serial.begin(115200);
    while (!Serial) {
        // シリアルポートが開くのを待つ
    }


    // エンコーダの設定
    pcnt_config_t pcnt_config;

    // encorder L

    // L-1 : channel 0 
    pcnt_config.pulse_gpio_num = PIN_MOTOR_L_ENC_A;
    pcnt_config.ctrl_gpio_num  = PIN_MOTOR_L_ENC_B;
    pcnt_config.lctrl_mode     = PCNT_MODE_REVERSE; // ctrl_gpioが lowの時のカウンタ増減
    pcnt_config.hctrl_mode     = PCNT_MODE_KEEP;    // ctrl_gpioがhighの時のカウンタ増減
    pcnt_config.pos_mode       = PCNT_COUNT_INC;    // 立ち上がりエッジの挙動
    pcnt_config.neg_mode       = PCNT_COUNT_DEC;    // 立ち下がりエッジの挙動
    pcnt_config.counter_h_lim  = COUNTER_MAX;       // カウンタの最大値
    pcnt_config.counter_l_lim  = COUNTER_MIN;       // カウンタの最小値
    pcnt_config.unit           = PCNT_UNIT_0;       //
    pcnt_config.channel        = PCNT_CHANNEL_0;    //
    pcnt_unit_config(&pcnt_config);//ユニット初期化

    //L-2 ;  channel 1
    pcnt_config.pulse_gpio_num = PIN_MOTOR_L_ENC_B;
    pcnt_config.ctrl_gpio_num  = PIN_MOTOR_L_ENC_A;
    pcnt_config.lctrl_mode     = PCNT_MODE_KEEP;    // ctrl_gpioが lowの時のカウンタ増減
    pcnt_config.hctrl_mode     = PCNT_MODE_REVERSE; // ctrl_gpioがhighの時のカウンタ増減
    pcnt_config.pos_mode       = PCNT_COUNT_INC;    // 立ち上がりエッジの挙動
    pcnt_config.neg_mode       = PCNT_COUNT_DEC;    // 立ち下がりエッジの挙動
    pcnt_config.counter_h_lim  = COUNTER_MAX;       // カウンタの最大値
    pcnt_config.counter_l_lim  = COUNTER_MIN;       // カウンタの最小値
    pcnt_config.unit           = PCNT_UNIT_0;       //
    pcnt_config.channel        = PCNT_CHANNEL_1;    //
    pcnt_unit_config(&pcnt_config);//ユニット初期化

    // encorder R

    // R-1 : channel 20
    pcnt_config.pulse_gpio_num = PIN_MOTOR_R_ENC_A;
    pcnt_config.ctrl_gpio_num  = PIN_MOTOR_R_ENC_B;
    pcnt_config.lctrl_mode     = PCNT_MODE_REVERSE; // ctrl_gpioが lowの時のカウンタ増減
    pcnt_config.hctrl_mode     = PCNT_MODE_KEEP;    // ctrl_gpioがhighの時のカウンタ増減
    pcnt_config.pos_mode       = PCNT_COUNT_INC;    // 立ち上がりエッジの挙動
    pcnt_config.neg_mode       = PCNT_COUNT_DEC;    // 立ち下がりエッジの挙動
    pcnt_config.counter_h_lim  = COUNTER_MAX;       // カウンタの最大値
    pcnt_config.counter_l_lim  = COUNTER_MIN;       // カウンタの最小値
    pcnt_config.unit           = PCNT_UNIT_1;       //
    pcnt_config.channel        = PCNT_CHANNEL_0;    //
    pcnt_unit_config(&pcnt_config);//ユニット初期化

    //R-2 ;  channel 1
    pcnt_config.pulse_gpio_num = PIN_MOTOR_R_ENC_B;
    pcnt_config.ctrl_gpio_num  = PIN_MOTOR_R_ENC_A;
    pcnt_config.lctrl_mode     = PCNT_MODE_KEEP;    // ctrl_gpioが lowの時のカウンタ増減;
    pcnt_config.hctrl_mode     = PCNT_MODE_REVERSE; // ctrl_gpioがhighの時のカウンタ増減;
    pcnt_config.pos_mode       = PCNT_COUNT_INC;    // 立ち上がりエッジの挙動;
    pcnt_config.neg_mode       = PCNT_COUNT_DEC;    // 立ち下がりエッジの挙動;
    pcnt_config.counter_h_lim  = COUNTER_MAX;       // カウンタの最大値;
    pcnt_config.counter_l_lim  = COUNTER_MIN;       // カウンタの最小値;
    pcnt_config.unit           = PCNT_UNIT_1;       //;
    pcnt_config.channel        = PCNT_CHANNEL_1;    //;
    pcnt_unit_config(&pcnt_config);//ユニット初期化;
;
;
    pcnt_counter_pause(PCNT_UNIT_0); //カウンタ一時停止;
    pcnt_counter_pause(PCNT_UNIT_1);;
    pcnt_counter_clear(PCNT_UNIT_0); //カウンタ初期化;
    pcnt_counter_clear(PCNT_UNIT_1);;
    Serial.begin(115200);;
    pcnt_counter_resume(PCNT_UNIT_0);// カウント開始;
    pcnt_counter_resume(PCNT_UNIT_1);;
;
;
    // タイマ割り込みのセットアップ;
    speed_timer_semaphore   = xSemaphoreCreateBinary();;
    control_timer_semaphore = xSemaphoreCreateBinary();;
;
    speed_timer   = timerBegin(SPEED_TIMER_FREQ);;
    control_timer = timerBegin(CONTROL_TIMER_FREQ);;
;
;
    timerAttachInterrupt(speed_timer  ,&speedTimerInterrupt  );;
    timerAttachInterrupt(control_timer,&controlTimerInterrupt);;
;
    timerAlarm(speed_timer  ,SPEED_TIMER_COUNT  ,true,0);
    timerAlarm(control_timer,CONTROL_TIMER_COUNT,true,0);

    //タイマ基準等間隔処理(print)の初期化
    nextPrint = millis() + PRINT_INTERVAL;
}


void loop(){
    if(nextPrint < millis()){{
        
        
        sprintf(serialBuff,"L:%9ld ",pulse_L);
        Serial.print(serialBuff);
        sprintf(serialBuff,"R:%9ld ",pulse_R);
        Serial.print(serialBuff);
        sprintf(serialBuff,"/ SPEED L:%5d R:%5d ",pulse_diff_L,pulse_diff_R);
        Serial.print(serialBuff);
        Serial.println();

        // Serial.print(pulse_L);
        // Serial.print(",");
        // Serial.print(pulse_R);
        // Serial.print(",");
        // Serial.print(pulse_diff_L);
        // Serial.print(",");
        // Serial.println(pulse_diff_R);


        // Serial.print(cnt_crnt[0]);
        // Serial.print(",");
        // Serial.print(cnt_crnt[1]);
        // Serial.print(",");
        // Serial.print(cnt_prv[0]);
        // Serial.print(",");
        // Serial.println(cnt_prv[1]);
    
    }
        nextPrint = millis() + PRINT_INTERVAL;
    }
}

