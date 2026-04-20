#include <Servo.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
Servo myServo;

const int servoPin = 9;

void setup() {
    // Hardware Serial handles both USB Monitor and ESP32 communication
    Serial.begin(115200); 
    myServo.attach(servoPin);

    // Initialize MPU-6050
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }

    // Configure sensor ranges
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {

    myServo.write(60);
    delay(1000);
    // 1. Read IMU Data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // 2. Transmit data over Hardware Serial (Sends to ESP32 and USB Monitor)
    Serial.print(a.acceleration.x); Serial.print(",");
    Serial.print(a.acceleration.y); Serial.print(",");
    Serial.print(a.acceleration.z); Serial.print(",");
    Serial.print(g.gyro.x); Serial.print(",");
    Serial.print(g.gyro.y); Serial.print(",");
    Serial.println(g.gyro.z);

    // 3. Receive Classification over Hardware Serial
    if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'P') {
            // Give the buffer a tiny moment to receive the actual number after 'P'
            delay(200); 
            int predictedClass = Serial.parseInt();
            
            // Critical: Only update if a valid number was parsed
            updateServoPosition(predictedClass);
            
            // Flush remaining characters (like \n)
            while (Serial.available() > 0) { Serial.read(); }

             myServo.write(60);
             delay(1000); 
        }
    }
    
    // 50Hz sampling rate (20ms delay)
    delay(20); 
}

void updateServoPosition(int movementClass) {
    int targetAngle;
    
    switch(movementClass) {
        case 0: targetAngle = 0; break;   // Sitting
        case 1: targetAngle = 90; break;  // Standing
        case 2: targetAngle = 60; break;  // Walking
        case 3: targetAngle = 90; break;  // Ramp Ascend
        case 4: targetAngle = 120; break; // Ramp Descend
        case 5: targetAngle = 150; break; // Stair Ascend
        case 6: targetAngle = 180; break; // Stair Descend
        default: return; // Invalid class, do nothing
    }
    
    myServo.write(targetAngle);
}