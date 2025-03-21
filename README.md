# HIDtaikoMOD ver.1.2.5 (Pro Micro)

## 概要

このArduinoスケッチは、OLEDディスプレイを搭載した自作の太鼓コントローラーとして機能します。
Pro Microをベースとしており、以下の kasasiki3 氏の HIDtaiko プロジェクトの主要な機能を統合しています。

* **PC/SW切り替え方式 (Ver 1.2)**
* **遅延処理 (Ver 1.3)**
* **OLED表示と接続器単体での設定変更 (Ver 2.0)**

それぞれのモードで異なる入力方式で操作でき、設定モードではOLED画面を見ながらアナログセンサーの感度や各種遅延などのパラメータを調整し、EEPROMに保存できます。

**オリジナルのアイデア:** [kasasiki3](https://github.com/kasasiki3) さん

*更新履歴
- オリジナル版 1.2
- MOD版 1.2.1　ボツ
- MOD版 1.2.2　キーボードを離すまでの遅延処理追加（オリジナルver1.3相当)
- MOD版 1.2.3　EEPROMによる設定変更導入　スイッチモードNG
- MOD版 1.2.4　一旦完成
- MOD版 1.2.5　WS2812B LEDによる動作モード表示
- MOD版 1.2.6　大ドン、大カッの同時検出、出力対応　（テスト中） 
 
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



## ピン配置

| ピン番号 (Pro Micro) | シルク印刷  | SWモード用途  | PCモード用途  | 設定モード用途  |
|---|---|---|---|---|
| 17 | A0  | アナログセンサー (ZR) | アナログセンサー (kキー) | - (無効) |
| 18 | A1  | アナログセンサー (Rスティック) | アナログセンサー (jキー) | - (無効) |
| 19 | A2  | アナログセンサー (Lスティック) | アナログセンサー (fキー) | - (無効) |
| 20 | A3  | アナログセンサー (ZL) | アナログセンサー (dキー) | - (無効) |
| 7 | D4 | Rボタン | キーボード上矢印 | - (無効) |
| 8 | D5 | PLUSボタン | キーボードリターン | - (無効) |
| 9 | D6 | Aボタン | キーボードF1 | 設定値減少ボタン |
| 10 | D7 | Bボタン | キーボードINSERT | 設定値増加ボタン |
| 11 | D8 | MINUSボタン | キーボードESC | 設定決定ボタン |
| 12 | D9 | Lボタン | キーボード下矢印 | - (無効) |
| 13 | D10  | HOMEボタン |  - (無効) | - (無効) |
| 14 | D16  | LED用データ出力 | LED用データ出力 | LED用データ出力 |
| 15 | D14  |  - (無効) | 設定モード切り替え |  - (無効) |
| 16 | D15  | SW/PCモード切替 (起動時判定) | - (無効)| - (無効)|


## OLED (I2C接続)

| ピン番号 (Pro Micro) | シルク印刷 | OLED接続先 |
|---|---|---|
| 3/4/23 | GND | OLED GND |
| 21 | VCC | OLED VCC |
| 5 | 2 | OLED SDA |
| 6 | 3 | OLED SCL |


## 使用方法

1. Arduino IDEにこのスケッチをコピー＆ペーストします。
2. 必要なライブラリ（SPI, Wire, Adafruit GFX Library, Adafruit SSD1306, EEPROM, NintendoSwitchControlLibrary, Keyboard）がインストールされていることを確認してください。
3. インストールされていない場合は、ライブラリマネージャーからインストールしてください。
4. Pro Microボードに適切な配線でアナログセンサー、デジタル入力ボタン、OLEDディスプレイを接続してください。
5. Pro Microのピン配置を再度確認し、スケッチのピン定義と一致させてください。
6. Pro MicroをPCに接続し、スケッチを書き込みます。
7. 起動時にPro Microのデジタルピン15の接続状態によってPCモードまたはSWモードで動作します。
8. PCモード中に設定を変更したい場合は、Pro Microのデジタルピン14に接続されたボタンを押して設定モードに移行し、OLEDの指示に従って設定値を変更してください。

## 設定

設定モードでは以下の項目を調整できます。

| 設定項目名 | 説明                                                                 | 範囲    | 初期値   |
|------------|----------------------------------------------------------------------|---------|---------|
| SE0        | アナログ入力 A0 (右) の入力検知閾値                                     | 0 - 100 | 40 |
| SE1        | アナログ入力 A1 (中央右) の入力検知閾値                                 | 0 - 100 | 33 |
| SE2        | アナログ入力 A2 (中央左) の入力検知閾値                                 | 0 - 100 | 33 |
| SE3        | アナログ入力 A3 (左) の入力検知閾値                                     | 0 - 100 | 40 |
| PA         | アナログ入力の再入力防止遅延時間 (共通)                                | 0 - 100 | 16 |
| PB         | アナログ入力 A1, A2 の同時入力防止遅延時間                             | 0 - 100 | 10 |
| PC         | アナログ入力の全体的な再入力防止遅延時間                               | 0 - 100 | 30 |
| PD         | アナログ入力 A0, A3 の同時入力防止遅延時間                             | 0 - 100 | 26 |
| PE         | キーボード入力時の遅延時間 (PCモード時) / Switchボタン押下後の遅延時間 (SWモード時) | 0 - 100 | 18 |
| LRD        | 大カッの入力検知閾値 （試験中）                                                  | 0 - 100 | 5 |
| CRD        | 大ドンの入力検知閾値 （試験中）                                                  | 0 - 100 | 5 |
| DHT        | 大判定の許容範囲 （試験中）                                                  | 0 - 100 | 50 |

- 初回起動時は数値がデタラメなのでまずは初期値に設定の上調整してみてください。

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

