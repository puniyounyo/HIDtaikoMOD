/*
 * HIDtaikoMOD 1.2.9c 🥁
 * --------------------------------------------------
 * 【概要】
 * 太鼓アナログ入力を最優先しつつ、デジタルボタンの操作性を改善。
 * * 【デジタルボタン構成】
 * - 十字キー (Hat): Pin 5(上), 4(下), 10(左), 16(右)
 * - メインボタン  : Pin 7(A), 8(B), 6(X), 9(Y)
 * - システム拡張  : Pin 14(MINUS), 15(PLUS)
 * └ [14+15 同時押し] -> HOMEボタン (50ms猶予ロジック搭載)
 * * 【更新履歴】
 * - delayを撤廃、全ボタン押しっぱなし判定に対応。
 * - 判定の「滑り」を抑えるため、最後に一括 sendReport() を実施。
 * 
 * - 起動時　15PINを押したまま接続するとPCモード、押さなければSWモード
 * - PCモード時 14PINを押すと設定モード、感度調整が可能！
 *
 * --------------------------------------------------
 */

#include <SPI.h>                      // SPI通信ライブラリ
#include <Wire.h>                     // I2C通信ライブラリ
#include <Adafruit_GFX.h>             // Adafruit GFXライブラリ（グラフィック描画用基本ライブラリ）
#include <Adafruit_SSD1306.h>         // Adafruit SSD1306 OLEDライブラリ（特定のOLEDディスプレイを制御）
#include <EEPROM.h>                   // EEPROMライブラリ（マイコン内蔵の不揮発性メモリへの読み書き）
#include <NintendoSwitchControlLibrary.h> // Nintendo Switch制御ライブラリ（Nintendo Switchへの入力送信）
#include <Keyboard.h>                 // キーボード制御ライブラリ（PCへのキーボード入力送信）

#define SCREEN_WIDTH 128             // OLED画面の幅を128ピクセルと定義
#define SCREEN_HEIGHT 64              // OLED画面の高さを64ピクセルと定義
#define OLED_RESET -1                // OLEDリセットピンを-1と定義（リセットピンを使用しない場合）
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Adafruit_SSD1306クラスのオブジェクトdisplayを作成（OLED制御用）

const int numSettings = 9;            // 設定項目の数を9と定義
int settings[numSettings];            // 設定値を格納する整数型配列settingsを定義（要素数9）
int currentSetting = 0;               // 現在選択されている設定項目のインデックスを0で初期化
bool settingMode = false;             // 設定モードの状態を表すブール型変数settingModeをfalse（通常モード）で初期化
bool swswitching = false;             // SWモードの状態を表すブール型変数swswitchingをfalse（PCモード）で初期化

unsigned long lastDebounceTime = 0;   // 最後にボタンが押された時間を保存する変数
unsigned long debounceDelay = 50;     // ボタンのチャタリング防止のための遅延時間を50ミリ秒と定義

String settingNames[numSettings] = {"SE0", "SE1", "SE2", "SE3", "PA ", "PB ", "PC ", "PD ", "PE "}; // 設定項目の名前を格納する文字列型配列
String modeNames[3] = {"PC MODE", "SW MODE", "SET MODE"};                                          // 動作モードの名前を格納する文字列型配列（SET MODEを追加）

int de = 220;                         // デジタル入力の遅延時間を220ミリ秒と定義
const int A0pin = A0;                 // アナログ入力ピンA0を定数A0pinとして定義
const int A1pin = A1;                 // アナログ入力ピンA1を定数A1pinとして定義
const int A2pin = A2;                 // アナログ入力ピンA2を定数A2pinとして定義
const int A3pin = A3;                 // アナログ入力ピンA3を定数A3pinとして定義

char left = 'd';                      // キーボードの左ボタンに対応する文字を'd'と定義
char middleleft = 'f';                // キーボードの中央左ボタンに対応する文字を'f'と定義
char middletight = 'j';               // キーボードの中央右ボタンに対応する文字を'j'と定義
char right = 'k';                     // キーボードの右ボタンに対応する文字を'k'と定義

char aa = 17;                         // Switchモードの遅延時間を17と定義
char cc = 20;                         // Switchモードの遅延時間を20と定義
char swA = 1;                         // Switchモードの遅延時間を1と定義
char swB = 3;                         // Switchモードの遅延時間を3と定義

