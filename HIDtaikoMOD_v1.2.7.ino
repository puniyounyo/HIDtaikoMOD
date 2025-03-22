#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <NintendoSwitchControlLibrary.h>
#include <Keyboard.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN 16
#define LED_COUNT 1
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const int numSettings = 12;
int settings[numSettings];
String settingNames[numSettings] = {"SE0", "SE1", "SE2", "SE3", "PA ", "PB ", "PC ", "PD ", "PE ", "LRD", "CRD", "DHT"};
String defaultSettingNames[numSettings] = {"SE0", "SE1", "SE2", "SE3", "PA ", "PB ", "PC ", "PD ", "PE ", "LRD", "CRD", "DHT"};

int currentSetting = 0;
bool settingMode = false;
bool swswitching = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

String modeNames[3] = {"PC MODE", "SW MODE", "SET MODE"};

int de = 220;
const int A0pin = A0;
const int A1pin = A1;
const int A2pin = A2;
const int A3pin = A3;

char left = 'd';
char middleleft = 'f';
char middletight = 'j';
char right = 'k';

char aa = 17;
char cc = 20;
char swA = 1;
char swB = 3;

long int sv1 = 0;
long int sv2 = 0;
long int sv3 = 0;
long int sv0 = 0;
long int ti1 = 0;
long int ti2 = 0;
long int ti3 = 0;
long int ti0 = 0;
long int time = 0;
long int timec = 0;
long int ti = 0;

uint32_t currentLedColor = 0;

const int settingNamesBaseAddress = 100;

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
    SwitchControlLibrary();
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
  display.println("ver.1.2.6");
  display.display();

  pixels.begin();
  pixels.show();

  if (swswitching == true) {
    Serial.println("Switchモードを有効化します。");
    delay(de);
    currentLedColor = pixels.Color(0, 255, 0);
  } else {
    Serial.println("PCモードを有効化します。");
    delay(de);
    currentLedColor = pixels.Color(0, 0, 255);
  }

  blinkyLoop(200, 1000);
  delay(2000);

  pixels.fill(currentLedColor);
  pixels.show();

  loadSettings();
  loadSettingNames();
  displayMode();

  if (swswitching == true) {
    for (int i = 0; i < 6; i++) {
      SwitchControlLibrary().pressButton(Button::LCLICK);
      SwitchControlLibrary().sendReport();
      delay(100);
      SwitchControlLibrary().releaseButton(Button::LCLICK);
      SwitchControlLibrary().sendReport();
      delay(100);
    }
  }
  Serial.println("EEPROM Writer with Get/Set Settings Integrated.");
}

