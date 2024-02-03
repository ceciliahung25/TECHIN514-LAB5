#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h" // Helper functions for Firebase token
#include "addons/RTDBHelper.h" // Helper functions for Firebase Realtime Database

// WiFi credentials
const char* ssid = "DC:54:75:D7:9F:68";
const char* password = "Y;$7Ym6g@V";

// Firebase project settings
#define DATABASE_URL "https://battery-management-lab-default-rtdb.firebaseio.com/" 
#define API_KEY "AIzaSyCehJGpWQfoAjRL_SOPemXhAKkz0CUZLeI"

// Deep sleep duration in microseconds (30 seconds)
#define DEEP_SLEEP_DURATION 30e6 
// Distance threshold in centimeters
#define DISTANCE_THRESHOLD 50.0  
// Maximum number of WiFi connection retries
#define MAX_WIFI_RETRIES 5

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool wifiConnected = false;
float lastDistance = 0;

// HC-SR04 sensor pins
const int trigPin = 2;
const int echoPin = 3;

// Speed of sound in cm/µs
const float soundSpeed = 0.034;

float measureDistance();
void goToDeepSleep();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize deep sleep wake-up timer
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);

  // Perform initial distance measurement to avoid immediate sleep
  lastDistance = measureDistance();
  delay(1000);  // Short wait

  // Enter deep sleep
  goToDeepSleep();
}

void loop() {
  float distance = measureDistance();

  // Check if data needs to be sent
  if (distance < DISTANCE_THRESHOLD && lastDistance < DISTANCE_THRESHOLD) {
    if (!wifiConnected) {
      connectToWiFi();
      wifiConnected = true;
    }
    initFirebase();
    sendDataToFirebase(distance);
  }
  lastDistance = distance;

  // Enter deep sleep
  goToDeepSleep();
}

float measureDistance() {
  // Ultrasonic sensor distance measurement logic
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi() {
  // WiFi connection logic
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES) {
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase() {
  // Firebase initialization logic
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up process (if necessary)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up OK");
    wifiConnected = true;
  } else {
    Serial.printf("Firebase sign-up failed: %s\n", config.signer.signupError.message.c_str());
  }

  // Callback function for token generation task
  config.token_status_callback = tokenStatusCallback; // Defined in addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void sendDataToFirebase(float distance) {
  // Sending data to Firebase
  if (Firebase.ready() && wifiConnected) {
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance)) {
      Serial.println("Data sent to Firebase");
    } else {
      Serial.print("Failed to send data: ");
      Serial.println(fbdo.errorReason());
    }
  }
}

void goToDeepSleep() {
  // Disconnect WiFi and enter deep sleep
  if (wifiConnected) {
    WiFi.disconnect();
    wifiConnected = false;
  }
  Serial.println("Going to deep sleep...");
  esp_deep_sleep_start();
}