long int sv1 = 0;                     // アナログ入力ピンA1の以前の値を保存する変数
long int sv2 = 0;                     // アナログ入力ピンA2の以前の値を保存する変数
long int sv3 = 0;                     // アナログ入力ピンA3の以前の値を保存する変数
long int sv0 = 0;                     // アナログ入力ピンA0の以前の値を保存する変数
long int ti1 = 0;                     // アナログ入力ピンA1のタイマー
long int ti2 = 0;                     // アナログ入力ピンA2のタイマー
long int ti3 = 0;                     // アナログ入力ピンA3のタイマー
long int ti0 = 0;                     // アナログ入力ピンA0のタイマー
long int time = 0;                     // 現在の時間を保存する変数
long int timec = 0;                    // 汎用タイマー変数
long int ti = 0;                       // 汎用タイマー変数

unsigned long lastPressTime = 0; // ボタンが押された時刻
const int waitTime = 50;         // 判定の猶予時間 (50ミリ秒)

void setup() {
  // ピンモードを直接指定
  pinMode(0, INPUT_PULLUP);            // デジタル入力ピン0を入力プルアップモードに設定（通常HIGH）SW L
  pinMode(1, INPUT_PULLUP);            // デジタル入力ピン1を入力プルアップモードに設定（通常HIGH）SW R


  pinMode(4, INPUT_PULLUP);            // デジタル入力ピン4を入力プルアップモードに設定（通常HIGH）SW 下
  pinMode(5, INPUT_PULLUP);            // デジタル入力ピン5を入力プルアップモードに設定（通常HIGH）SW 上
  pinMode(6, INPUT_PULLUP);            // デジタル入力ピン6を入力プルアップモードに設定（通常HIGH）SW X
  pinMode(7, INPUT_PULLUP);            // デジタル入力ピン7を入力プルアップモードに設定（通常HIGH）SW A
  pinMode(8, INPUT_PULLUP);            // デジタル入力ピン8を入力プルアップモードに設定（通常HIGH）SW B
  pinMode(9, INPUT_PULLUP);            // デジタル入力ピン9を入力プルアップモードに設定（通常HIGH）SW Y

  pinMode(10, INPUT_PULLUP);           // デジタル入力ピン10を入力プルアップモードに設定（通常HIGH）SW 左
  pinMode(16, INPUT_PULLUP);           // デジタル入力ピン16を入力プルアップモードに設定（通常HIGH）SW 右
  pinMode(14, INPUT_PULLUP);           // デジタル入力ピン14を入力プルアップモードに設定（通常HIGH）SW MINUS　右設定モード切り替えボタンのピン14を入力プルアップモードに設定（通常HIGH）
  pinMode(15, INPUT_PULLUP);           // デジタル入力ピン15を入力プルアップモードに設定（通常HIGH）SW PLUS　右SWモード切り替えピン15を入力プルアップモードに設定（通常HIGH）
  
  if (digitalRead(15) == HIGH) {       // ピン15の状態を読み取り、HIGHの場合（SWモードが選択されている場合）
    swswitching = true;                 // SWモードの状態をtrueに設定
    SwitchControlLibrary();             // Nintendo Switch制御ライブラリを初期化
  }

  delay(1000);                          // 起動時に1000ミリ秒（1秒）の遅延
  Serial.begin(115200);                 // シリアル通信をボーレート115200で開始
  Serial.println("Starting...");         // シリアルモニタに"Starting..."と表示

  // OLED 初期化
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // OLEDディスプレイを初期化（電源供給方式とI2Cアドレスを指定）
  display.clearDisplay();                 // OLED画面の表示内容をクリア
  display.setTextSize(2);                 // 文字サイズを2倍に設定
  display.setTextColor(WHITE);             // 文字色を白に設定
  display.setCursor(0, 20);                 // カーソルの開始位置を(0, 20)に設定
  display.println("HIDtaiko");             // OLEDに"HIDtaiko"という文字列を表示
  display.setCursor(0, 40);                 // カーソルの開始位置を(0, 40)に設定
  display.println("ver.1.2.9c");           // OLEDに"ver.1.2.9b"という文字列を表示
  display.display();                      // OLED画面に設定した内容を実際に表示
  delay(2000);                          // 起動時に2000ミリ秒（2秒）の遅延

  if(swswitching == true){
    Serial.println("Switchモードを有効化します。"); // シリアルモニタに表示
    delay(de);                            // デジタル入力遅延時間だけ待つ
  } else {
    Serial.println("PCモードを有効化します。");   // シリアルモニタに表示
    delay(de);                            // デジタル入力遅延時間だけ待つ
  }

  loadSettings();                         // EEPROMから保存されている設定値を読み込む関数を呼び出す
  displayMode();                          // 現在の動作モードをOLEDに表示する関数を呼び出す

  if (swswitching == true) {             // SWモードが有効の場合
    for (int i = 0; i < 6; i++) {         // Lボタンを6回押す処理を繰り返す
      SwitchControlLibrary().pressButton(Button::LCLICK); // Nintendo SwitchのLスティッククリックボタンを押す
      SwitchControlLibrary().sendReport();                // 押したボタンの状態を送信
      delay(100);                                       // 100ミリ秒の遅延
      SwitchControlLibrary().releaseButton(Button::LCLICK); // Nintendo SwitchのLスティッククリックボタンを離す
      SwitchControlLibrary().sendReport();                  // 離したボタンの状態を送信
      delay(100);                                       // 100ミリ秒の遅延
    }
  }
}

