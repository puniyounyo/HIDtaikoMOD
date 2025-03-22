#include <Adafruit_NeoPixel.h> // WS2812B LED制御ライブラリ
#include <SPI.h>              // SPI通信ライブラリ
#include <Wire.h>             // I2C通信ライブラリ
#include <Adafruit_GFX.h>     // Adafruit GFXライブラリ（グラフィック描画用基本ライブラリ）
#include <Adafruit_SSD1306.h> // Adafruit SSD1306 OLEDライブラリ（特定のOLEDディスプレイを制御）
#include <EEPROM.h>           // EEPROMライブラリ（マイコン内蔵の不揮発性メモリへの読み書き）
#include <NintendoSwitchControlLibrary.h> // Nintendo Switch制御ライブラリ（Nintendo Switchへの入力送信）
#include <Keyboard.h>         // キーボード制御ライブラリ（PCへのキーボード入力送信）
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128   // OLED画面の幅を128ピクセルと定義
#define SCREEN_HEIGHT 64  // OLED画面の高さを64ピクセルと定義
#define OLED_RESET -1     // OLEDリセットピンを-1と定義（リセットピンを使用しない場合）
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Adafruit_SSD1306クラスのオブジェクトdisplayを作成（OLED制御用）

#define LED_PIN 16     // WS2812B のデータピン (Pro Micro D16)
#define LED_COUNT 1    // 使用する LED の数
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); // Adafruit_NeoPixelクラスのオブジェクトpixelsを作成（LED制御用）

const int numSettings = 12; // 設定項目の数を11と定義
int settings[numSettings]; // 設定値を格納する整数型配列settingsを定義（要素数11）
int currentSetting = 0;   // 現在選択されている設定項目のインデックスを0で初期化
bool settingMode = false; // 設定モードの状態を表すブール型変数settingModeをfalse（通常モード）で初期化
bool swswitching = false; // SWモードの状態を表すブール型変数swswitchingをfalse（PCモード）で初期化

unsigned long lastDebounceTime = 0; // 最後にボタンが押された時間を保存する変数
unsigned long debounceDelay = 50;  // ボタンのチャタリング防止のための遅延時間を50ミリ秒と定義

String settingNames[numSettings] = {"SE0", "SE1", "SE2", "SE3", "PA ", "PB ", "PC ", "PD ", "PE ", "LRD", "CRD", "DHT"}; // 設定項目の名前を格納する文字列型配列
// LRD: 左右同時入力遅延, CRD: 中央左右同時入力遅延 , HDT: 大判定の許容範囲
String modeNames[3] = {"PC MODE", "SW MODE", "SET MODE"}; // 動作モードの名前を格納する文字列型配列（SET MODEを追加）

int de = 220;         // デジタル入力の遅延時間を220ミリ秒と定義
const int A0pin = A0; // アナログ入力ピンA0を定数A0pinとして定義
const int A1pin = A1; // アナログ入力ピンA1を定数A1pinとして定義
const int A2pin = A2; // アナログ入力ピンA2を定数A2pinとして定義
const int A3pin = A3; // アナログ入力ピンA3を定数A3pinとして定義

char left = 'd';       // キーボードの左ボタンに対応する文字を'd'と定義
char middleleft = 'f'; // キーボードの中央左ボタンに対応する文字を'f'と定義
char middletight = 'j'; // キーボードの中央右ボタンに対応する文字を'j'と定義
char right = 'k';      // キーボードの右ボタンに対応する文字を'k'と定義

char aa = 17;  // Switchモードの遅延時間を17と定義
char cc = 20;  // Switchモードの遅延時間を20と定義
char swA = 1;  // Switchモードの遅延時間を1と定義
char swB = 3;  // Switchモードの遅延時間を3と定義

