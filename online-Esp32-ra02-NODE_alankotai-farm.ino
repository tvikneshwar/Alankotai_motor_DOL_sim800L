/* 
🌾 LoRa Farm Motor Node (ESP32 + Ra-02)
Node Identity: alankotai-farm (Gateway Compatible)
*/

#include <SPI.h>
#include <LoRa.h>

// Unique Identifier for your farm
const String baseNodeID = "alankotai-farm"; 
// Combined string to instantly satisfy Gateway filter: if (receivedMessage.startsWith("NODE_"))
String nodeID = "NODE_" + baseNodeID; 

// Hardware Pin Definitions (Matching your Gateway setup)
#define LORA_CS    4
#define HW_RST    14
#define LORA_DIO0  2

// Farm Starter Relay Pins (From your GSM code)
#define RELAY_START 5
#define RELAY_STOP  18
#define STARTER_DELAY 1500 // 1.5-second pulse for industrial starters

// Variables
bool motorState = false;
unsigned long lastReportTime = 0;
const unsigned long reportInterval = 10000; // Send status every 10 seconds

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n--- Farm LoRa Motor Node Booting ---");
    Serial.print("Node Wire Identity: ");
    Serial.println(nodeID);

    // Configure Starter Pins (Active LOW relay blocks setup)
    pinMode(RELAY_START, OUTPUT);
    pinMode(RELAY_STOP, OUTPUT);
    digitalWrite(RELAY_START, HIGH); // Default Open Circuit
    digitalWrite(RELAY_STOP, HIGH);  // Default Open Circuit

    // Initialize LoRa Radio
    Serial.println("[LoRa] Initializing Transceiver...");
    LoRa.setPins(LORA_CS, HW_RST, LORA_DIO0);
    
    // Must match the Gateway frequency (433 MHz)
    if (!LoRa.begin(433E6)) { 
        Serial.println("[LoRa] Hardware initialization failed! Check wiring.");
        while (1); 
    }
    
    // Fine-tuned Radio Parameters (EXACT match to your gateway for communication compatibility)
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN); // Max power for outdoor range
    LoRa.setSignalBandwidth(62.5E3);  
    LoRa.setSpreadingFactor(12);      
    LoRa.setCodingRate4(8);           
    LoRa.setGain(6);                  
    
    Serial.println("[LoRa] Node is online, listening for gateway instructions...");
}

void loop() {
    // 1. Check for incoming payloads from the gateway
    parseIncomingLoRa();

    // 2. Periodically transmit status telemetry string (Uplink)
    if (millis() - lastReportTime >= reportInterval) {
        lastReportTime = millis();
        sendTelemetry();
    }
}

// Process structural inbound traffic from Gateway
void parseIncomingLoRa() {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return; // No packets received

    String incomingMessage = "";
    while (LoRa.available()) {
        incomingMessage += (char)LoRa.read();
    }

    Serial.print("[LoRa Inbound] Command Received: ");
    Serial.println(incomingMessage);

    // Case A: Remote ID Re-configuration (CONFIG:CurrentID:NewID)
    if (incomingMessage.startsWith("CONFIG:")) {
        int firstColon = incomingMessage.indexOf(':');
        int secondColon = incomingMessage.indexOf(':', firstColon + 1);
        
        if (secondColon != -1) {
            String targetNode = incomingMessage.substring(firstColon + 1, secondColon);
            String newID = incomingMessage.substring(secondColon + 1);

            if (targetNode == nodeID) {
                Serial.println("[Config] Changing identity to: " + newID);
                sendAck(); // Stop Gateway retry loops instantly before change
                nodeID = newID; 
            }
        }
        return;
    }

    // Case B: General Motor Control Commands
    // Expected incoming strings via your gateway's control topic: "ON" or "OFF"
    if (incomingMessage == "ON") {
        if (!motorState) {
            startMotor();
        } else {
            Serial.println("[Motor] Already ON.");
        }
        sendAck(); // Immediately fire back ACK packet
    } 
    else if (incomingMessage == "OFF") {
        if (motorState) {
            stopMotor();
        } else {
            Serial.println("[Motor] Already OFF.");
        }
        sendAck(); // Immediately fire back ACK packet
    }
}

// Transmit Acknowledgement back to the Gateway
void sendAck() {
    delay(100); // Small turnaround delay for radio stabilization
    LoRa.beginPacket();
    LoRa.print(nodeID + ":ACK");
    LoRa.endPacket();
    Serial.println("[LoRa Outbound] Acknowledgment dispatched.");
}

// Send status telemetry (Uplink) to MQTT broker via Gateway
void sendTelemetry() {
    // Construct payload starting with NODE_alankotai-farm
    // Will safely publish on MQTT as: NODE_alankotai-farm:STATUS=ON or OFF
    String telemetryPayload = nodeID + ":STATUS=" + (motorState ? "ON" : "OFF");

    Serial.print("[LoRa Outbound] Transmitting Status: ");
    Serial.println(telemetryPayload);

    LoRa.beginPacket();
    LoRa.print(telemetryPayload);
    LoRa.endPacket();
}

// Dual-Relay Pulse Starter Control Logic
void startMotor() {
    Serial.println("[Action] Pulsing START Relay...");
    digitalWrite(RELAY_START, LOW);   // Press Green Button
    delay(STARTER_DELAY);
    digitalWrite(RELAY_START, HIGH);  // Release Green Button
    motorState = true;
    Serial.println("[Motor] Status set to ON");
}

void stopMotor() {
    Serial.println("[Action] Pulsing STOP Relay...");
    digitalWrite(RELAY_STOP, LOW);    // Press Red Button
    delay(STARTER_DELAY);
    digitalWrite(RELAY_STOP, HIGH);   // Release Red Button
    motorState = false;
    Serial.println("[Motor] Status set to OFF");
}
