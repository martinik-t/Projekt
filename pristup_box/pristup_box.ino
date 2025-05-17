#include <Keypad.h>
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// === WiFi & Firebase nastavení ===
#define WIFI_SSID "aaa"
#define WIFI_PASSWORD "aaa"
#define Web_API_KEY "AIzaSyC1oK9T1jjmNsRnKnPvbci6idWWGVC4XFs"
#define DATABASE_URL "https://test-1e784-default-rtdb.europe-west1.firebasedatabase.app/"
#define USER_EMAIL "espbox17@gmail.com"
#define USER_PASS "Espbox32*"

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// === LCD ===
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === Keypad ===
const byte ROW_NUM = 4;
const byte COLUMN_NUM = 4;
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte pin_rows[ROW_NUM] = {14, 12, 13, 15};
byte pin_column[COLUMN_NUM] = {5, 18, 19, 23};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// === Servo ===
Servo servo;
const int SERVO_PIN = 16;
bool servoOpened = false;
unsigned long servoOpenedAt = 0;
const unsigned long servoOpenDuration = 5000;  // 5 s

// === Čas a kód ===
unsigned long previousMillis = 0;
const long interval = 120000;  // 2 min
String generatedCode = "";
String enteredCode = "";
unsigned long resultDisplayedAt = 0;
const unsigned long resultDisplayDuration = 5000;

void setup() {
  Serial.begin(115200);
  lcd.begin(); lcd.backlight();
  servo.attach(SERVO_PIN); servo.write(0);
  lcd.setCursor(0, 0); lcd.print("Zadej kod:");

  // WiFi a Firebase
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  ssl_client.setInsecure(); ssl_client.setConnectionTimeout(1000);
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  generateCode();  // první kód
}

void loop() {
  app.loop();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    generateCode();
  }

  if (resultDisplayedAt > 0 && currentMillis - resultDisplayedAt >= resultDisplayDuration) {
    lcd.clear(); lcd.setCursor(0, 0); lcd.print("Zadej kod:");
    resultDisplayedAt = 0;
  }

  if (servoOpened && currentMillis - servoOpenedAt >= servoOpenDuration) {
    closeLid("Zavreno");
  }

  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      lcd.clear();
      if (enteredCode == generatedCode) {
        openLid();
      } else {
        lcd.setCursor(0, 0); lcd.print("Zamitnuto");
        Database.set<String>(aClient, "/box/status", "Spatny kod", processData, "RTDB_Status");
      }
      enteredCode = "";
      resultDisplayedAt = millis();
    } else if (key == '*') {
      enteredCode = ""; lcd.clear(); lcd.setCursor(0, 0); lcd.print("Zadej kod:");
    } else if (key == 'A') {
      if (servoOpened) closeLid("Zavreno manualne");
    } else {
      enteredCode += key;
    }

    if (resultDisplayedAt == 0) {
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Zadej kod:");
      lcd.setCursor(0, 1);
      for (int i = 0; i < enteredCode.length(); i++) lcd.print("*");
      lcd.print("    ");
    }
  }
}

void generateCode() {
  generatedCode = "";
  for (int i = 0; i < 4; i++) generatedCode += String(random(0, 10));
  Serial.print("Novy kod: "); Serial.println(generatedCode);
  Database.set<String>(aClient, "/box/code", generatedCode, processData, "RTDB_Code");
}

void openLid() {
  Serial.println("Otevreno"); lcd.setCursor(0, 0); lcd.print("Otevreno");
  servo.write(180); servoOpened = true; servoOpenedAt = millis();
  Database.set<String>(aClient, "/box/status", "Otevreno", processData, "RTDB_Status");
}

void closeLid(String status) {
  servo.write(0); servoOpened = false;
  Serial.println("Viko zavreno");
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(status);
  Database.set<String>(aClient, "/box/status", status, processData, "RTDB_Status");
}

void processData(AsyncResult &aResult) {
  if (aResult.isError()) Firebase.printf("Chyba: %s\n", aResult.error().message().c_str());
}
