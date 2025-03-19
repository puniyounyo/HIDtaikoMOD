#include <Adafruit_NeoPixel.h>
#include <SPI.h>                                      // SPI通信ライブラリ
#include <Wire.h>                                     // I2C通信ライブラリ
#include <Adafruit_GFX.h>                             // Adafruit GFXライブラリ（グラフィック描画用基本ライブラリ）
#include <Adafruit_SSD1306.h>                         // Adafruit SSD1306 OLEDライブラリ（特定のOLEDディスプレイを制御）
#include <EEPROM.h>                                   // EEPROMライブラリ（マイコン内蔵の不揮発性メモリへの読み書き）
#include <NintendoSwitchControlLibrary.h>             // Nintendo Switch制御ライブラリ（Nintendo Switchへの入力送信）
#include <Keyboard.h>                                 // キーボード制御ライブラリ（PCへのキーボード入力送信）

#define SCREEN_WIDTH 128                               // OLED画面の幅を128ピクセルと定義
#define SCREEN_HEIGHT 64                              // OLED画面の高さを64ピクセルと定義
#define OLED_RESET -1                                // OLEDリセットピンを-1と定義（リセットピンを使用しない場合）
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Adafruit_SSD1306クラスのオブジェクトdisplayを作成（OLED制御用）

#define LED_PIN     16                                 // WS2812B のデータピン (Pro Micro D16)
#define LED_COUNT   2                                  // 使用する LED の数
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const int numSettings = 9;                            // 設定項目の数を9と定義
int settings[numSettings];                            // 設定値を格納する整数型配列settingsを定義（要素数9）
int currentSetting = 0;                             // 現在選択されている設定項目のインデックスを0で初期化
bool settingMode = false;                           // 設定モードの状態を表すブール型変数settingModeをfalse（通常モード）で初期化
bool swswitching = false;                           // SWモードの状態を表すブール型変数swswitchingをfalse（PCモード）で初期化

unsigned long lastDebounceTime = 0;                  // 最後にボタンが押された時間を保存する変数（チャタリング防止用）
unsigned long debounceDelay = 50;                    // ボタンのチャタリング防止のための遅延時間を50ミリ秒と定義

String settingNames[numSettings] = {"SE0", "SE1", "SE2", "SE3", "PA ", "PB ", "PC ", "PD ", "PE "}; // 設定項目の名前を格納する文字列型配列
String modeNames[3] = {"PC MODE", "SW MODE", "SET MODE"}; // 動作モードの名前を格納する文字列型配列（SET MODEを追加）

int de = 5;                                         // デジタル入力の遅延時間を短縮 (同時押し判定への影響を減らすため)
const int A0pin = A0;                               // アナログ入力ピンA0を定数A0pinとして定義 (右ドン)
const int A1pin = A1;                               // アナログ入力ピンA1を定数A1pinとして定義 (右カッ - 仮)
const int A2pin = A2;                               // アナログ入力ピンA2を定数A2pinとして定義 (左カッ - 仮)
const int A3pin = A3;                               // アナログ入力ピンA3を定数A3pinとして定義 (左ドン)

char leftDonKey = 'd';                              // PCモード：キーボードの左ドンのキー
char rightDonKey = 'k';                             // PCモード：キーボードの右ドンのキー
char leftKapKey = 'f';                              // PCモード：キーボードの左カッのキー
char rightKapKey = 'j';                             // PCモード：キーボードの右カッのキー
// char bigDonKey = ' ';                               // PCモード：キーボードの大ドンのキー (定義不要)
// char bigKapKey = 'v';                               // PCモード：キーボードの大カッのキー (定義不要)

char aa = 17;                                       // Switchモードのボタンを離す際の遅延 (ms)
char cc = 20;                                       // Switchモードのボタンを押す際の遅延 (ms)
char swA = 1;                                        // Switchモードのアナログ入力の再入力防止遅延 (ms)
char swB = 3;                                        // Switchモードの同時入力防止遅延 (ms)
Button swLeftDonButton = Button::ZL;                // Switchモードの左ドンに割り当てるボタン (例：ZLボタン)
Button swRightDonButton = Button::ZR;               // Switchモードの右ドンに割り当てるボタン (例：ZRボタン)
Button swLeftKapButton = Button::LCLICK;            // Switchモードの左カツに割り当てるボタン (例：Lスティッククリック)
Button swRightKapButton = Button::RCLICK;           // Switchモードの右カツに割り当てるボタン (例：Rスティッククリック)