long int sv1 = 0;  // アナログ入力ピンA1の以前の値を保存する変数
long int sv2 = 0;  // アナログ入力ピンA2の以前の値を保存する変数
long int sv3 = 0;  // アナログ入力ピンA3の以前の値を保存する変数
long int sv0 = 0;  // アナログ入力ピンA0の以前の値を保存する変数
long int ti1 = 0;  // アナログ入力ピンA1のタイマー
long int ti2 = 0;  // アナログ入力ピンA2のタイマー
long int ti3 = 0;  // アナログ入力ピンA3のタイマー
long int ti0 = 0;  // アナログ入力ピンA0のタイマー
long int time = 0; // 現在の時間を保存する変数
long int timec = 0; // 汎用タイマー変数
long int ti = 0;   // 汎用タイマー変数

uint32_t currentLedColor = 0; // LED の状態を保持する変数

void setup() {
  // ピンモードを直接指定
  pinMode(4, INPUT_PULLUP);  // デジタル入力ピン4を入力プルアップモードに設定（通常HIGH）
  pinMode(5, INPUT_PULLUP);  // デジタル入力ピン5を入力プルアップモードに設定（通常HIGH）
  pinMode(6, INPUT_PULLUP);  // デジタル入力ピン6を入力プルアップモードに設定（通常HIGH）
  pinMode(7, INPUT_PULLUP);  // デジタル入力ピン7を入力プルアップモードに設定（通常HIGH）
  pinMode(8, INPUT_PULLUP);  // デジタル入力ピン8を入力プルアップモードに設定（通常HIGH）
  pinMode(9, INPUT_PULLUP);  // デジタル入力ピン9を入力プルアップモードに設定（通常HIGH）
  pinMode(10, INPUT_PULLUP); // デジタル入力ピン10を入力プルアップモードに設定（通常HIGH）
  pinMode(14, INPUT_PULLUP); // 設定モード切り替えボタンのピン14を入力プルアップモードに設定（通常HIGH）
  pinMode(15, INPUT_PULLUP); // SWモード切り替えピン15を入力プルアップモードに設定（通常HIGH）

  if (digitalRead(15) == HIGH) { // ピン15の状態を読み取り、HIGHの場合（SWモードが選択されている場合）
    swswitching = true;         // SWモードの状態をtrueに設定
    SwitchControlLibrary();     // Nintendo Switch制御ライブラリを初期化
  }

  delay(2000);              // 起動時に2000ミリ秒（2秒）の遅延
  Serial.begin(9600);      // シリアル通信をボーレート9600で開始
  Serial.println("Starting..."); // シリアルモニタに"Starting..."と表示

  loadSettings();

  // OLED 初期化
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // OLEDディスプレイを初期化（電源供給方式とI2Cアドレスを指定）
  display.clearDisplay();     // OLED画面の表示内容をクリア
  display.setTextSize(2);     // 文字サイズを2倍に設定
  display.setTextColor(WHITE); // 文字色を白に設定
  display.setCursor(0, 20);    // カーソルの開始位置を(0, 20)に設定
  display.println("HIDtaiko"); // OLEDに"HIDtaiko"という文字列を表示
  display.setCursor(0, 40);    // カーソルの開始位置を(0, 40)に設定
  display.println("ver.1.2.7"); // OLEDに"ver.1.2.7"という文字列を表示
  display.display();          // OLED画面に設定した内容を実際に表示

  pixels.begin(); // ネオピクセルLEDの初期化
  pixels.show();  // LEDを初期状態（消灯）にする

  if (swswitching == true) {
    Serial.println("Switchモードを有効化します。"); // シリアルモニタに表示
    delay(de);                                    // デジタル入力遅延時間だけ待つ
    currentLedColor = pixels.Color(0, 255, 0);    // 初期色を緑に設定
  } else {
   Serial.println("PCモードを有効化します。");     // シリアルモニタに表示
    delay(de);                                    // デジタル入力遅延時間だけ待つ
    currentLedColor = pixels.Color(0, 0, 255);     // 初期色を青に設定
  }

  // 起動時アニメーション：赤、青、緑 (高速切り替えループ、合計約2秒)
  blinkyLoop(200, 1000); // delayTime (ms), totalDuration (ms)
  delay(2000);           // 2000ミリ秒（2秒）の遅延

  pixels.fill(currentLedColor); // 起動時の初期色を点灯
  pixels.show();

  loadSettings(); // EEPROMから保存されている設定値を読み込む関数を呼び出す
  displayMode();  // 現在の動作モードをOLEDに表示する関数を呼び出す

  if (swswitching == true) { // SWモードが有効の場合
    for (int i = 0; i < 6; i++) { // Lボタンを6回押す処理を繰り返す
      SwitchControlLibrary().pressButton(Button::LCLICK); // Nintendo SwitchのLスティッククリックボタンを押す
      SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
      delay(100);                                         // 100ミリ秒の遅延
      SwitchControlLibrary().releaseButton(Button::LCLICK); // Nintendo SwitchのLスティッククリックボタンを離す
      SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
      delay(100);                                         // 100ミリ秒の遅延
    }
  }
}

