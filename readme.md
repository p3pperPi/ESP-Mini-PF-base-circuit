# 概要
ESP32を使用して、DCブラシモータを使用したライントレースをするための基板です。

# 回路図

![回路図](/KiCAD/Moving_V4_ESP.svg)

# 機能
## 電源
[UZ1085L-22-TN3-R](https://www.lcsc.com/product-detail/Low-Dropout-Regulators-LDO_UZ1085L-AD-TN3-R_C146666.html)を使用し、マイコンへの電源供給を行います。
| 項目             | 値  | 単位 |
| ---------------- | --- | ---- |
| 最大定格入力電圧 | 18  | V    |
| 最大電流(Typ.)   | 4   | A |


## マイコン
ESP32-WROOM-32を使用します。  
[CH340X](https://www.lcsc.com/product-detail/USB-Converters_WCH-Jiangsu-Qin-Heng-CH340X_C3035748.html?s_z=n_CH340X)を使用した書き込み回路を持ちます。SW1を押下しながら電源を投入すると、書き込みモードになります。


## DCブラシモータ
モータドライバC[P2119LDTR](https://www.lcsc.com/product-detail/brushed-dc-motor-drivers_xblw-cp2119ldtr-xblw_C42395498.html)を使用して、DCモータを制御します。

| 項目                  | 値       | 単位       |
| --------------------- | -------- | ---------- |
| 最大定格電圧          | 20       | V          |
| 最大定格電流          | 9.0      | A          |
| 推奨電源電圧範囲      | 3.0~18.0 | V          |
| 推奨最大電流          | 5.0      | A          |
| FETオン抵抗 (load=1A) | 58       | m $\Omega$ |
| FETオン抵抗 (load=3A) | 71       | m $\Omega$ |

## フォトリフレクタ
[ITR8307/S17/TR8(B)](https://www.lcsc.com/product-detail/Reflective-Optical-Interrupters_Everlight-Elec-ITR8307-S17-TR8-B_C81632.html?s_z=n_ITR8307-S17) を3つ搭載し、テープの判定を行います。


## LED
3mm砲弾型LEDを4つ搭載可能です。  
D1は、電源投入時に点灯します。  
D2, D3, D4はESP32から点灯/消灯 を管理可能です。

# ピンアサイン

ESP32-WROOM-32のピンアサインは下記の通りです。

| ピン | IO pin   | 機能           | IN/OUT       |
| ---- | -------- | -------------- | ------------ |
| 27   | IO16     | モータL出力1   | OUT (PWM)    |
| 28   | IO17     | モータL出力2   | OUT (PWM)    |
| 30   | IO18     | モータR出力1   | OUT (PWM)    |
| 31   | IO19     | モータR出力2   | OUT (PWM)    |
| 34   | IO3/RxD0 | PCとのUART RxD | IN (COM.)    |
| 35   | IO1/TxD0 | PCとのUART TxD | OUT (COM.)   |
| 25   | IO0      | Boot           | -            |
| 9    | IO33     | ラインセンサ1  | IN(Analog)   |
| 6    | IO34     | ラインセンサ2  | IN(Analog)   |
| 8    | IO32     | ラインセンサ3  | IN(Analog)   |
| 33   | IO21     | LED 赤         | OUT(Digital) |
| 36   | IO22     | LED 黄         | OUT(Digital) |
| 37   | IO23     | LED 緑         | OUT(Digital) |

なお、ESP32-S3も使用可能。その場合のピンアサインは下記の通りです。

| ピン | IO pin | 機能           | IN/OUT       |
| ---- | ------ | -------------- | ------------ |
| 29   | IO36   | モータL出力1   | OUT (PWM)    |
| 30   | IO37   | モータL出力2   | OUT (PWM)    |
| 32   | IO39   | モータR出力1   | OUT (PWM)    |
| 33   | IO40   | モータR出力2   | OUT (PWM)    |
| 36   | RxD0   | PCとのUART RxD | IN (COM.)    |
| 37   | TxD0   | PCとのUART TxD | OUT (COM.)   |
| 27   | IO0    | Boot           | -            |
| 9    | IO16   | ラインセンサ1  | IN(Analog)   |
| 6    | IO6    | ラインセンサ2  | IN(Analog)   |
| 8    | IO15   | ラインセンサ3  | IN(Analog)   |
| 35   | IO42   | LED 赤         | OUT(Digital) |
| 38   | IO2    | LED 黄         | OUT(Digital) |
| 39   | IO1    | LED 緑         | OUT(Digital) |