long int sv1 = 0;                                    // アナログ入力ピンA1の以前の値を保存する変数 (右カッ)
long int sv2 = 0;                                    // アナログ入力ピンA2の以前の値を保存する変数 (左カッ)
long int sv3 = 0;                                    // アナログ入力ピンA3の以前の値を保存する変数 (左ドン)
long int sv0 = 0;                                    // アナログ入力ピンA0の以前の値を保存する変数 (右ドン)
long int svKapR = 0;                                 // アナログ入力ピンA1 (右カッ) の以前の値を保存する変数
long int svKapL = 0;                                 // アナログ入力ピンA2 (左カッ) の以前の値を保存する変数

long int ti1 = 0;                                    // アナログ入力ピンA1のタイマー (右カッ - 最後に反応した時間)
long int ti2 = 0;                                    // アナログ入力ピンA2のタイマー (左カッ - 最後に反応した時間)
long int ti3 = 0;                                    // アナログ入力ピンA3のタイマー (左ドン - 最後に反応した時間)
long int ti0 = 0;                                    // アナログ入力ピンA0のタイマー (右ドン - 最後に反応した時間)
long int tiKapR = 0;                                 // アナログ入力ピンA1 (右カッ) のタイマー (最後に反応した時間)
long int tiKapL = 0;                                 // アナログ入力ピンA2 (左カッ) のタイマー (最後に反応した時間)
long int ti = 0;                                     // 汎用タイマー変数 (最後に何らかの入力があった時間)
long int timec = 0;                                  // 同時押し判定後の再入力防止用タイマー (最後に同時押しを処理した時間)

uint32_t currentLedColor = 0;                       // LED の現在の色を保持する変数

void setup() {
    pinMode(4, INPUT_PULLUP);                       // デジタル入力ピン4を入力プルアップモードに設定（通常HIGH）
    pinMode(5, INPUT_PULLUP);                       // デジタル入力ピン5を入力プルアップモードに設定（通常HIGH）
    pinMode(6, INPUT_PULLUP);                       // デジタル入力ピン6を入力プルアップモードに設定（通常HIGH）
    pinMode(7, INPUT_PULLUP);                       // デジタル入力ピン7を入力プルアップモードに設定（通常HIGH）
    pinMode(8, INPUT_PULLUP);                       // デジタル入力ピン8を入力プルアップモードに設定（通常HIGH）
    pinMode(9, INPUT_PULLUP);                       // デジタル入力ピン9を入力プルアップモードに設定（通常HIGH）
    pinMode(10, INPUT_PULLUP);                      // デジタル入力ピン10を入力プルアップモードに設定（通常HIGH）
    pinMode(14, INPUT_PULLUP);                      // 設定モード切り替えボタンのピン14を入力プルアップモードに設定（通常HIGH）
    pinMode(15, INPUT_PULLUP);                      // SWモード切り替えピン15を入力プルアップモードに設定（通常HIGH）

    if (digitalRead(15) == HIGH) {                 // ピン15の状態を読み取り、HIGHの場合（SWモードが選択されている場合）
        swswitching = true;                         // SWモードの状態をtrueに設定
        SwitchControlLibrary();                     // Nintendo Switch制御ライブラリを初期化
    }

    delay(1000);
    Serial.begin(115200);
    Serial.println("Starting...");

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);      // OLEDディスプレイを初期化（電源供給方式とI2Cアドレスを指定）
    display.clearDisplay();                         // OLED画面の表示内容をクリア
    display.setTextSize(2);                         // 文字サイズを2倍に設定
    display.setTextColor(WHITE);                    // 文字色を白に設定
    display.setCursor(0, 20);                       // カーソルの開始位置を(0, 20)に設定
    display.println("HIDtaiko");                   // OLEDに"HIDtaiko"という文字列を表示
    display.setCursor(0, 40);                       // カーソルの開始位置を(0, 40)に設定
    display.println("ver.1.2.5");                  // OLEDに"ver.1.2.5"という文字列を表示
    display.display();                              // OLED画面に設定した内容を実際に表示

    pixels.begin();                                 // NeoPixelライブラリを初期化
    pixels.show();                                  // すべてのLEDをオフにする

    if (swswitching == true) {
        Serial.println("Switchモードを有効化します。");
        delay(de);
        currentLedColor = pixels.Color(0, 255, 0); // 初期色を緑に設定 (Switchモード)
    } else {
        Serial.println("PCモードを有効化します。");
        delay(de);
        currentLedColor = pixels.Color(0, 0, 255); // 初期色を青に設定 (PCモード)
    }

    blinkyLoop(200, 1000);                         // 起動時アニメーション（LEDを点滅させる）
    delay(2000);

    pixels.fill(currentLedColor);                   // 起動時の初期色でLEDを点灯
    pixels.show();

    loadSettings();                                 // EEPROMから保存されている設定値を読み込む関数を呼び出す
    displayMode();                                  // 現在の動作モードをOLEDに表示する関数を呼び出す

    if (swswitching == true) {
        for (int i = 0; i < 6; i++) {
            SwitchControlLibrary().pressButton(Button::LCLICK); // Nintendo SwitchのLスティッククリックボタンを押す
            SwitchControlLibrary().sendReport();                 // 押したボタンの状態を送信
            delay(100);
            SwitchControlLibrary().releaseButton(Button::LCLICK);// Nintendo SwitchのLスティッククリックボタンを離す
            SwitchControlLibrary().sendReport();                 // 離したボタンの状態を送信
            delay(100);
        }
    }
}