void loop() {
  checkButtons();    // ボタンの状態をチェックし、設定モードへの移行などを処理する関数を呼び出す
  updateLedColor(); // モードが変わった場合のみ LED を更新

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input == "GET") {
      sendSettings();
    } else {
      updateSettings(input);
    }
  
  if (settingMode) { // 設定モードがtrueの場合
    if (digitalRead(6) == LOW) { // デジタル入力ピン6がLOWの場合（数値減少ボタンが押された場合）
      settings[currentSetting]--; // 現在選択されている設定項目の値を1減らす
      Serial.println("Up Button Pressed"); // シリアルモニタに"Up Button Pressed"と表示
      delay(debounceDelay);                // チャタリング防止のための遅延
      if (settings[currentSetting] < 0) { // 設定値が0未満になった場合
        settings[currentSetting] = 100;    // 設定値を100にリセット
      }
      displayValue(); // 現在の設定値をOLEDに表示する関数を呼び出す
      delay(100);     // 100ミリ秒の遅延
    }
    if (digitalRead(7) == LOW) { // デジタル入力ピン7がLOWの場合（数値増加ボタンが押された場合）
      settings[currentSetting]++; // 現在選択されている設定項目の値を1増やす
      Serial.println("Down Button Pressed"); // シリアルモニタに"Down Button Pressed"と表示
      delay(debounceDelay);                 // チャタリング防止のための遅延
      if (settings[currentSetting] > 100) { // 設定値が100を超えた場合
        settings[currentSetting] = 0;     // 設定値を0にリセット
      }
      displayValue(); // 現在の設定値をOLEDに表示する関数を呼び出す
      delay(100);     // 100ミリ秒の遅延
    }
    if (digitalRead(8) == LOW) { // デジタル入力ピン8がLOWの場合（決定ボタンが押された場合）
      currentSetting++;          // 現在選択されている設定項目のインデックスを1増やす
      Serial.println("OK Button Pressed");
      delay(debounceDelay);
      if (currentSetting >= numSettings) { // 全ての設定項目を巡回した場合
        currentSetting = 0;               // 設定項目のインデックスを最初に戻す
        settingMode = false;              // 設定モードを終了する
        saveSettings();                   // 現在の設定値をEEPROMに保存する関数を呼び出す
        displayMode();                    // 通常モードの表示に戻す関数を呼び出す
        Serial.println("設定モードを終了します。");
      } else {
        displaySettings(); // 次の設定項目をOLEDに表示する関数を呼び出す
      }
      delay(200); // 200ミリ秒の遅延
    }
  } else { // 通常モード（設定モードがfalseの場合）
    long int a3 = analogRead(A3pin); // アナログ入力ピンA3の値を読み取る
    long int a0 = analogRead(A0pin); // アナログ入力ピンA0の値を読み取る
    long int a1 = analogRead(A1pin); // アナログ入力ピンA1の値を読み取る
    long int a2 = analogRead(A2pin); // アナログ入力ピンA2の値を読み取る
    time = millis();                // 現在のミリ秒単位の時間を取得

    if (swswitching == false) { // PCモードの場合
      // 同時入力の検出と処理 (遅延時間を個別設定)
      if (a3 - sv3 >= settings[3] && a0 - sv0 >= settings[0] && abs(ti3 - ti0) <= settings[11]) {  // 左右同時入力
        Serial.println("大カッ!");
        Keyboard.press(left);
        Keyboard.press(right);
        delay(settings[9]); // 左右同時入力遅延 (settings[9]を使用)
        Keyboard.release(left);
        Keyboard.release(right);
        ti = millis();
      }

      if (a3 - sv3 >= settings[3] && time - ti3 > settings[4] && time - ti > settings[6]) { // アナログ入力A3の変化が閾値を超え、かつ一定時間経過した場合
        Serial.println("左カッ!");
        Keyboard.press(left);  // キーボードの左ボタンを押す
        delay(settings[8]);   // 設定された遅延時間待つ
        Keyboard.release(left); // キーボードの左ボタンを離す
        ti3 = millis();        // 最後にA3が反応した時間を更新
        ti = millis();         // 最後に何らかの入力があった時間を更新
      }
      if (a0 - sv0 >= settings[0] && time - ti0 > settings[4] && time - ti > settings[6]) { // アナログ入力A0の変化が閾値を超え、かつ一定時間経過した場合
        Serial.println("右カッ!");
        Keyboard.press(right); // キーボードの右ボタンを押す
        delay(settings[8]);   // 設定された遅延時間待つ
        Keyboard.release(right); // キーボードの右ボタンを離す
        ti0 = millis();        // 最後にA0が反応した時間を更新
        ti = millis();         // 最後に何らかの入力があった時間を更新
      }
      if (a1 - sv1 >= settings[1] && a2 - sv2 >= settings[2] && abs(ti1 - ti2) <= settings[11]) { // 中央左右同時入力
        Serial.println("大ドン!");
        Keyboard.press(middletight);
        Keyboard.press(middleleft);
        delay(settings[10]); // 中央左右同時入力遅延 (settings[10]を使用)
        Keyboard.release(middletight);
        Keyboard.release(middleleft);
        ti = millis();
      }
      if (a1 - sv1 >= settings[1] && time - ti1 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) { // アナログ入力A1の変化が閾値を超え、かつ複数の条件を満たす場合
        Serial.println("右ドン!");
        Keyboard.press(middletight); // キーボードの中央右ボタンを押す
        delay(settings[8]);        // 設定された遅延時間待つ
        Keyboard.release(middletight); // キーボードの中央右ボタンを離す
        ti1 = millis();            // 最後にA1が反応した時間を更新
        ti = millis();             // 最後に何らかの入力があった時間を更新
      }
      if (a2 - sv2 >= settings[2] && time - ti2 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) { // アナログ入力A2の変化が閾値を超え、かつ複数の条件を満たす場合
        Serial.println("左ドン!");
        Keyboard.press(middleleft); // キーボードの中央左ボタンを押す
        delay(settings[8]);        // 設定された遅延時間待つ
        Keyboard.release(middleleft); // キーボードの中央左ボタンを離す
        ti2 = millis();            // 最後にA2が反応した時間を更新
        ti = millis();             // 最後に何らかの入力があった時間を更新
      }
      if (digitalRead(4) == LOW) { // デジタル入力ピン4がLOWの場合
        Keyboard.write(KEY_UP_ARROW); // キーボードの上矢印キーを送信
        delay(de);                   // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(5) == LOW) { // デジタル入力ピン5がLOWの場合
        Keyboard.write(KEY_RETURN); // キーボードのリターンキーを送信
        delay(de);                  // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(6) == LOW) { // デジタル入力ピン6がLOWの場合
        Keyboard.write(KEY_F1);     // キーボードのF1キーを送信
        delay(de);                  // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(7) == LOW) { // デジタル入力ピン7がLOWの場合
        Keyboard.write(KEY_INSERT); // キーボードのINSERTキーを送信
        delay(de);                  // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(8) == LOW) { // デジタル入力ピン8がLOWの場合
        Keyboard.write(KEY_ESC);  // キーボードのESCキーを送信
        delay(de);                 // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(9) == LOW) { // デジタル入力ピン9がLOWの場合
        Keyboard.write(KEY_DOWN_ARROW); // キーボードの下矢印キーを送信
        delay(de);                     // デジタル入力遅延時間だけ待つ
      }


    } else { // SWモードの場合
      // 同時入力の検出と処理 (遅延時間を個別設定)
      if (a3 - sv3 >= settings[3] && a0 - sv0 >= settings[0] && abs(ti3 - ti0) <= settings[11]) {  // 左右同時入力
        Serial.println("大カッ!");
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(settings[9]); // 左右同時入力遅延 (settings[9]を使用)
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = millis();
      }
      if (a3 - sv3 >= settings[3] && time - ti3 > swA && time - ti > swB) { // アナログ入力A3の変化が閾値を超え、かつSwitchモードの遅延時間を満たす場合
        Serial.println("左カッ!");
        SwitchControlLibrary().pressButton(Button::ZL); // SwitchのZLボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(cc);                                         // Switchモードの遅延時間待つ
        SwitchControlLibrary().releaseButton(Button::ZL); // SwitchのZLボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(aa);                                         // Switchモードの遅延時間待つ
        ti3 = millis();                                    // 最後にA3が反応した時間を更新
        ti = millis();                                     // 最後に何らかの入力があった時間を更新
      }
      if (a0 - sv0 >= settings[0] && time - ti0 > swA && time - ti > swB) { // アナログ入力A0の変化が閾値を超え、かつSwitchモードの遅延時間を満たす場合
        Serial.println("右カッ!");
        SwitchControlLibrary().pressButton(Button::ZR); // SwitchのZRボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(cc);                                         // Switchモードの遅延時間待つ
        SwitchControlLibrary().releaseButton(Button::ZR); // SwitchのZRボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(aa);                                         // Switchモードの遅延時間待つ
        ti0 = millis();                                    // 最後にA0が反応した時間を更新
        ti = millis();                                     // 最後に何らかの入力があった時間を更新
      }
     if (a1 - sv1 >= settings[1] && a2 - sv2 >= settings[2] && abs(ti1 - ti2) <= settings[11]) { // 中央左右同時入力
        Serial.println("大ドン!");
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(settings[10]); // 中央左右同時入力遅延 (settings[10]を使用)
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = millis();
      }
      if (a1 - sv1 >= settings[1] && time - ti1 > swA && time - ti > swB) { // アナログ入力A1の変化が閾値を超え、かつSwitchモードの遅延時間を満たす場合
        Serial.println("右ドン!");
        SwitchControlLibrary().pressButton(Button::RCLICK); // SwitchのRスティッククリックボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(cc);                                         // Switchモードの遅延時間待つ
        SwitchControlLibrary().releaseButton(Button::RCLICK); // SwitchのRスティッククリックボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(aa);                                         // Switchモードの遅延時間待つ
        ti1 = millis();                                    // 最後にA1が反応した時間を更新
        ti = millis();                                     // 最後に何らかの入力があった時間を更新
      }
      if (a2 - sv2 >= settings[2] && time - ti2 > swA && time - ti > swB) { // アナログ入力A2の変化が閾値を超え、かつSwitchモードの遅延時間を満たす場合
        Serial.println("左ドン!");
        SwitchControlLibrary().pressButton(Button::LCLICK); // SwitchのLスティッククリックボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(cc);                                         // Switchモードの遅延時間待つ
        SwitchControlLibrary().releaseButton(Button::LCLICK); // SwitchのLスティッククリックボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(aa);                                         // Switchモードの遅延時間待つ
        ti2 = millis();                                    // 最後にA2が反応した時間を更新
        ti = millis();                                     // 最後に何らかの入力があった時間を更新
      }
      if (digitalRead(4) == LOW) { // デジタル入力ピン4がLOWの場合
        SwitchControlLibrary().pressButton(Button::R); // SwitchのRボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::R); // SwitchのRボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(5) == LOW) { // デジタル入力ピン5がLOWの場合
        SwitchControlLibrary().pressButton(Button::PLUS); // SwitchのPLUSボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::PLUS); // SwitchのPLUSボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(6) == LOW) { // デジタル入力ピン6がLOWの場合
        SwitchControlLibrary().pressButton(Button::A); // SwitchのAボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::A); // SwitchのAボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(7) == LOW) { // デジタル入力ピン7がLOWの場合
        SwitchControlLibrary().pressButton(Button::B); // SwitchのBボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::B); // SwitchのBボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(8) == LOW) { // デジタル入力ピン8がLOWの場合
        SwitchControlLibrary().pressButton(Button::MINUS); // SwitchのMINUSボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::MINUS); // SwitchのMINUSボタンを離す
        SwitchControlLibrary().sendReport();                 // レポートを送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(9) == LOW) { // デジタル入力ピン9がLOWの場合
        SwitchControlLibrary().pressButton(Button::L); // SwitchのLボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::L); // SwitchのLボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }
      if (digitalRead(10) == LOW) { // デジタル入力ピン10がLOWの場合
        SwitchControlLibrary().pressButton(Button::HOME); // SwitchのHOMEボタンを押す
        SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
        SwitchControlLibrary().releaseButton(Button::HOME); // SwitchのHOMEボタンを離す
        SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
        delay(de);                                         // デジタル入力遅延時間だけ待つ
      }


 
    }

    sv3 = a3; // 現在のA3アナログ入力値を以前の値として保存
    sv0 = a0; // 現在のA0アナログ入力値を以前の値として保存
    sv1 = a1; // 現在のA1アナログ入力値を以前の値として保存
    sv2 = a2; // 現在のA2アナログ入力値を以前の値として保存
  }
}

