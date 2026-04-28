// #include <WiFi.h>
// #include <WiFiUdp.h>

// // WiFi credentials
// const char* ssid = "STUDBME2";
// const char* password = "BME2Stud";

// // Network destination (MUST MATCH YOUR PYTHON COMPUTER'S IP)
// const char* targetIP = "192.168.1.100"; 
// const int targetPort = 8080;            

// WiFiUDP udp;

// // Hardware Serial 2 pins for ESP32
// #define RXD2 16
// #define TXD2 17

// // IMU Data Storage
// float ax = 0.0, ay = 0.0, az = 0.0;
// float gx = 0.0, gy = 0.0, gz = 0.0;

// void setup() {
//     Serial.begin(115200);      
//     Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); 
    
//     Serial.println("\nConnecting to WiFi...");
//     WiFi.begin(ssid, password);
    
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
    
//     Serial.println("\nWiFi connected.");
//     Serial.print("ESP32 IP Address: ");
//     Serial.println(WiFi.localIP());

//     // Start listening for incoming UDP packets on a local port
//     udp.begin(8080);
// }

// void loop() {
//     // 1. Maintain WiFi connection
//     if (WiFi.status() != WL_CONNECTED) {
//         WiFi.begin(ssid, password);
//         return; 
//     }

//     // 2. Read incoming serial data from Arduino
//     while (Serial2.available() > 0) {
//         String incomingString = Serial2.readStringUntil('\n');
//         incomingString.trim(); 
        
//         if (incomingString.length() > 0) {
//             int parsed = sscanf(incomingString.c_str(), "%f,%f,%f,%f,%f,%f", &ax, &ay, &az, &gx, &gy, &gz);
            
//             if (parsed == 6) {
//                 // 3. Transmit the valid data over WiFi via UDP
//                 sendUDPData(incomingString);
//             }
//         }
//     }

//     // 4. Listen for prediction from Python and send to Arduino
//     int packetSize = udp.parsePacket();
//     if (packetSize) {
//         String prediction = udp.readString();
//         prediction.trim();
        
//         // Pass the prediction directly to the Arduino over Hardware Serial
//         Serial2.println(prediction);
        
//         // Echo to USB Serial for debugging
//         Serial.println("Received Prediction & Sent to Arduino: " + prediction);
//     }
// }

// void sendUDPData(String dataPayload) {
//     udp.beginPacket(targetIP, targetPort);
//     udp.print(dataPayload);
//     udp.endPacket();
// }

#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi credentials
// const char* ssid = "STUDBME2";
// const char* password = "BME2Stud";

const char* ssid = "mina,sara";
const char* password = "samsung123";

// Network destination (MUST MATCH YOUR PYTHON COMPUTER'S IP)
// const char* targetIP = "172.28.128.103"; 
const char* targetIP = "192.168.1.16"; 
const int targetPort = 8080;         
   

WiFiUDP udp;

// Hardware Serial 2 pins for ESP32
#define RXD2 16
#define TXD2 17

// IMU Data Storage
float ax = 0.0, ay = 0.0, az = 0.0;
float gx = 0.0, gy = 0.0, gz = 0.0;

void setup() {
    Serial.begin(115200);      
    
    // NOTE: If you receive garbage data, lower BOTH this and the Arduino to 38400
    // Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); 
    
    Serial.println("\nConnecting to WiFi...");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected.");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Start listening for incoming UDP packets on a local port
    udp.begin(8080);
}

void loop() {
    // 1. Maintain WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
        return; 
    }

    // 2. Read incoming serial data from Arduino
    while (Serial.available() > 0) {
        String incomingString = Serial.readStringUntil('\n');
        incomingString.trim(); 
        
        // ==========================================
        // DEBUG: Print EXACTLY what the ESP32 hears
        // ==========================================
        Serial.println("RAW: [" + incomingString + "]");
        
        if (incomingString.length() > 0) {
            int parsed = sscanf(incomingString.c_str(), "%f,%f,%f,%f,%f,%f", &ax, &ay, &az, &gx, &gy, &gz);
            
            if (parsed == 6) {
                // Transmit the valid data over WiFi via UDP
                sendUDPData(incomingString);
                Serial.println("SUCCESS: Sent 6 values over UDP.");
            } else {
                Serial.println("ERROR: Failed to parse 6 values. Found: " + String(parsed));
            }
        }
    }

    // 3. Listen for prediction from Python and send to Arduino
    int packetSize = udp.parsePacket();
    if (packetSize) {
        String prediction = udp.readString();
        prediction.trim();
        
        // Pass the prediction with a prefix 'P' so Arduino can identify it
        // Serial.print('P'); 
        Serial.println(prediction);
        
        // Optional debug for PC
        Serial.println("DEBUG: Sent P" + prediction + " to Arduino");
    }
}

void sendUDPData(String dataPayload) {
    udp.beginPacket(targetIP, targetPort);
    udp.print(dataPayload);
    udp.endPacket();
}