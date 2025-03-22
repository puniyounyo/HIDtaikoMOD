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
  Serial.println("EEPROM Writer for Settings Integrated.");
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
    } else if (command == "readall") {
      Serial.println("--- EEPROM Contents ---");
      for (int i = 0; i < EEPROM.length(); i++) {
        Serial.print(i);
        Serial.print(": ");
        Serial.println(EEPROM.read(i));
        delay(10);
      }
      Serial.println("--- End of EEPROM ---");
    }
  }

  if (settingMode) {
    if (digitalRead(6) == LOW) {
      settings[currentSetting]--;
      Serial.println("Up Button Pressed");
      delay(debounceDelay);
      if (settings[currentSetting] < 0) {
        settings[currentSetting] = 100;
      }
      displayValue();
      delay(100);
    }
    if (digitalRead(7) == LOW) {
      settings[currentSetting]++;
      Serial.println("Down Button Pressed");
      delay(debounceDelay);
      if (settings[currentSetting] > 100) {
        settings[currentSetting] = 0;
      }
      displayValue();
      delay(100);
    }
    if (digitalRead(8) == LOW) {
      currentSetting++;
      Serial.println("OK Button Pressed");
      delay(debounceDelay);
      if (currentSetting >= numSettings) {
        currentSetting = 0;
        settingMode = false;
        saveSettings();
        displayMode();
        Serial.println("設定モードを終了します。");
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

    if (swswitching == false) {
      if (a3 - sv3 >= settings[3] && a0 - sv0 >= settings[0] && abs(ti3 - ti0) <= settings[11]) {
        Serial.println("大カッ!");
        Keyboard.press(left);
        Keyboard.press(right);
        delay(settings[9]);
        Keyboard.release(left);
        Keyboard.release(right);
        ti = millis();
      }
      if (a3 - sv3 >= settings[3] && time - ti3 > settings[4] && time - ti > settings[6]) {
        Serial.println("左カッ!");
        Keyboard.press(left);
        delay(settings[8]);
        Keyboard.release(left);
        ti3 = millis();
        ti = millis();
      }
      if (a0 - sv0 >= settings[0] && time - ti0 > settings[4] && time - ti > settings[6]) {
        Serial.println("右カッ!");
        Keyboard.press(right);
        delay(settings[8]);
        Keyboard.release(right);
        ti0 = millis();
        ti = millis();
      }
      if (a1 - sv1 >= settings[1] && a2 - sv2 >= settings[2] && abs(ti1 - ti2) <= settings[11]) {
        Serial.println("大ドン!");
        Keyboard.press(middletight);
        Keyboard.press(middleleft);
        delay(settings[10]);
        Keyboard.release(middletight);
        Keyboard.release(middleleft);
        ti = millis();
      }
      if (a1 - sv1 >= settings[1] && time - ti1 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) {
        Serial.println("右ドン!");
        Keyboard.press(middletight);
        delay(settings[8]);
        Keyboard.release(middletight);
        ti1 = millis();
        ti = millis();
      }
      if (a2 - sv2 >= settings[2] && time - ti2 > settings[4] && time - ti > settings[5] && time - ti0 > settings[7] && time - ti3 > settings[7]) {
        Serial.println("左ドン!");
        Keyboard.press(middleleft);
        delay(settings[8]);
        Keyboard.release(middleleft);
        ti2 = millis();
        ti = millis();
      }
      if (digitalRead(4) == LOW) {
        Keyboard.write(KEY_UP_ARROW);
        delay(de);
      }
      if (digitalRead(5) == LOW) {
        Keyboard.write(KEY_RETURN);
        delay(de);
      }
      if (digitalRead(6) == LOW) {
        Keyboard.write(KEY_F1);
        delay(de);
      }
      if (digitalRead(7) == LOW) {
        Keyboard.write(KEY_INSERT);
        delay(de);
      }
      if (digitalRead(8) == LOW) {
        Keyboard.write(KEY_ESC);
        delay(de);
      }
      if (digitalRead(9) == LOW) {
        Keyboard.write(KEY_DOWN_ARROW);
        delay(de);
      }
    } else {
      if (a3 - sv3 >= settings[3] && a0 - sv0 >= settings[0] && abs(ti3 - ti0) <= settings[11]) {
        Serial.println("大カッ!");
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(settings[9]);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = millis();
      }
      if (a3 - sv3 >= settings[3] && time - ti3 > swA && time - ti > swB) {
        Serial.println("左カッ!");
        SwitchControlLibrary().pressButton(Button::ZL);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::ZL);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti3 = millis();
        ti = millis();
      }
      if (a0 - sv0 >= settings[0] && time - ti0 > swA && time - ti > swB) {
        Serial.println("右カッ!");
        SwitchControlLibrary().pressButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::ZR);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti0 = millis();
        ti = millis();
      }
      if (a1 - sv1 >= settings[1] && a2 - sv2 >= settings[2] && abs(ti1 - ti2) <= settings[11]) {
        Serial.println("大ドン!");
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(settings[10]);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti = millis();
      }
      if (a1 - sv1 >= settings[1] && time - ti1 > swA && time - ti > swB) {
        Serial.println("右ドン!");
        SwitchControlLibrary().pressButton(Button::RCLICK);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::RCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
        ti1 = millis();
        ti = millis();
      }
      if (a2 - sv2 >= settings[2] && time - ti2 > swA && time - ti > swB) {
        Serial.println("左ドン!");
        SwitchControlLibrary().pressButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(cc);
        SwitchControlLibrary().releaseButton(Button::LCLICK);
        SwitchControlLibrary().sendReport();
        delay(aa);
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