void loadSettings() { // EEPROMから設定値を読み込む関数
  for (int i = 0; i < numSettings; i++) { // 設定項目の数だけ繰り返す
    settings[i] = EEPROM.read(i); // EEPROMのi番地から1バイト読み込み、settings配列のi番目に格納
  }
}

void saveSettings() { // EEPROMに設定値を保存する関数
  Serial.println("EEPROM Saved:"); // シリアルモニタに"EEPROM Saved:"と表示
  for (int i = 0; i < numSettings; i++) { // 設定項目の数だけ繰り返す
    int oldValue = EEPROM.read(i); // EEPROMのi番地から以前の値を読み込む
    if (oldValue != settings[i]) { // 現在の設定値と以前の値が異なる場合
      EEPROM.write(i, settings[i]); // EEPROMのi番地に現在の設定値を書き込む
      Serial.print(settingNames[i]); // シリアルモニタに設定項目名を表示
      Serial.print(":"); // シリアルモニタに":"を表示
      Serial.print(oldValue); // シリアルモニタに以前の値を表示
      Serial.print("->"); // シリアルモニタに"->"を表示
      Serial.println(settings[i]); // シリアルモニタに現在の値を表示
    } else { // 設定値が変更されていない場合
      Serial.print(settingNames[i]); // シリアルモニタに設定項目名を表示
      Serial.println(":No change"); // シリアルモニタに":No change"と表示
    }
  }
}