void loop() {
  checkButtons();
  updateLedColor();

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    Serial.print("Received command (WebSerial): ");
    Serial.println(command);

    if (command.startsWith("write:")) {
      String data = command.substring(6);
      int colonIndex = data.indexOf(':');
      if (colonIndex != -1) {
        String addressStr = data.substring(0, colonIndex);
        String valueStr = data.substring(colonIndex + 1);

        int address = addressStr.toInt();
        int value = valueStr.toInt();

        if (address >= 0 && address < numSettings) { // アドレスの範囲を numSettings に修正
          settings[address] = value; // settings配列を直接更新
          EEPROM.write(address, value); // EEPROM に保存
          Serial.print("Wrote value ");
          Serial.print(value);
          Serial.print(" to setting ");
          Serial.println(settingNames[address]);
          Serial.println("OK:SETTING_UPDATED");
          if (!settingMode) {
            displayValue();
          }
        } else {
          Serial.println("Error: Invalid setting index.");
          Serial.println("ERROR:INVALID_INDEX");
        }
      } else {
        Serial.println("Error: Invalid write command format (index:value).");
        Serial.println("ERROR:WRITE_FORMAT");
      }
    } else if (command == "getsettings") {
      Serial.println("--- Settings ---");
      for (int i = 0; i < numSettings; i++) {
        Serial.print(i);
        Serial.print(":");
        Serial.print(settingNames[i]);
        Serial.print(":");
        Serial.println(settings[i]);
      }
      Serial.println("--- End Settings ---");
    } else if (command.startsWith("setname:")) {
      String data = command.substring(8);
      int commaIndex = data.indexOf(',');
      if (commaIndex != -1) {
        String indexStr = data.substring(0, commaIndex);
        String newName = data.substring(commaIndex + 1);
        int index = indexStr.toInt();
        if (index >= 0 && index < numSettings) {
          settingNames[index] = newName;
          Serial.print("Setting name at index ");
          Serial.print(index);
          Serial.print(" updated to: ");
          Serial.println(newName);
          saveSettingNames(); // 設定名をEEPROMに保存
          if (settingMode && currentSetting == index) {
            displaySettings(); // 設定モードなら表示を更新
          } else if (!settingMode) {
            displayMode(); // 通常モードならモード表示を更新（設定名が変わったことを反映）
          }
          Serial.println("OK:NAME_UPDATED");
        } else {
          Serial.println("Error: Invalid setting index.");
          Serial.println("ERROR:INVALID_INDEX");
        }
      }
ti2 = millis();
        ti = millis();
      }
      if (digitalRead(4) == LOW) {
        SwitchControlLibrary().pressButton(Button::R);
        SwitchControlLibrary().sendReport();
        delay(de);
        SwitchControlLibrary().releaseButton(Button::R);
        SwitchControlLibrary().sendReport();
        delay(de);
      }
      if (digitalRead(5) == LOW) {
        SwitchControlLibrary().pressButton(Button::PLUS);
        SwitchControlLibrary().sendReport();
        delay(de);
        SwitchControlLibrary().releaseButton(Button::PLUS);
        SwitchControlLibrary().sendReport();
        delay(de);
      }
      if (digitalRead(6) == LOW) {
        SwitchControlLibrary().pressButton(Button::A);
        SwitchControlLibrary().sendReport();
        delay(de);
        SwitchControlLibrary().releaseButton(Button::A);
        SwitchControlLibrary().sendReport();
        delay(de);
      }
      if (digitalRead(7) == LOW) {
        SwitchControlLibrary().pressButton(Button::B);
        SwitchControlLibrary().sendReport();
        delay(de);
        SwitchControlLibrary().releaseButton(Button::B);
        SwitchControlLibrary().sendReport();
        delay(de);
      }
      if (digitalRead(8) == LOW) {
        SwitchControlLibrary().pressButton(Button::MINUS);
        SwitchControlLibrary().sendReport();
        delay(de);
        SwitchControlLibrary().releaseButton(Button::MINUS);
        SwitchControlLibrary().sendReport();
        delay(de);
      }
      if (digitalRead(9) == LOW) {
        SwitchControlLibrary().pressButton(Button::DOWN);
        SwitchControlLibrary().sendReport();
        delay(de);
        SwitchControlLibrary().releaseButton(Button::DOWN);
        SwitchControlLibrary().sendReport();
        delay(de);
      }
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
  }
}

void saveSettings() {
  Serial.println("EEPROM Settings Saved:");
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
    pixels.show();
    delay(delayTime);
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    delay(delayTime);
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
    delay(delayTime);
  }
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

void updateLedColor() {
  uint32_t newColor;
  if (settingMode) {
    newColor = pixels.Color(255, 0, 0); // 赤
  } else if (swswitching) {
    newColor = pixels.Color(0, 255, 0); // 緑
  } else {
    newColor = pixels.Color(0, 0, 255); // 青
  }

  if (newColor != currentLedColor) {
    pixels.fill(newColor);
    pixels.show();
    currentLedColor = newColor;
  }
}

// EEPROM から設定名をロードする関数
void loadSettingNames() {
  Serial.println("Loading Setting Names from EEPROM...");
  for (int i = 0; i < numSettings; i++) {
    int address = settingNamesBaseAddress + i * 32; // 各設定名に32バイト割り当て
    String name = "";
    for (int j = 0; j < 31; j++) { // 最大31文字 + null終端
      char c = EEPROM.read(address + j);
      if (c == 0) break; // null終端で終了
      name += c;
    }
    if (name.length() > 0) {
      settingNames[i] = name;
      Serial.print("Loaded name for index ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(settingNames[i]);
    } else {
      settingNames[i] = defaultSettingNames[i]; // EEPROMに保存されていなければデフォルト値を使用
      Serial.print("No custom name found for index ");
      Serial.print(i);
      Serial.print(", using default: ");
      Serial.println(settingNames[i]);
    }
  }
}

// 設定名を EEPROM に保存する関数
void saveSettingNames() {
  Serial.println("Saving Setting Names to EEPROM...");
  for (int i = 0; i < numSettings; i++) {
    int address = settingNamesBaseAddress + i * 32;
    const char* name = settingNames[i].c_str();
    for (int j = 0; j < 31; j++) {
      if (name[j] == 0) {
        EEPROM.write(address + j, 0); // null終端を書き込む
        break;
      }
      EEPROM.write(address + j, name[j]);
    }
    Serial.print("Saved name for index ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(settingNames[i]);
  }
  Serial.println("Setting Names saved to EEPROM.");
}

// デフォルトの設定名をロードする関数
void loadDefaultSettingNames() {
  for (int i = 0; i < numSettings; i++) {
    settingNames[i] = defaultSettingNames[i];
  }
  Serial.println("Default setting names loaded.");
}
}
