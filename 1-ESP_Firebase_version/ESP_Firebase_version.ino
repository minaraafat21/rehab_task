#include <WiFi.h>
#include <FirebaseESP32.h> 
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <addons/TokenHelper.h> 

// =========================================
// 1. NETWORK & FIREBASE CREDENTIALS
// =========================================
#define WIFI_SSID "Orange-207C"
#define WIFI_PASSWORD "BQED6B14B91"

#define API_KEY "AIzaSyAusWKwJZgqoRC8xY-uBs6vazBQH2ULfI8" 
#define USER_EMAIL "prosthetic@test.com"
#define USER_PASSWORD "12345678"
#define DATABASE_URL "rehab-project-1135b-default-rtdb.firebaseio.com"

// =========================================
// 2. SENSORS & GLOBALS
// =========================================
Adafruit_MPU6050 mpu;

// ---> FIX 1: Two separate Firebase objects to prevent collisions <---
FirebaseData fbdo_read;
FirebaseData fbdo_write;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long printPrevMillis = 0; // New timer just for a clean Serial Monitor
int current_prediction = -1; 

const char* classNames[] = {"Sitting", "Standing", "Walking", "Running", "Stairs Up", "Stairs Down", "Falling"};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("   PROSTHETIC LEG IMU MONITORING");
  Serial.println("========================================\n");

  // Initialize MPU6050
  Wire.begin(21, 20);
  if (!mpu.begin()) {
    Serial.println("Trying alt I2C pins 8,9...");
    Wire.begin(8, 9);
    if (!mpu.begin()) {
      Serial.println("Failed to find MPU6050 chip!");
      while (1) { delay(10); }
    }
  }
  Serial.println("MPU6050 Ready");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initialize WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Initialize Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Connected!\n");
}

void loop() {
  // =========================================
  // TASK 1: READ & UPLOAD (Every 200ms)
  // =========================================
  if (millis() - sendDataPrevMillis > 200) {
    sendDataPrevMillis = millis();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // READ the AI prediction using the dedicated READ object
    if (Firebase.ready()) {
      if (Firebase.getInt(fbdo_read, "/sensors/esp32_device/prediction")) {
        current_prediction = fbdo_read.intData();
      }
    }

    // UPLOAD the sensor data using the dedicated WRITE object
    if (Firebase.ready()) {
      FirebaseJson content;
      content.set("accel_x", a.acceleration.x);
      content.set("accel_y", a.acceleration.y);
      content.set("accel_z", a.acceleration.z);
      content.set("gyro_x", g.gyro.x);
      content.set("gyro_y", g.gyro.y);
      content.set("gyro_z", g.gyro.z);
      
      if (Firebase.updateNode(fbdo_write, "/sensors/esp32_device/imu", content)) {
         // ---> FIX 2: We replaced the spammy text with a single subtle dot <---
         Serial.print("."); 
      }
    }
  }

  // =========================================
  // TASK 2: CLEAN SERIAL MONITOR (Every 2 Seconds)
  // =========================================
  if (millis() - printPrevMillis > 2000) {
    printPrevMillis = millis();
    
    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║          LIVE PATIENT STATUS           ║");
    Serial.println("╠════════════════════════════════════════╣");
    
    Serial.print("║ AI Prediction: ");
    if(current_prediction >= 0 && current_prediction <= 6) {
      Serial.print(classNames[current_prediction]);
      for(int i = String(classNames[current_prediction]).length(); i < 23; i++) Serial.print(" ");
    } else {
      Serial.print("Waiting for data...");
      for(int i = 19; i < 23; i++) Serial.print(" ");
    }
    Serial.println("║");
    Serial.println("╚════════════════════════════════════════╝");
  }
}