void displaySettings() { // 設定モードの画面を表示する関数
  display.clearDisplay(); // OLED画面をクリア
  display.setTextSize(2); // 文字サイズを2倍に設定
  display.setTextColor(WHITE); // 文字色を白に設定
  display.setCursor(0, 0); // カーソルの開始位置を(0, 0)に設定
  display.println("SET MODE"); // OLEDに"SET MODE"と表示
  display.setTextSize(2); // 文字サイズを2倍に設定
  display.setCursor(0, 24); // カーソルの開始位置を(0, 24)に設定
  display.print(settingNames[currentSetting]); // 現在選択されている設定項目名を表示
  display.print(": "); // ":"を表示
  display.println(settings[currentSetting]); // 現在の設定値を表示
  display.display(); // OLED画面を更新
}

void displayValue() { // 数値部分のみを表示する関数
  display.setTextSize(2); // 文字サイズを2倍に設定
  display.setTextColor(WHITE); // 文字色を白に設定

  // 数値部分を塗りつぶし (座標とサイズは調整が必要)
  display.fillRect(48, 24, 80, 24, BLACK); // 指定範囲を黒で塗りつぶす（以前の値を消すため）

  display.setCursor(0, 24); // カーソルの開始位置を(0, 24)に設定
  display.print(settingNames[currentSetting]); // 現在選択されている設定項目名を表示
  display.print(": "); // ":"を表示
  display.println(settings[currentSetting]); // 現在の設定値を表示
  display.display(); // OLED画面を更新
}