void loop() {
  checkButtons();                         // ボタンの状態をチェックし、設定モードへの移行などを処理する関数を呼び出す
  if (settingMode) {                       // 設定モードがtrueの場合
    if (digitalRead(5) == LOW) {         // デジタル入力ピン5がLOWの場合（数値増加ボタンが押された場合）
      settings[currentSetting]++;         // 現在選択されている設定項目の値を1増やす
      Serial.println("Up Button Pressed"); // シリアルモニタに"Up Button Pressed"と表示
      delay(debounceDelay);                 // チャタリング防止のための遅延
      if (settings[currentSetting] < 0) { // 設定値が0未満になった場合
        settings[currentSetting] = 100;   // 設定値を100にリセット
      }
      displayValue();                     // 現在の設定値をOLEDに表示する関数を呼び出す
      delay(100);                         // 100ミリ秒の遅延
    }
    if (digitalRead(4) == LOW) {         // デジタル入力ピン4がLOWの場合（数値減少ボタンが押された場合）
      settings[currentSetting]--;         // 現在選択されている設定項目の値を1減らす
      Serial.println("Down Button Pressed");// シリアルモニタに"Down Button Pressed"と表示
      delay(debounceDelay);                 // チャタリング防止のための遅延
      if (settings[currentSetting] > 100) {// 設定値が100を超えた場合
        settings[currentSetting] = 0;     // 設定値を0にリセット
      }
      displayValue();                     // 現在の設定値をOLEDに表示する関数を呼び出す
      delay(100);                         // 100ミリ秒の遅延
    }
    if (digitalRead(7) == LOW) {         // デジタル入力ピン8がLOWの場合（決定ボタンが押された場合）
      currentSetting++;                   // 現在選択されている設定項目のインデックスを1増やす
      Serial.println("OK Button Pressed");
      delay(debounceDelay);
      if (currentSetting >= numSettings) {// 全ての設定項目を巡回した場合
        currentSetting = 0;               // 設定項目のインデックスを最初に戻す
        settingMode = false;              // 設定モードを終了する
        saveSettings();                   // 現在の設定値をEEPROMに保存する関数を呼び出す
        displayMode();                    // 通常モードの表示に戻す関数を呼び出す
        Serial.println("設定モードを終了します。");
      } else {
        displaySettings();                // 次の設定項目をOLEDに表示する関数を呼び出す
      }
      delay(200);                         // 200ミリ秒の遅延
    }
  } else {                               // 通常モード（設定モードがfalseの場合）
    long int a3 = analogRead(A3pin);     // アナログ入力ピンA3の値を読み取る
    long int a0 = analogRead(A0pin);     // アナログ入力ピンA0の値を読み取る
    long int a1 = analogRead(A1pin);     // アナログ入力ピンA1の値を読み取る
    long int a2 = analogRead(A2pin);     // アナログ入力ピンA2の値を読み取る
    time = millis();                     // 現在のミリ秒単位の時間を取得

    if (swswitching == false) {         // PCモードの場合
      if (a3 - sv3 >= settings[3] && time -ti3 > settings[4] && time - ti > settings[6]) { // アナログ入力A3の変化が閾値を超え、かつ一定時間経過した場合
        Keyboard.press(left);           // キーボードの左ボタンを押す
        delay(settings[8]);             // 設定された遅延時間待つ
        Keyboard.release(left);         // キーボードの左ボタンを離す
        ti3 = millis();                 // 最後にA3が反応した時間を更新
        ti = millis();                  // 最後に何らかの入力があった時間を更新
      }
      if (a0 - sv0 >= settings[0] && time - ti0 > settings[4] && time - ti > settings[6]) { // アナログ入力A0の変化が閾値を超え、かつ一定時間経過した場合
        Keyboard.press(right);          // キーボードの右ボタンを押す
        delay(settings[8]);             // 設定された遅延時間待つ
        Keyboard.release(right);        // キーボードの右ボタンを離す
        ti0 = millis();                 // 最後にA0が反応した時間を更新
        ti = millis();                  // 最後に何らかの入力があった時間を更新
      }
      if (a1 - sv1 >= settings[1] && time - ti1 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) { // アナログ入力A1の変化が閾値を超え、かつ複数の条件を満たす場合
        Keyboard.press(middletight);     // キーボードの中央右ボタンを押す
        delay(settings[8]);             // 設定された遅延時間待つ
        Keyboard.release(middletight);    // キーボードの中央右ボタンを離す
        ti1 = millis();                 // 最後にA1が反応した時間を更新
        ti = millis();                  // 最後に何らかの入力があった時間を更新
      }
      if (a2 - sv2 >= settings[2] && time - ti2 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) { // アナログ入力A2の変化が閾値を超え、かつ複数の条件を満たす場合
        Keyboard.press(middleleft);      // キーボードの中央左ボタンを押す
        delay(settings[8]);             // 設定された遅延時間待つ
        Keyboard.release(middleleft);     // キーボードの中央左ボタンを離す
        ti2 = millis();                 // 最後にA2が反応した時間を更新
        ti = millis();                  // 最後に何らかの入力があった時間を更新
      }
      if (digitalRead(4) == LOW) {       // デジタル入力ピン4がLOWの場合
        Keyboard.write(KEY_DOWN_ARROW);    // キーボードの下矢印キーを送信
        delay(de);                      // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(5) == LOW) {       // デジタル入力ピン5がLOWの場合
        Keyboard.write(KEY_UP_ARROW);  // キーボードの上矢印キーを送信
        delay(de);                      // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(6) == LOW) {       // デジタル入力ピン6がLOWの場合
        Keyboard.write(KEY_F1);          // キーボードのF1キーを送信
        delay(de);                      // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(7) == LOW) {       // デジタル入力ピン7がLOWの場合
        Keyboard.write(KEY_RETURN);      // キーボードのリターンキーを送信
        delay(de);                      // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(8) == LOW) {       // デジタル入力ピン8がLOWの場合
        Keyboard.write(KEY_ESC);      // キーボードのESCキーを送信
        delay(de);                      // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(9) == LOW) {       // デジタル入力ピン9がLOWの場合
        Keyboard.write(KEY_INSERT);         // キーボードのINSERTキーを送信   
        delay(de);                      // デジタル入力遅延時間だけ待つ
      }
} else {                               // SWモードの場合

// --- 1. デジタルボタンの状態を読み取り ---
      
      // ボタンの状態を一時保存する変数を定義（前回の状態と比較用）
      static uint32_t lastButtonState = 0;
      static uint8_t lastHatState = 8; // 8はNEUTRAL(離している状態)
      
      // 十字キー (Hat) の読み取りを数値(uint8_t)で行う
      uint8_t currentHat = 8; // NEUTRAL
      if      (digitalRead(5) == LOW)  currentHat = 0; // UP
      else if (digitalRead(4) == LOW)  currentHat = 4; // DOWN
      else if (digitalRead(10) == LOW) currentHat = 6; // LEFT
      else if (digitalRead(16) == LOW) currentHat = 2; // RIGHT
      SwitchControlLibrary().pressHatButton(currentHat);

      // メインボタン読み取り & 状態をビットにまとめる（変化検知用）
      uint32_t currentButtonState = 0;
      if (digitalRead(7) == LOW) { SwitchControlLibrary().pressButton(Button::A); currentButtonState |= 0x01; } else SwitchControlLibrary().releaseButton(Button::A);
      if (digitalRead(8) == LOW) { SwitchControlLibrary().pressButton(Button::B); currentButtonState |= 0x02; } else SwitchControlLibrary().releaseButton(Button::B);
      if (digitalRead(6) == LOW) { SwitchControlLibrary().pressButton(Button::X); currentButtonState |= 0x04; } else SwitchControlLibrary().releaseButton(Button::X);
      if (digitalRead(9) == LOW) { SwitchControlLibrary().pressButton(Button::Y); currentButtonState |= 0x08; } else SwitchControlLibrary().releaseButton(Button::Y);
      if (digitalRead(0) == LOW) { SwitchControlLibrary().pressButton(Button::L); currentButtonState |= 0x10; } else SwitchControlLibrary().releaseButton(Button::L);
      if (digitalRead(1) == LOW) { SwitchControlLibrary().pressButton(Button::R); currentButtonState |= 0x20; } else SwitchControlLibrary().releaseButton(Button::R);
      
      // システムボタン読み取り (Pin 14 & 15)
      bool p14 = (digitalRead(14) == LOW);
      bool p15 = (digitalRead(15) == LOW);


      // システムボタン合成ロジック
      static bool lastAnyPressed = false;
      if ((p14 || p15) && !lastAnyPressed) lastPressTime = millis();
      lastAnyPressed = (p14 || p15);

      if (p14 && p15) {
        SwitchControlLibrary().pressButton(Button::HOME);
        currentButtonState |= 0x100; // HOME押下状態をビット管理
      } else if ((p14 || p15) && (millis() - lastPressTime > waitTime)) {
        SwitchControlLibrary().releaseButton(Button::HOME);
        if (p14) { SwitchControlLibrary().pressButton(Button::MINUS); currentButtonState |= 0x40; } else SwitchControlLibrary().releaseButton(Button::MINUS);
        if (p15) { SwitchControlLibrary().pressButton(Button::PLUS);  currentButtonState |= 0x80; } else SwitchControlLibrary().releaseButton(Button::PLUS);
      } else {
        SwitchControlLibrary().releaseButton(Button::HOME);
        SwitchControlLibrary().releaseButton(Button::MINUS);
        SwitchControlLibrary().releaseButton(Button::PLUS);
      }

      // --- 2. 状態に変化があったら即送信 (デジタル操作用) ---
      if (currentButtonState != lastButtonState || currentHat != lastHatState) {
        SwitchControlLibrary().sendReport();
        lastButtonState = currentButtonState;
        lastHatState = currentHat;
      }
      
      // --- 3. 太鼓アナログ入力 (ここが最優先) ---
      
      // 判定の直前で「最新の時間」を取得！情報の鮮度を100%にします 📈
      time = millis();

      if (a3 - sv3 >= settings[3] && time - ti3 > swA && time - ti > swB) { // ZL (左ドン)
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti3 = millis(); ti = ti3; time = ti3; // ←ここでも時間をリフレッシュ！
      }
      
      if (a0 - sv0 >= settings[0] && time - ti0 > swA && time - ti > swB) { // ZR (右ドン)
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti0 = millis(); ti = ti0; time = ti0; 
      }
      
      if (a1 - sv1 >= settings[1] && time - ti1 > swA && time - ti > swB) { // RCLICK (右カッ)
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti1 = millis(); ti = ti1; time = ti1;
      }
      
      if (a2 - sv2 >= settings[2] && time - ti2 > swA && time - ti > swB) { // LCLICK (左カッ)
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti2 = millis(); ti = ti2; time = ti2;
      }

    }

    sv3 = a3;                             // 現在のA3アナログ入力値を以前の値として保存
    sv0 = a0;                             // 現在のA0アナログ入力値を以前の値として保存
    sv1 = a1;                             // 現在のA1アナログ入力値を以前の値として保存
    sv2 = a2;                             // 現在のA2アナログ入力値を以前の値として保存
  }
}

