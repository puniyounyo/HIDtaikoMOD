# HIDtaiko ver.1.2.4 (Pro Micro)

## 概要

このArduinoスケッチは、OLEDディスプレイを搭載した自作の太鼓コントローラーとして機能します。
Pro Microをベースとしており、以下の kasasiki3 氏の HIDtaiko プロジェクトの主要な機能を統合しています。

* **PC/SW切り替え方式 (Ver 1.2)**
* **遅延処理 (Ver 1.3)**
* **OLED表示と接続器単体での設定変更 (Ver 2.0)**

それぞれのモードで異なる入力方式で操作でき、設定モードではOLED画面を見ながらアナログセンサーの感度や各種遅延などのパラメータを調整し、EEPROMに保存できます。

**オリジナルのアイデア:** [kasasiki3](https://github.com/kasasiki3) さん

## 動作

### 通常モード (PC/SWモード)

- **アナログ入力 (A0, A1, A2, A3):** 太鼓の左右の面や中央部分の入力をアナログセンサーで検知し、設定された閾値を超えると対応するキーボード入力（PCモード）またはSwitchのボタン入力（SWモード）を送信します。Pro Microのアナログ入力ピンを使用します。
- **デジタル入力 (4, 5, 6, 7, 8, 9, 10):** その他の操作ボタンとして機能し、押されると設定されたキーボード入力（PCモード）またはSwitchのボタン入力（SWモード）を送信します。Pro MicroのデジタルI/Oピンを使用します。
- **モード切り替え:**
    - 起動時にPro Microのデジタルピン15の状態を読み取り、HIGHであればSWモード、LOWであればPCモードで起動します。
    - PCモード中にPro Microのデジタルピン14のボタンを押すと設定モードに移行します。SWモード中は設定モードへの移行はできません。
- **OLED表示:** 起動時やモード切り替え時、設定モード時に動作モードや設定項目、設定値を表示します。I2C通信で接続されたOLEDディスプレイに情報を表示します。

### 設定モード

- Pro Microのデジタルピン14に接続されたボタンを押すことでPCモードから設定モードに移行します。
- OLED画面に設定項目名と現在の設定値が表示されます。
- Pro Microのデジタルピン6 (LOWで減少) と 7 (LOWで増加) に接続されたボタンで、現在選択されている設定項目の値を変更できます。
- Pro Microのデジタルピン8 (LOWで決定) に接続されたボタンを押すと、次の設定項目に移動します。最後の項目で決定ボタンを押すと設定モードが終了し、現在の設定値がPro MicroのEEPROMに保存され、通常モードに戻ります。

## 回路図
 -Pro Micro

           +-----------+
    (A0) --| 14        |--> アナログセンサー (右)
    (A1) --| 15        |--> アナログセンサー (中央右)
    (A2) --| 16        |--> アナログセンサー (中央左)
    (A3) --| 17        |--> アナログセンサー (左)
    (D4) --| 4         |--> デジタル入力ボタン
    (D5) --| 5         |--> デジタル入力ボタン
    (D6) --| 6         |--> 設定値減少ボタン
    (D7) --| 7         |--> 設定値増加ボタン
    (D8) --| 8         |--> 設定決定ボタン
    (D9) --| 9         |--> デジタル入力ボタン
   (D10) --| 10        |--> デジタル入力ボタン
   (D14) --| 14        |--> 設定モード切り替えボタン (PCモード時のみ有効)
   (D15) --| 15        |--> SW/PCモード切り替え (起動時判定)
     GND --| GND       |
     VCC --| VCC       | (通常 5V)
           +-----------+

 -OLED (I2C接続)

 Pro Micro GND -- OLED GND
 Pro Micro VCC -- OLED VCC
 Pro Micro SDA -- OLED SDA (通常 2)
 Pro Micro SCL -- OLED SCL (通常 3)

## 使用方法

1. Arduino IDEにこのスケッチをコピー＆ペーストします。
2. 必要なライブラリ（SPI, Wire, Adafruit GFX Library, Adafruit SSD1306, EEPROM, NintendoSwitchControlLibrary, Keyboard）がインストールされていることを確認してください。
3. インストールされていない場合は、ライブラリマネージャーからインストールしてください。
4. Pro Microボードに適切な配線でアナログセンサー、デジタル入力ボタン、OLEDディスプレイを接続してください。
5. **Pro Microのピン配置を再度確認し、スケッチのピン定義と一致させてください。特にI2CのSDA/SCLピンはArduino Unoと異なる場合があるので注意が必要です。**
6. Pro MicroをPCに接続し、スケッチを書き込みます。
7. 起動時にPro Microのデジタルピン15の接続状態によってPCモードまたはSWモードで動作します。
8. PCモード中に設定を変更したい場合は、Pro Microのデジタルピン14に接続されたボタンを押して設定モードに移行し、OLEDの指示に従って設定値を変更してください。

## 設定

設定モードでは以下の項目を調整できます。

| 設定項目名 | 説明                                                                 | 範囲    |
|------------|----------------------------------------------------------------------|---------|
| SE0        | アナログ入力 A0 (右) の入力検知閾値                                     | 0 - 100 |
| SE1        | アナログ入力 A1 (中央右) の入力検知閾値                                 | 0 - 100 |
| SE2        | アナログ入力 A2 (中央左) の入力検知閾値                                 | 0 - 100 |
| SE3        | アナログ入力 A3 (左) の入力検知閾値                                     | 0 - 100 |
| PA         | アナログ入力の再入力防止遅延時間 (共通)                                | 0 - 100 |
| PB         | アナログ入力 A1, A2 の同時入力防止遅延時間                             | 0 - 100 |
| PC         | アナログ入力の全体的な再入力防止遅延時間                               | 0 - 100 |
| PD         | アナログ入力 A0, A3 の同時入力防止遅延時間                             | 0 - 100 |
| PE         | キーボード入力時の遅延時間 (PCモード時) / Switchボタン押下後の遅延時間 (SWモード時) | 0 - 100 |

| 操作 | 説明                                                                 | 
|------------|----------------------------------------------------------------------|
| D6        | 押す度に数値が-1                                     |
| D7        | 押す度に数値が+1                                 |
| D8        | 設定値の決定、次項目へ（PE値決定後はPCモードへ移行）                                 |


## 注意事項

- SWモードを使用する場合は、Nintendo SwitchがProコントローラーとして認識するように設定する必要があります。
- アナログセンサーの出力特性に合わせて、閾値を適切に調整してください。
- ボタンのチャタリング防止のために、適切な遅延時間が設定されていますが、必要に応じて `debounceDelay` の値を調整してください。
- Pro MicroのEEPROMへの書き込み回数には寿命があります。頻繁な設定変更は避けることを推奨します。
- **Pro Microのピン配置は、Arduino Unoと異なります。特にデジタルピンとアナログピンの対応、I2Cピンの位置を間違えないように配線してください。**