void checkButtons() { // ボタンの状態を確認する
  if (digitalRead(14) == LOW && !swswitching && !settingMode) { // 設定モード切り替えボタンが押され、かつSWモードでなく、現在設定モードでない場合
    Serial.println("設定モードを有効化します。"); // シリアルモニタに表示
    settingMode = true; // 設定モードをtrueにする
    currentSetting = 0; // 設定項目を最初に戻す
    displaySettings(); // 設定画面を表示する
    updateLedColor(); // 設定モード開始時に LED を更新
    delay(debounceDelay); // チャタリング防止のための遅延
  }
  // 設定モード中は、14番ピンの入力はここでは処理しない
}

void displayMode() { // 現在の動作モードをOLEDに表示する関数
  display.clearDisplay(); // OLED画面をクリア
  display.setTextSize(2); // 文字サイズを2倍に設定
  display.setTextColor(WHITE); // 文字色を白に設定
  display.setCursor(0, 0); // カーソルの開始位置を(0, 0)に設定
  if (settingMode) {
    display.println(modeNames[2]); // 設定モードの場合は"SET MODE"を表示
  } else {
    display.println(modeNames[swswitching]); // 通常モードの場合はPCモードまたはSWモードの名前を表示
  }
  display.display(); // OLED画面を更新
}