void loop() {
    checkButtons();                                 // ボタンの状態をチェックし、設定モードへの移行などを処理する関数を呼び出す
    updateLedColor();                               // モードが変わった場合のみ LED を更新
    if (settingMode) {                             // 設定モードがtrueの場合
        if (digitalRead(6) == LOW) {               // デジタル入力ピン6がLOWの場合（数値減少ボタンが押された場合）
            settings[currentSetting]--;             // 現在選択されている設定項目の値を1減らす
            Serial.println("Up Button Pressed");
            delay(debounceDelay);
            if (settings[currentSetting] < 0) {     // 設定値が0未満になった場合
                settings[currentSetting] = 100;     // 設定値を100にリセット
            }
            displayValue();                         // 現在の設定値をOLEDに表示する関数を呼び出す
            delay(100);
        }
        if (digitalRead(7) == LOW) {               // デジタル入力ピン7がLOWの場合（数値増加ボタンが押された場合）
            settings[currentSetting]++;             // 現在選択されている設定項目の値を1増やす
            Serial.println("Down Button Pressed");
            delay(debounceDelay);
            if (settings[currentSetting] > 100) {    // 設定値が100を超えた場合
                settings[currentSetting] = 0;       // 設定値を0にリセット
            }
            displayValue();                         // 現在の設定値をOLEDに表示する関数を呼び出す
            delay(100);
        }
        if (digitalRead(8) == LOW) {               // デジタル入力ピン8がLOWの場合（決定ボタンが押された場合）
            currentSetting++;                       // 現在選択されている設定項目のインデックスを1増やす
            Serial.println("OK Button Pressed");
            delay(debounceDelay);
            if (currentSetting >= numSettings) {    // 全ての設定項目を巡回した場合
                currentSetting = 0;                 // 設定項目のインデックスを最初に戻す
                settingMode = false;                // 設定モードを終了する
                saveSettings();                     // 現在の設定値をEEPROMに保存する関数を呼び出す
                displayMode();                      // 通常モードの表示に戻す関数を呼び出す
                Serial.println("設定モードを終了します。");
            } else {
                displaySettings();                  // 次の設定項目をOLEDに表示する関数を呼び出す
            }
            delay(200);
        }
    } else {                                       // 通常モード（設定モードがfalseの場合）
        long int a3 = analogRead(A3pin);           // アナログ入力ピンA3の値を読み取る (左ドン)
        long int a0 = analogRead(A0pin);           // アナログ入力ピンA0の値を読み取る (右ドン)
        long int kapR = analogRead(rightKapPin);   // アナログ入力ピンA1の値を読み取る (右カッ)
        long int kapL = analogRead(leftKapPin);    // アナログ入力ピンA2の値を読み取る (左カッ)
        time = millis();                             // 現在のミリ秒単位の時間を取得

        // 各入力が閾値を超え、かつ再入力防止時間が経過しているかを判定
        bool leftDon = (a3 - sv3 >= settings[3] && time - ti3 > settings[4] && time - ti > settings[6]);
        bool rightDon = (a0 - sv0 >= settings[0] && time - ti0 > settings[4] && time - ti > settings[6]);
        bool leftKap = (kapL - svKapL >= settings[2] && time - tiKapL > settings[4] && time - ti > settings[6]);
        bool rightKap = (kapR - svKapR >= settings[1] && time - tiKapR > settings[4] && time - ti > settings[6]);

        if (swswitching == false) {                 // PCモードの場合
            if (leftDon && rightDon && time - timec > settings[7]) { // 左右ドン同時押し判定 (設定[7]ms以内に同時入力)
                Serial.println("大ドン! (同時押しで d と k を送信)"); // デバッグ用出力：大ドンと判定し、dとkを送信することを示す
                Keyboard.press(leftDonKey);         // 左ドンのキー（'d'）を押す
                Keyboard.press(rightDonKey);        // 右ドンのキー（'k'）を押す
                Keyboard.release(leftDonKey);       // 左ドンのキー（'d'）を離す
                Keyboard.release(rightDonKey);      // 右ドンのキー（'k'）を離す
                ti3 = time;                         // 最後に左ドンが反応した時間を更新
                ti0 = time;                         // 最後に右ドンが反応した時間を更新
                ti = time;                          // 最後に何らかの入力があった時間を更新
                timec = time;                       // 最後に同時押しを処理した時間を更新
            } else if (leftKap && rightKap && time - timec > settings[7]) { // 左右カッ同時押し判定 (設定[7]ms以内に同時入力)
                Serial.println("大カッ! (同時押しで f と j を送信)"); // デバッグ用出力：大カッと判定し、fとjを送信することを示す
                Keyboard.press(leftKapKey);         // 左カッのキー（'f'）を押す
                Keyboard.press(rightKapKey);        // 右カッのキー（'j'）を押す
                Keyboard.release(leftKapKey);       // 左カッのキー（'f'）を離す
                Keyboard.release(rightKapKey);      // 右カッのキー（'j'）を離す
tiKapL = time;                      // 最後に左カッが反応した時間を更新
                tiKapR = time;                      // 最後に右カッが反応した時間を更新
                ti = time;                          // 最後に何らかの入力があった時間を更新
                timec = time;                       // 最後に同時押しを処理した時間を更新
            } else {                                 // 同時押しでなかった場合、個別の入力を処理
                if (leftDon) {
                    Keyboard.press(leftDonKey);     // 左ドンキーを押す
                    delay(settings[8]);            // キーを押している時間
                    Keyboard.release(leftDonKey);   // 左ドンキーを離す
                    ti3 = time;                     // 最後に左ドンが反応した時間を更新
                    ti = time;                      // 最後に何らかの入力があった時間を更新
                }
                if (rightDon) {
                    Keyboard.press(rightDonKey);    // 右ドンキーを押す
                    delay(settings[8]);            // キーを押している時間
                    Keyboard.release(rightDonKey);  // 右ドンキーを離す
                    ti0 = time;                     // 最後に右ドンが反応した時間を更新
                    ti = time;                      // 最後に何らかの入力があった時間を更新
                }
                if (leftKap) {
                    Keyboard.press(leftKapKey);     // 左カッキーを押す
                    delay(settings[8]);            // キーを押している時間
                    Keyboard.release(leftKapKey);   // 左カッキーを離す
                    tiKapL = time;                  // 最後に左カッが反応した時間を更新
                    ti = time;                      // 最後に何らかの入力があった時間を更新
                }
                if (rightKap) {
                    Keyboard.press(rightKapKey);    // 右カッキーを押す
                    delay(settings[8]);            // キーを押している時間
                    Keyboard.release(rightKapKey);  // 右カッキーを離す
                    tiKapR = time;                  // 最後に右カッが反応した時間を更新
                    ti = time;                      // 最後に何らかの入力があった時間を更新
                }
            }
            if (digitalRead(4) == LOW) Keyboard.write(KEY_UP_ARROW);
            if (digitalRead(5) == LOW) Keyboard.write(KEY_RETURN);
            if (digitalRead(6) == LOW) Keyboard.write(KEY_F1);
            if (digitalRead(7) == LOW) Keyboard.write(KEY_INSERT);
            if (digitalRead(8) == LOW) Keyboard.write(KEY_ESC);
            if (digitalRead(9) == LOW) Keyboard.write(KEY_DOWN_ARROW);
            if (digitalRead(4) == LOW || digitalRead(5) == LOW || digitalRead(6) == LOW ||
                digitalRead(7) == LOW || digitalRead(8) == LOW || digitalRead(9) == LOW) {
                delay(de);
            }
        } else {                                       // SWモードの場合
            if (leftDon && rightDon && time - timec > swB) { // 左右ドン同時押し判定 (swB ms以内に同時入力)
                Serial.println("大ドン! (同時押しで ZL と ZR を押下)"); // デバッグ用出力
                SwitchControlLibrary().pressButton(swLeftDonButton);   // 左ドンボタンを押す
                SwitchControlLibrary().pressButton(swRightDonButton);  // 右ドンボタンを押す
                SwitchControlLibrary().sendReport();
                delay(cc);
                SwitchControlLibrary().releaseButton(swLeftDonButton); // 左ドンボタンを離す
                SwitchControlLibrary().releaseButton(swRightDonButton);// 右ドンボタンを離す
                SwitchControlLibrary().sendReport();
                ti3 = time;
                ti0 = time;
                ti = time;
                timec = time;
                delay(aa);
            } else if (leftKap && rightKap && time - timec > swB) { // 左右カッ同時押し判定 (swB ms以内に同時入力)
                Serial.println("大カッ! (同時押しで LCLICK と RCLICK を押下)"); // デバッグ用出力
                SwitchControlLibrary().pressButton(swLeftKapButton);   // 左カッボタンを押す
                SwitchControlLibrary().pressButton(swRightKapButton);  // 右カッボタンを押す
                SwitchControlLibrary().sendReport();
                delay(cc);
                SwitchControlLibrary().releaseButton(swLeftKapButton); // 左カッボタンを離す
                SwitchControlLibrary().releaseButton(swRightKapButton);// 右カッボタンを離す
                SwitchControlLibrary().sendReport();
                tiKapL = time;
                tiKapR = time;
                ti = time;
                timec = time;
                delay(aa);
            } else {
                if (leftDon) {
                    SwitchControlLibrary().pressButton(swLeftDonButton);
                    SwitchControlLibrary().sendReport();
                    delay(cc);
                    SwitchControlLibrary().releaseButton(swLeftDonButton);
                    SwitchControlLibrary().sendReport();
                    ti3 = time;
                    ti = time;
                    delay(aa);
                }
                if (rightDon) {
                    SwitchControlLibrary().pressButton(swRightDonButton);
                    SwitchControlLibrary().sendReport();
                    delay(cc);
                    SwitchControlLibrary().releaseButton(swRightDonButton);
                    SwitchControlLibrary().sendReport();
                    ti0 = time;
                    ti = time;
                    delay(aa);
                }
                if (leftKap) {
                    SwitchControlLibrary().pressButton(swLeftKapButton);
                    SwitchControlLibrary().sendReport();
                    delay(cc);
                    SwitchControlLibrary().releaseButton(swLeftKapButton);
                    SwitchControlLibrary().sendReport();
                    tiKapL = time;
                    ti = time;
                    delay(aa);
                }
                if (rightKap) {
                    SwitchControlLibrary().pressButton(swRightKapButton);
                    SwitchControlLibrary().sendReport();
                    delay(cc);
                    SwitchControlLibrary().releaseButton(swRightKapButton);
                    SwitchControlLibrary().sendReport();
                    delay(aa);
                }
            }
            if (digitalRead(4) == LOW) { SwitchControlLibrary().pressButton(Button::R); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::R); SwitchControlLibrary().sendReport(); delay(de); }
            if (digitalRead(5) == LOW) { SwitchControlLibrary().pressButton(Button::PLUS); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::PLUS); SwitchControlLibrary().sendReport(); delay(de); }
            if (digitalRead(6) == LOW) { SwitchControlLibrary().pressButton(Button::A); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::A); SwitchControlLibrary().sendReport(); delay(de); }
            if (digitalRead(7) == LOW) { SwitchControlLibrary().pressButton(Button::B); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::B); SwitchControlLibrary().sendReport(); delay(de); }
            if (digitalRead(8) == LOW) { SwitchControlLibrary().pressButton(Button::MINUS); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::MINUS); SwitchControlLibrary().sendReport(); delay(de); }
            if (digitalRead(9) == LOW) { SwitchControlLibrary().pressButton(Button::L); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::L); SwitchControlLibrary().sendReport(); delay(de); }
            if (digitalRead(10) == LOW) { SwitchControlLibrary().pressButton(Button::HOME); SwitchControlLibrary().sendReport(); SwitchControlLibrary().releaseButton(Button::HOME); SwitchControlLibrary().sendReport(); delay(de); }
        }
        sv3 = a3;
        sv0 = a0;
        svKapR = kapR;
        svKapL = kapL;
    }
}

