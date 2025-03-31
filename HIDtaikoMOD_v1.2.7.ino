#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <NintendoSwitchControlLibrary.h>
#include <Keyboard.h>

// OLEDディスプレイ設定
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LED設定
#define LED_PIN 16
#define LED_COUNT 1
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 設定項目
const int numSettings = 13;
int settings[numSettings];
int currentSetting = 0;
bool settingMode = false;
bool swswitching = false;

// デバウンス設定
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// 設定項目名
String settingNames[numSettings] = {"SE0", "SE1", "SE2", "SE3", "PA ", "PB ", "PC ", "PD ", "PE ", "LRD", "CRD", "DHT", "BHD"};
String modeNames[4] = {"PC MODE", "SW MODE", "SET MODE", "INIT MODE"};

// 現在のモード
int currentMode = 0; // 0: PC, 1: SW, 2: SET, 3: INIT

// 入力ピン定義
int de = 220;
const int A0pin = A0;
const int A1pin = A1;
const int A2pin = A2;
const int A3pin = A3;

// キーボード/Switch ボタン定義
char left = 'd';
char middleleft = 'f';
char middletight = 'j';
char right = 'k';
char aa = 17;
char cc = 20;
char swA = 1;
char swB = 3;

// アナログ入力値保存用
long int sv1 = 0, sv2 = 0, sv3 = 0, sv0 = 0;
long int ti1 = 0, ti2 = 0, ti3 = 0, ti0 = 0, time = 0, timec = 0, ti = 0;

// 大判定後クールダウン
unsigned long lastBigHitTime = 0;

// LED カラー
uint32_t currentLedColor = 0;

// 自動閾値調整モード
bool autoThresholdMode = false;
enum AdjustmentStep { RIGHT_KAT, RIGHT_DON, LEFT_DON, LEFT_KAT, FINISHED };
AdjustmentStep currentStep = RIGHT_KAT;
int maxValues[4] = {0, 0, 0, 0};
int hitCounts[4] = {0, 0, 0, 0};
unsigned long lastHitTime = 0;
const unsigned long hitInterval = 200;
const int requiredHits = 5;
int autoThresholds[4];
int noiseThreshold = 50;

// 大打判定用
unsigned long firstDonStartTime = 0;
bool firstLeftDonDetected = false;
bool firstRightDonDetected = false;
unsigned long firstKatStartTime = 0;
bool firstLeftKatDetected = false;
bool firstRightKatDetected = false;
int doubleDonTimeout = 100;
int doubleKatTimeout = 100;
unsigned long firstDonStartTimeSW = 0;
bool firstLeftDonDetectedSW = false;
bool firstRightDonDetectedSW = false;
unsigned long firstKatStartTimeSW = 0;
bool firstLeftKatDetectedSW = false;
bool firstRightKatDetectedSW = false;
int doubleDonTimeoutSW = 100;
int doubleKatTimeoutSW = 100;

void setup() {
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);

  if (digitalRead(15) == HIGH) {
    swswitching = true;
    currentMode = 1; // SW MODE
  } else {
    currentMode = 0; // PC MODE
  }

  delay(1000);
  Serial.begin(115200);
  Serial.println("Starting...");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println("HIDtaiko");
  display.setCursor(0, 40);
  display.println("ver.1.2.7");
  display.display();

  pixels.begin();
  pixels.show();

  updateLedColor();
  blinkyLoop(200, 1000);
  delay(2000);
  pixels.fill(currentLedColor);
  pixels.show();

  if (digitalRead(14) == LOW) {
    autoThresholdMode = true;
    currentMode = 3; // INIT MODE
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("初期設定");
    display.setCursor(0, 40);
    display.println("モード");
    display.display();
    delay(2000);
  } else {
    loadSettings();
  }
  displayMode();

  if (swswitching) {
    for (int i = 0; i < 6; i++) {
      SwitchControlLibrary().pressButton(Button::LCLICK);
      SwitchControlLibrary().sendReport();
      delay(100);
      SwitchControlLibrary().releaseButton(Button::LCLICK);
      SwitchControlLibrary().sendReport();
      delay(100);
    }
  }
}