void loadSettings() {                     // EEPROMから設定値を読み込む関数
  for (int i = 0; i < numSettings; i++) { // 設定項目の数だけ繰り返す
    settings[i] = EEPROM.read(i);         // EEPROMのi番地から1バイト読み込み、settings配列のi番目に格納
  }
}

void saveSettings() {                     // EEPROMに設定値を保存する関数
  Serial.println("EEPROM Saved:");       // シリアルモニタに"EEPROM Saved:"と表示
  for (int i = 0; i < numSettings; i++) { // 設定項目の数だけ繰り返す
    int oldValue = EEPROM.read(i);       // EEPROMのi番地から以前の値を読み込む
    if (oldValue != settings[i]) {       // 現在の設定値と以前の値が異なる場合
      EEPROM.write(i, settings[i]);     // EEPROMのi番地に現在の設定値を書き込む
      Serial.print(settingNames[i]);     // シリアルモニタに設定項目名を表示
      Serial.print(":");                 // シリアルモニタに":"を表示
      Serial.print(oldValue);           // シリアルモニタに以前の値を表示
      Serial.print("->");               // シリアルモニタに"->"を表示
      Serial.println(settings[i]);     // シリアルモニタに現在の値を表示
    } else {                             // 設定値が変更されていない場合
      Serial.print(settingNames[i]);     // シリアルモニタに設定項目名を表示
      Serial.println(":No change");     // シリアルモニタに":No change"と表示
    }
  }
}
void displaySettings() {                   // 設定モードの画面を表示する関数
  display.clearDisplay();                 // OLED画面をクリア
  display.setTextSize(2);                 // 文字サイズを2倍に設定
  display.setTextColor(WHITE);             // 文字色を白に設定
  display.setCursor(0, 0);                 // カーソルの開始位置を(0, 0)に設定
  display.println("SET MODE");             // OLEDに"SET MODE"と表示
  display.setTextSize(2);                 // 文字サイズを2倍に設定
  display.setCursor(0, 24);                // カーソルの開始位置を(0, 24)に設定
  display.print(settingNames[currentSetting]); // 現在選択されている設定項目名を表示
  display.print(": ");                     // ":"を表示
  display.println(settings[currentSetting]);   // 現在の設定値を表示
  display.display();                      // OLED画面を更新
}