void loadSettings() {
    for (int i = 0; i < numSettings; i++) {
        settings[i] = EEPROM.read(i);
    }
}

void saveSettings() {
    Serial.println("EEPROM Saved:");
    for (int i = 0; i < numSettings; i++) {
        int oldValue = EEPROM.read(i);
        if (oldValue != settings[i]) {
            EEPROM.write(i, settings[i]);
            Serial.print(settingNames[i]);
            Serial.print(":");
            Serial.print(oldValue);
            Serial.print("->");
            Serial.println(settings[i]);
        } else {
            Serial.print(settingNames[i]);
            Serial.println(":No change");
        }
    }
}

void displaySettings() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("SET MODE");
    display.setTextSize(2);
    display.setCursor(0, 24);
    display.print(settingNames[currentSetting]);
    display.print(": ");
    display.println(settings[currentSetting]);
    display.display();
}

void displayValue() {
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.fillRect(48, 24, 80, 24, BLACK);
    display.setCursor(0, 24);
    display.print(settingNames[currentSetting]);
    display.print(": ");
    display.println(settings[currentSetting]);
    display.display();
}

void checkButtons() {
    if (digitalRead(14) == LOW && !swswitching && !settingMode) {
        Serial.println("設定モードを有効化します。");
        settingMode = true;
        currentSetting = 0;
        displaySettings();
        updateLedColor();
        delay(debounceDelay);
    }
}

void displayMode() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    if (settingMode) {
        display.println(modeNames[2]);
    } else {
        display.println(modeNames[swswitching]);
    }
    display.display();
}

void blinkyLoop(int delayTime, unsigned long totalDuration) {
    unsigned long startTime = millis();
    while (millis() - startTime < totalDuration) {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.setPixelColor(1, pixels.Color(255, 0, 0));
        pixels.show();
        delay(delayTime);
        pixels.setPixelColor(0, pixels.Color(0, 0, 255));
        pixels.setPixelColor(1, pixels.Color(0, 0, 255));
        pixels.show();
        delay(delayTime);
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 0));
        pixels.show();
        delay(delayTime);
    }
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
    pixels.show();
}

void updateLedColor() {
    uint32_t newColor;
    if (settingMode) {
        newColor = pixels.Color(255, 0, 0);
    } else if (swswitching) {
        newColor = pixels.Color(0, 255, 0);
    } else {
        newColor = pixels.Color(0, 0, 255);
    }

    if (newColor != currentLedColor) {
        pixels.fill(newColor);
        pixels.show();
        currentLedColor = newColor;
    }
}