void loop() {
  checkButtons();
  updateLedColor();

  if (autoThresholdMode) {
    adjustThresholds();
  } else if (settingMode) {
    if (digitalRead(6) == LOW) {
      if (currentSetting == 11) {
        settings[currentSetting] -= 16;
        if (settings[currentSetting] < 0) settings[currentSetting] = 0;
      } else settings[currentSetting] = (settings[currentSetting] - 1 + 101) % 101;
      delay(debounceDelay);
      displayValue();
      delay(100);
    }
    if (digitalRead(7) == LOW) {
      if (currentSetting == 11) {
        settings[currentSetting] += 16;
        if (settings[currentSetting] > 4096) settings[currentSetting] = 4096;
      } else settings[currentSetting] = (settings[currentSetting] + 1) % 101;
      delay(debounceDelay);
      displayValue();
      delay(100);
    }
    if (digitalRead(8) == LOW) {
      currentSetting++;
      delay(debounceDelay);
      if (currentSetting >= numSettings) {
        currentSetting = 0;
        settingMode = false;
        saveSettings();
        displayMode();
      } else {
        displaySettings();
      }
      delay(200);
    }
  } else {
    long int a3 = analogRead(A3pin);
    long int a0 = analogRead(A0pin);
    long int a1 = analogRead(A1pin);
    long int a2 = analogRead(A2pin);
    time = millis();

    if (!swswitching) {
      // 大カッ判定
      if (a3 - sv3 >= settings[3] && !firstLeftKatDetected && a0 - sv0 >= settings[0] && !firstRightKatDetected && abs(ti3 - ti0) <= settings[11]) {
        Keyboard.press(left);
        Keyboard.press(right);
        delay(settings[9]);
        Keyboard.release(left);
        Keyboard.release(right);
        ti = time;
        lastBigHitTime = time;
        firstLeftKatDetected = false;
        firstRightKatDetected = false;
      }
      // 左カッ
      else if (a3 - sv3 >= settings[3] && !firstLeftKatDetected && time - ti3 > settings[4] && time - ti > settings[6] && (time - lastBigHitTime > settings[12])) {
        firstLeftKatDetected = true;
        firstKatStartTime = time;
        ti3 = time;
        ti = time;
      } else if (a0 - sv0 >= settings[0] && firstLeftKatDetected && time - firstKatStartTime <= doubleKatTimeout && time - ti0 > settings[4] && time - ti > settings[6]) {
        Keyboard.press(left);
        Keyboard.press(right);
        delay(settings[9]);
        Keyboard.release(left);
        Keyboard.release(right);
        firstLeftKatDetected = false;
        firstRightKatDetected = false;
        ti = time;
        ti0 = time;
      } else if (firstLeftKatDetected && time - firstKatStartTime > doubleKatTimeout) {
        Keyboard.press(left);
        delay(settings[8]);
        Keyboard.release(left);
        firstLeftKatDetected = false;
        ti = time;
      }
      // 右カッ
      else if (a0 - sv0 >= settings[0] && !firstRightKatDetected && time - ti0 > settings[4] && time - ti > settings[6] && (time - lastBigHitTime > settings[12])) {
        firstRightKatDetected = true;
        firstKatStartTime = time;
        ti0 = time;
        ti = time;
      } else if (a3 - sv3 >= settings[3] && firstRightKatDetected && time - firstKatStartTime <= doubleKatTimeout && time - ti3 > settings[4] && time - ti > settings[6]) {
        Keyboard.press(left);
        Keyboard.press(right);
        delay(settings[9]);
        Keyboard.release(left);
        Keyboard.release(right);
        firstLeftKatDetected = false;
        firstRightKatDetected = false;
        ti = time;
        ti3 = time;
      } else if (firstRightKatDetected && time - firstKatStartTime > doubleKatTimeout) {
        Keyboard.press(right);
        delay(settings[8]);
        Keyboard.release(right);
        firstRightKatDetected = false;
        ti = time;
      }
      // 大ドン判定
      if (a1 - sv1 >= settings[1] && !firstRightDonDetected && a2 - sv2 >= settings[2] && !firstLeftDonDetected && abs(ti1 - ti2) <= settings[11]) {
        Keyboard.press(middletight);
        Keyboard.press(middleleft);
        delay(settings[10]);
        Keyboard.release(middletight);
        Keyboard.release(middleleft);
        ti = time;
        lastBigHitTime = time;
        firstLeftDonDetected = false;
        firstRightDonDetected = false;
      }
      // 右ドン
      else if (a1 - sv1 >= settings[1] && !firstRightDonDetected && time - ti1 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7] && (time - lastBigHitTime > settings[12])) {
        firstRightDonDetected = true;
        firstDonStartTime = time;
        ti1 = time;
        ti = time;
      } else if (a2 - sv2 >= settings[2] && firstRightDonDetected && time - firstDonStartTime <= doubleDonTimeout && time - ti2 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) {
        Keyboard.press(middletight);
        Keyboard.press(middleleft);
        delay(settings[10]);
        Keyboard.release(middletight);
        Keyboard.release(middleleft);
        firstLeftDonDetected = false;
        firstRightDonDetected = false;
        ti = time;
        ti2 = time;
      } else if (firstRightDonDetected && time - firstDonStartTime > doubleDonTimeout) {
        Keyboard.press(middletight);
        delay(settings[8]);
        Keyboard.release(middletight);
        firstRightDonDetected = false;
        ti = time;
      }
      // 左ドン
      else if (a2 - sv2 >= settings[2] && !firstLeftDonDetected && time - ti2 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7] && (time - lastBigHitTime > settings[12])) {
        firstLeftDonDetected = true;
        firstDonStartTime = time;
        ti2 = time;
        ti = time;
      } else if (a1 - sv1 >= settings[1] && firstLeftDonDetected && time - firstDonStartTime <= doubleDonTimeout && time - ti1 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) {
        Keyboard.press(middletight);
        Keyboard.press(middleleft);
        delay(settings[10]);
        Keyboard.release(middletight);
        Keyboard.release(middleleft);
        firstLeftDonDetected = false;
        firstRightDonDetected = false;
        ti = time;
        ti1 = time;
      } else if (firstLeftDonDetected && time - firstDonStartTime > doubleDonTimeout) {
        Keyboard.press(middleleft);
        delay(settings[8]);
        Keyboard.release(middleleft);
        firstLeftDonDetected = false;
        ti = time;
      }

      if (digitalRead(4) == LOW) Keyboard.write(KEY_UP_ARROW), delay(de);
      if (digitalRead(5) == LOW) Keyboard.write(KEY_RETURN), delay(de);
      if (digitalRead(6) == LOW) Keyboard.write(KEY_F1), delay(de);
      if (digitalRead(7) == LOW) Keyboard.write(KEY_INSERT), delay(de);
      if (digitalRead(8) == LOW) Keyboard.write(KEY_ESC), delay(de);
      if (digitalRead(9) == LOW) Keyboard.write(KEY_DOWN_ARROW), delay(de);
    } else {
      // SWモード
      // 大カッ判定
      if (a3 - sv3 >= settings[3] && !firstLeftKatDetectedSW && a0 - sv0 >= settings[0] && !firstRightKatDetectedSW && abs(ti3 - ti0) <= settings[11]) {
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(settings[9]);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = time;
        lastBigHitTime = time;
        firstLeftKatDetectedSW = false;
        firstRightKatDetectedSW = false;
      }
      // 左カッ
      else if (a3 - sv3 >= settings[3] && !firstLeftKatDetectedSW && time - ti3 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        firstLeftKatDetectedSW = true;
        firstKatStartTimeSW = time;
        ti3 = time;
        ti = time;
      } else if (a0 - sv0 >= settings[0] && firstLeftKatDetectedSW && time - firstKatStartTimeSW <= doubleKatTimeoutSW && time - ti0 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(settings[9]);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = time;
        lastBigHitTime = time;
        firstLeftKatDetectedSW = false;
        firstRightKatDetectedSW = false;
      } else if (firstLeftKatDetectedSW && time - firstKatStartTimeSW > doubleKatTimeoutSW) {
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti3 = time;
        ti = time;
        firstLeftKatDetectedSW = false;
      }
      // 右カッ
      else if (a0 - sv0 >= settings[0] && !firstRightKatDetectedSW && time - ti0 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        firstRightKatDetectedSW = true;
        firstKatStartTimeSW = time;
        ti0 = time;
        ti = time;
      } else if (a3 - sv3 >= settings[3] && firstRightKatDetectedSW && time - firstKatStartTimeSW <= doubleKatTimeoutSW && time - ti3 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(settings[9]);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = time;
        lastBigHitTime = time;
        firstLeftKatDetectedSW = false;
        firstRightKatDetectedSW = false;
      } else if (firstRightKatDetectedSW && time - firstKatStartTimeSW > doubleKatTimeoutSW) {
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti0 = time;
        ti = time;
        firstRightKatDetectedSW = false;
      }
      // 大ドン判定
      if (a1 - sv1 >= settings[1] && !firstRightDonDetectedSW && a2 - sv2 >= settings[2] && !firstLeftDonDetectedSW && abs(ti1 - ti2) <= settings[11]) {
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(settings[10]);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = time;
        lastBigHitTime = time;
        firstLeftDonDetectedSW = false;
        firstRightDonDetectedSW = false;
      }
      // 右ドン
      else if (a1 - sv1 >= settings[1] && !firstRightDonDetectedSW && time - ti1 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        firstRightDonDetectedSW = true;
        firstDonStartTimeSW = time;
        ti1 = time;
        ti = time;
      } else if (a2 - sv2 >= settings[2] && firstRightDonDetectedSW && time - firstDonStartTimeSW <= doubleDonTimeoutSW && time - ti2 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(settings[10]);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = time;
        lastBigHitTime = time;
        firstLeftDonDetectedSW = false;
        firstRightDonDetectedSW = false;
      } else if (firstRightDonDetectedSW && time - firstDonStartTimeSW > doubleDonTimeoutSW) {
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti1 = time;
        ti = time;
        firstRightDonDetectedSW = false;
      }
      // 左ドン
      else if (a2 - sv2 >= settings[2] && !firstLeftDonDetectedSW && time - ti2 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        firstLeftDonDetectedSW = true;
        firstDonStartTimeSW = time;
        ti2 = time;
        ti = time;
      } else if (a1 - sv1 >= settings[1] && firstLeftDonDetectedSW && time - firstDonStartTimeSW <= doubleDonTimeoutSW && time - ti1 > swA && time - ti > swB && (time - lastBigHitTime > settings[12])) {
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(settings[10]);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = time;
        lastBigHitTime = time;
        firstLeftDonDetectedSW = false;
        firstRightDonDetectedSW = false;
      } else if (firstLeftDonDetectedSW && time - firstDonStartTimeSW > doubleDonTimeoutSW) {
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti2 = time;
        ti = time;
        firstLeftDonDetectedSW = false;
      }

      if (digitalRead(4) == LOW) SwitchControlLibrary().pressButton(Button::R), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::R), SwitchControlLibrary().sendReport(), delay(de);
      if (digitalRead(5) == LOW) SwitchControlLibrary().pressButton(Button::PLUS), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::PLUS), SwitchControlLibrary().sendReport(), delay(de);
      if (digitalRead(6) == LOW) SwitchControlLibrary().pressButton(Button::A), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::A), SwitchControlLibrary().sendReport(), delay(de);
      if (digitalRead(7) == LOW) SwitchControlLibrary().pressButton(Button::B), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::B), SwitchControlLibrary().sendReport(), delay(de);
      if (digitalRead(8) == LOW) SwitchControlLibrary().pressButton(Button::MINUS), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::MINUS), SwitchControlLibrary().sendReport(), delay(de);
      if (digitalRead(9) == LOW) SwitchControlLibrary().pressButton(Button::L), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::L), SwitchControlLibrary().sendReport(), delay(de);
      if (digitalRead(10) == LOW) SwitchControlLibrary().pressButton(Button::HOME), SwitchControlLibrary().sendReport(), delay(de), SwitchControlLibrary().releaseButton(Button::HOME), SwitchControlLibrary().sendReport(), delay(de);
    }
    sv3 = a3;
    sv0 = a0;
    sv1 = a1;
    sv2 = a2;
  }
}

