#include <WiFi.h>
#include <FirebaseESP32.h> 
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <addons/TokenHelper.h> 

// --- 1. NETWORK CREDENTIALS ---
#define WIFI_SSID "STUDBME2"
#define WIFI_PASSWORD "BME2Stud"

#define API_KEY "" 
#define USER_EMAIL ""
#define USER_PASSWORD ""
#define DATABASE_URL ""

// --- 2. SENSORS ---
Adafruit_MPU6050 mpu;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("   ESP32 MPU6050 MONITORING SYSTEM");
  Serial.println("========================================\n");

  // Try I2C on default pins, then S3 specific pins
  Wire.begin(21, 20);
  if (!mpu.begin()) {
    Serial.println("Trying alt I2C pins 8,9...");
    Wire.begin(8, 9);
    if (!mpu.begin()) {
      Serial.println("Failed to find MPU6050 chip!");
      while (1) {
        delay(10);
      }
    }
  }
  Serial.println("MPU6050 Ready");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("   IP Address: ");
  Serial.println(WiFi.localIP());

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Connected!\n");
  
  Serial.println("========================================");
  Serial.println("   SYSTEM READY - Monitoring Started");
  Serial.println("========================================\n");
}

void loop() {
  // Update every 200 milliseconds
  if (millis() - sendDataPrevMillis > 200) {
    sendDataPrevMillis = millis();

    // =========================================
    // 1. MPU6050 DATA READ
    // =========================================
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // =========================================
    // 2. PRINT STATUS TO SERIAL MONITOR
    // =========================================
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║         SYSTEM STATUS REPORT           ║");
    Serial.println("╠════════════════════════════════════════╣");
    
    Serial.print("║ Accel X: ");
    Serial.print(a.acceleration.x, 2);
    Serial.print(" m/s²");
    for(int i = String(a.acceleration.x, 2).length() + 5; i < 28; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Accel Y: ");
    Serial.print(a.acceleration.y, 2);
    Serial.print(" m/s²");
    for(int i = String(a.acceleration.y, 2).length() + 5; i < 28; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Accel Z: ");
    Serial.print(a.acceleration.z, 2);
    Serial.print(" m/s²");
    for(int i = String(a.acceleration.z, 2).length() + 5; i < 28; i++) Serial.print(" ");
    Serial.println("║");

    Serial.print("║ Gyro X:  ");
    Serial.print(g.gyro.x, 2);
    Serial.print(" rad/s");
    for(int i = String(g.gyro.x, 2).length() + 6; i < 28; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.println("╚════════════════════════════════════════╝\n");

    // =========================================
    // 3. UPLOAD TO FIREBASE
    // =========================================
    if (Firebase.ready()) {
      FirebaseJson content;
      
      content.set("accel_x", a.acceleration.x);
      content.set("accel_y", a.acceleration.y);
      content.set("accel_z", a.acceleration.z);
      content.set("gyro_x", g.gyro.x);
      content.set("gyro_y", g.gyro.y);
      content.set("gyro_z", g.gyro.z);
      
      if (Firebase.updateNode(fbdo, "/sensors/esp32_device", content)) {
        Serial.println("Data uploaded to Firebase successfully");
      } else {
        Serial.println("Firebase upload failed");
        Serial.print("   Error: ");
        Serial.println(fbdo.errorReason());
      }
    }
    
    Serial.println("----------------------------------------\n");
  }
}