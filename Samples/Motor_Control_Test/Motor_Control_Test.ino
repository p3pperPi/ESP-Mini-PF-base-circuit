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

// 速度を移動平均するための設定
#define SPEED_BUFFER_LENGTH 5
const int  SPEED_AVE_WEIGHT[SPEED_BUFFER_LENGTH] = {6, 5, 2, 2, 1}; // 重み付けの設定、総和がビットシフトで演算できる値にする
const int  SPEED_AVE_DIVISOR = 16; // 重みの総和

volatile  int64_t pulse_R[SPEED_BUFFER_LENGTH];
volatile  int64_t pulse_L[SPEED_BUFFER_LENGTH];
volatile  int32_t pulse_diff_R[SPEED_BUFFER_LENGTH];
volatile  int32_t pulse_diff_L[SPEED_BUFFER_LENGTH];
volatile  int32_t crnt_pulse_per_microsec_R = 0;
volatile  int32_t crnt_pulse_per_microsec_L = 0;
volatile uint16_t pulse_itr = 0;



volatile uint32_t time_diff = 0xFFFFFFFF;// 意図しない0割りを防ぐため大きな値を初期値とする
volatile uint64_t last_read_time = 0;



// カウンタオーバーフロー処理
#define COUNTER_SKIP_THRESHOLD    3000
#define COUNTER_MAX               5000
#define COUNTER_MIN              -5000

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
#define SPEED_CONTROL_TIMER_FREQ  100000
#define SPEED_CONTROL_TIMER_COUNT   1000

hw_timer_t * speed_control_timer = NULL;
volatile SemaphoreHandle_t speed_control_timer_semaphore;
portMUX_TYPE speed_control_timer_mux = portMUX_INITIALIZER_UNLOCKED;


// 速度制御用のタイマ割り込みハンドラ
void ARDUINO_ISR_ATTR speedTimerInterrupt(){
    portENTER_CRITICAL_ISR(&speed_control_timer_mux);

    // バッファイテレータのインクリメント
    pulse_itr = (pulse_itr + 1) % SPEED_BUFFER_LENGTH;

    //  カウンタの値取得
    time_diff = millis() - last_read_time;
    last_read_time += time_diff;
    pulse_diff_L[pulse_itr] = getDiff(PCNT_UNIT_0, 0);
    pulse_diff_R[pulse_itr] = getDiff(PCNT_UNIT_1, 1);

    pulse_L[pulse_itr] += pulse_diff_L[pulse_itr];
    pulse_R[pulse_itr] += pulse_diff_R[pulse_itr];

    // 平均速度の計算
    int32_t L_ave = 0, R_ave = 0;

    for(byte i = 0; i < SPEED_BUFFER_LENGTH; i++) {
        byte itr_buff =  (pulse_itr + i) % SPEED_BUFFER_LENGTH;
        L_ave += pulse_diff_L[itr_buff] * SPEED_AVE_WEIGHT[i];
        R_ave += pulse_diff_R[itr_buff] * SPEED_AVE_WEIGHT[i];
    }
    L_ave /= SPEED_AVE_DIVISOR;
    R_ave /= SPEED_AVE_DIVISOR;


    crnt_pulse_per_microsec_L = L_ave * 1000 / (int32_t)time_diff; 
    crnt_pulse_per_microsec_R = R_ave * 1000 / (int32_t)time_diff; 


    portEXIT_CRITICAL_ISR(&speed_control_timer_mux);
    xSemaphoreGiveFromISR(speed_control_timer_semaphore, NULL);
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


    pcnt_counter_pause(PCNT_UNIT_0); //カウンタ一時停止;
    pcnt_counter_pause(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_0); //カウンタ初期化;
    pcnt_counter_clear(PCNT_UNIT_1);
    Serial.begin(115200);
    pcnt_counter_resume(PCNT_UNIT_0);// カウント開始;
    pcnt_counter_resume(PCNT_UNIT_1);


    // タイマ割り込みのセットアップ;
    speed_control_timer_semaphore = xSemaphoreCreateBinary();
    speed_control_timer           = timerBegin(SPEED_CONTROL_TIMER_FREQ);

    timerAttachInterrupt(speed_control_timer  ,&speedTimerInterrupt  );
    timerAlarm(speed_control_timer  ,SPEED_CONTROL_TIMER_COUNT  ,true,0);

    //タイマ基準等間隔処理(print)の初期化
    nextPrint = millis() + PRINT_INTERVAL;
}


void loop(){
    if(nextPrint < millis()){{
        

        //  生値の表示
        // Serial.print(cnt_crnt[0]);
        // Serial.print(",");
        // Serial.print(cnt_crnt[1]);
        // Serial.print(",");
        // Serial.print(cnt_prv[0]);
        // Serial.print(",");
        // Serial.println(cnt_prv[1]);

        // 平均計算前のエンコーダ値と速度
        // Serial.print(pulse_L[pulse_itr]);
        // Serial.print(",");
        // Serial.print(pulse_R[pulse_itr]);
        // Serial.print(",");
        // Serial.print(pulse_diff_L[pulse_itr]);
        // Serial.print(",");
        // Serial.println(pulse_diff_R[pulse_itr]);


        // 平均計算後の速度
        Serial.print("ave p/ms L : ");
        Serial.print(crnt_pulse_per_microsec_L);
        Serial.print(",R");
        Serial.print(crnt_pulse_per_microsec_R);
        Serial.println();

    
    }
        nextPrint = millis() + PRINT_INTERVAL;
    }
}