void loadSettings() {
  for (int i = 0; i < numSettings; i++) {
    settings[i] = EEPROM.read(i);
    if (settings[i] == 255) settings[i] = 50; // 初期値
  }
}

void saveSettings() {
  for (int i = 0; i < numSettings; i++) {
    EEPROM.update(i, settings[i]);
  }
}

void displaySettings() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SET MODE");
  display.print("Item: ");
  display.println(settingNames[currentSetting]);
  display.print("Value: ");
  display.println(settings[currentSetting]);
  display.display();
}

void displayValue() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SET MODE");
  display.print("Item: ");
  display.println(settingNames[currentSetting]);
  display.print("Value: ");
  display.println(settings[currentSetting]);
  display.display();
}

void checkButtons() {
  if (digitalRead(14) == LOW && !settingMode && !autoThresholdMode) {
    settingMode = true;
    currentSetting = 0;
    displaySettings();
    delay(200);
  }
}

void displayMode() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Mode: ");
  display.println(modeNames[currentMode]);
  display.display();
}

void blinkyLoop(int blinkDelay, int duration) {
  long startTime = millis();
  while (millis() - startTime < duration) {
    pixels.fill(pixels.Color(255, 255, 255));
    pixels.show();
    delay(blinkDelay);
    pixels.fill(pixels.Color(0, 0, 0));
    pixels.show();
    delay(blinkDelay);
  }
}