void blinkyLoop(int delayTime, unsigned long totalDuration) { // LED点滅アニメーション関数
  unsigned long startTime = millis();
  while (millis() - startTime < totalDuration) { // 指定時間内ループ
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // LED0を赤色に設定
    pixels.setPixelColor(1, pixels.Color(255, 0, 0)); // LED1を赤色に設定
    pixels.show(); // LED表示
    delay(delayTime); // 指定時間待ち
    pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // LED0を青色に設定
    pixels.setPixelColor(1, pixels.Color(0, 0, 255)); // LED1を青色に設定
    pixels.show(); // LED表示
    delay(delayTime); // 指定時間待ち
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // LED0を緑色に設定
    pixels.setPixelColor(1, pixels.Color(0, 255, 0)); // LED1を緑色に設定
    pixels.show(); // LED表示
    delay(delayTime); // 指定時間待ち
  }
  pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // LED0を消灯
  pixels.setPixelColor(1, pixels.Color(0, 0, 0)); // LED1を消灯
  pixels.show(); // LED表示
}

void updateLedColor() { // LEDの色を更新する関数
  uint32_t newColor;
  if (settingMode) {
    newColor = pixels.Color(255, 0, 0); // 赤
  } else if (swswitching) {
    newColor = pixels.Color(0, 255, 0); // 緑
  } else {
    newColor = pixels.Color(0, 0, 255); // 青
  }

  if (newColor != currentLedColor) { // LEDの色が変更された場合
    pixels.fill(newColor); // LEDを新しい色で塗りつぶす
    pixels.show(); // LED表示
    currentLedColor = newColor; // 現在の色を更新
  }
}

void loadSettings() {
  for (int i = 0; i < numSettings; i++) {
    settings[i] = EEPROM.read(i);
  }
}

void saveSettings() {
  for (int i = 0; i < numSettings; i++) {
    EEPROM.write(i, settings[i]);
  }
  EEPROM.commit();
}

void sendSettings() {
  StaticJsonDocument<256> doc;
  for (int i = 0; i < numSettings; i++) {
    doc[settingNames[i]] = settings[i];
  }
  String json;
  serializeJson(doc, json);
  Serial.println(json);
}

void updateSettings(String jsonInput) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonInput);
  if (error) {
    Serial.println("JSON Error");
    return;
  }

  for (int i = 0; i < numSettings; i++) {
    if (doc.containsKey(settingNames[i])) {
      settings[i] = doc[settingNames[i]];
    }
  }
  saveSettings();
  Serial.println("OK");
}
