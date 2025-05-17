#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// Wi-Fi a Firebase přihlašovací údaje
#define WIFI_SSID "aaa"
#define WIFI_PASSWORD "aaa"

#define Web_API_KEY "AIzaSyC1oK9T1jjmNsRnKnPvbci6idWWGVC4XFs"
#define DATABASE_URL "https://test-1e784-default-rtdb.europe-west1.firebasedatabase.app/"
#define USER_EMAIL "espbox17@gmail.com"
#define USER_PASS "Espbox32*"

// Firebase klient a autentizace
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Ultrazvukové senzory
#define TRIG_PIN_1 27
#define ECHO_PIN_1 26
#define TRIG_PIN_2 32
#define ECHO_PIN_2 33

long distance1, distance2;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 3000;  // Interval odesílání (3s)

// Funkce pro zpracování odpovědi z Firebase
void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;
  if (aResult.isEvent())
    Firebase.printf("Event: %s, msg: %s\n", aResult.uid().c_str(), aResult.eventLog().message().c_str());
  if (aResult.isError())
    Firebase.printf("Error: %s, msg: %s\n", aResult.uid().c_str(), aResult.error().message().c_str());
}

// Inicializace
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  // Připojení k WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Připojuji se k WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Připojeno.");

  // Firebase
  ssl_client.setInsecure();
  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

// Hlavní smyčka
void loop() {
  app.loop();  // pro autentizaci a async

  if (!app.ready()) return;

  unsigned long currentTime = millis();
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;

    distance1 = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
    distance2 = measureDistance(TRIG_PIN_2, ECHO_PIN_2);

    String stav;

    if (distance1 < 10 && distance2 < 10) {
      stav = "Schránka je plná";
    } else if ((distance1 < 10 && distance2 >= 10) || (distance2 < 10 && distance1 >= 10)) {
      stav = "Půlka boxu je plná";
    } else {
      stav = "Schránka je prázdná";
    }

    Serial.println(stav);

    // Odeslání stavu do Firebase
    Database.set<String>(aClient, "/stav_krabice", stav, processData, "Odesilani_stavu");
  }

  delay(100);  // drobná pauza, aby se šetřil procesor
}

// Měření vzdálenosti
long measureDistance(int trigPin, int echoPin) {
  long duration, distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  if (distance < 1) distance = 999;
  if (distance > 60) distance = 60;

  return distance;
}