void updateLedColor() {
  uint32_t newColor;
  switch (currentMode) {
    case 3: // INIT MODE
      newColor = pixels.Color(255, 0, 255); // マゼンタ
      break;
    case 2: // SET MODE
      newColor = pixels.Color(255, 255, 0); // 黄色
      break;
    case 1: // SW MODE
      newColor = pixels.Color(0, 255, 0);   // 緑
      break;
    case 0: // PC MODE
    default:
      newColor = pixels.Color(0, 0, 255);   // 青
      break;
  }
  if (newColor != currentLedColor) {
    pixels.fill(newColor);
    pixels.show();
    currentLedColor = newColor;
  }
}

void adjustThresholds() {
  if (currentStep != FINISHED) {
    int sensorValues[4];
    int sensorIndex;

    sensorValues[0] = analogRead(A0pin); // 右カッ
    sensorValues[1] = analogRead(A1pin); // 右ドン
    sensorValues[2] = analogRead(A2pin); // 左ドン
    sensorValues[3] = analogRead(A3pin); // 左カッ

    switch (currentStep) {
      case RIGHT_KAT: sensorIndex = 0; break;
      case RIGHT_DON: sensorIndex = 1; break;
      case LEFT_DON: sensorIndex = 2; break;
      case LEFT_KAT: sensorIndex = 3; break;
      default: return;
    }

    if (sensorValues[sensorIndex] > maxValues[sensorIndex]) {
      maxValues[sensorIndex] = sensorValues[sensorIndex];
    }

    if (sensorValues[sensorIndex] > noiseThreshold && millis() - lastHitTime > hitInterval) {
      hitCounts[sensorIndex]++;
      lastHitTime = millis();
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("初期設定モード");
      display.print("調整中: ");
      display.println(settingNames[currentStep]);
      display.print("回数: ");
      display.println(hitCounts[sensorIndex]);
      display.display();
      delay(500);

      if (hitCounts[sensorIndex] >= requiredHits) {
        autoThresholds[sensorIndex] = maxValues[sensorIndex] * 0.6; // 例：最大値の60%
        settings[sensorIndex] = autoThresholds[sensorIndex]; // 閾値を settings に反映
        maxValues[sensorIndex] = 0;
        hitCounts[sensorIndex] = 0;
        currentStep = (AdjustmentStep)(currentStep + 1);
        if (currentStep != FINISHED) {
          display.clearDisplay();
          display.setTextSize(1);
          display.setCursor(0, 0);
          display.println("初期設定モード");
          display.print("次は ");
          display.println(settingNames[currentStep]);
          display.println("を叩いてください");
          display.display();
          delay(2000);
        } else {
          saveSettings();
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 20);
          display.println("調整完了!");
          display.display();
          delay(2000);
          autoThresholdMode = false; // 調整完了
          if (swswitching) currentMode = 1;
          else currentMode = 0;
          displayMode();
        }
      }
    }
  }
}