void displayValue() {                      // 数値部分のみを表示する関数
  display.setTextSize(2);                 // 文字サイズを2倍に設定
  display.setTextColor(WHITE);             // 文字色を白に設定

  // 数値部分を塗りつぶし (座標とサイズは調整が必要)
  display.fillRect(48, 24, 80, 24, BLACK); // 指定範囲を黒で塗りつぶす（以前の値を消すため）

  display.setCursor(0, 24);                // カーソルの開始位置を(0, 24)に設定
  display.print(settingNames[currentSetting]); // 現在選択されている設定項目名を表示
  display.print(": ");                     // ":"を表示
  display.println(settings[currentSetting]);   // 現在の設定値を表示
  display.display();                      // OLED画面を更新
}

void checkButtons() { // ボタンの状態を確認する
  if (digitalRead(14) == LOW && !swswitching && !settingMode) { // 設定モード切り替えボタンが押され、かつSWモードでなく、現在設定モードでない場合
    Serial.println("設定モードを有効化します。"); // シリアルモニタに表示
    settingMode = true;                     // 設定モードをtrueにする
    currentSetting = 0;                     // 設定項目を最初に戻す
    displaySettings();                      // 設定画面を表示する
//    updateLedColor(); // 設定モード開始時に LED を更新
    delay(debounceDelay);                 // チャタリング防止のための遅延
  }
  // 設定モード中は、14番ピンの入力はここでは処理しない
}

void displayMode() {                      // 現在の動作モードをOLEDに表示する関数
  display.clearDisplay();                 // OLED画面をクリア
  display.setTextSize(2);                 // 文字サイズを2倍に設定
  display.setTextColor(WHITE);             // 文字色を白に設定
  display.setCursor(0, 0);                 // カーソルの開始位置を(0, 0)に設定
  if (settingMode) {
    display.println(modeNames[2]);       // 設定モードの場合は"SET MODE"を表示
  } else {
    display.println(modeNames[swswitching]); // 通常モードの場合はPCモードまたはSWモードの名前を表示
  }
  display.display();                      // OLED画面を更新
}
