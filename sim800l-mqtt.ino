// refrence link: http://www.raviyp.com/arduino-mqtt-library-with-publish-and-subscribe-example/


#include <SoftwareSerial.h>
#define STARTER_DELAY 2000

// Configure software serial port
SoftwareSerial SIM900(3, 2);

// Relay connected to pin 12
const int relay = A0;
const int relay2 = A1;

int led = 13;
unsigned int Counter = 0;
unsigned long datalength, CheckSum, RLength;
unsigned short topiclength;
unsigned char topic[30];
char str[250];
unsigned char encodedByte;
int X;
unsigned short MQTTProtocolNameLength;
unsigned short MQTTClientIDLength;
unsigned short MQTTUsernameLength;
unsigned short MQTTPasswordLength;
const char MQTTHost[30] = "broker.hivemq.com";
const char MQTTPort[10] = "1883";
const char MQTTClientID[20] = "ABCDEF";
const char MQTTTopic[30] = "viky";
const char MQTTProtocolName[10] = "MQTT";
const char MQTTLVL = 0x03;
const char MQTTFlags = 0xC2;
const unsigned int MQTTKeepAlive = 60;
const char MQTTUsername[30] = "";
const char MQTTPassword[35] = "";
const char MQTTQOS = 0x00;
const char MQTTPacketID = 0x0001;
void setup() {
  pinMode(led, OUTPUT);
   digitalWrite(relay, HIGH);
  digitalWrite(relay2, HIGH);

  Serial.begin(9600);
  SIM900.begin(19200);
  delay(3000);
}
void SendConnectPacket(void) {

  
  SIM900.print("\r\nAT+CIPSEND\r\n");
  delay(3000);
  SIM900.write(0x10);
  MQTTProtocolNameLength = strlen(MQTTProtocolName);
  MQTTClientIDLength = strlen(MQTTClientID);
  MQTTUsernameLength = strlen(MQTTUsername);
  MQTTPasswordLength = strlen(MQTTPassword);
  datalength = MQTTProtocolNameLength + 2 + 4 + MQTTClientIDLength + 2 + MQTTUsernameLength + 2 + MQTTPasswordLength + 2;
  X = datalength;
  do {
    encodedByte = X % 128;
    X = X / 128;
    if (X > 0) {
      encodedByte |= 128;
    }
    SIM900.write(encodedByte);
  }
  while (X > 0);
  SIM900.write(MQTTProtocolNameLength >> 8);
  SIM900.write(MQTTProtocolNameLength & 0xFF);
  SIM900.print(MQTTProtocolName);
  SIM900.write(MQTTLVL); // LVL
  SIM900.write(MQTTFlags); // Flags
  SIM900.write(MQTTKeepAlive >> 8);
  SIM900.write(MQTTKeepAlive & 0xFF);
  SIM900.write(MQTTClientIDLength >> 8);
  SIM900.write(MQTTClientIDLength & 0xFF);
  SIM900.print(MQTTClientID);
  SIM900.write(MQTTUsernameLength >> 8);
  SIM900.write(MQTTUsernameLength & 0xFF);
  SIM900.print(MQTTUsername);
  SIM900.write(MQTTPasswordLength >> 8);
  SIM900.write(MQTTPasswordLength & 0xFF);
  SIM900.print(MQTTPassword);
  SIM900.write(0x1A);
}
void SendPublishPacket(void) {
  SIM900.print("\r\nAT+CIPSEND\r\n");
  delay(3000);
  memset(str, 0, 250);
  topiclength = sprintf((char * ) topic, MQTTTopic);
  datalength = sprintf((char * ) str, "%s%u", topic, Counter);
  delay(1000);
  SIM900.write(0x30);
  X = datalength + 2;
  do {
    encodedByte = X % 128;
    X = X / 128;
    if (X > 0) {
      encodedByte |= 128;
    }
    SIM900.write(encodedByte);
  }
  while (X > 0);
  SIM900.write(topiclength >> 8);
  SIM900.write(topiclength & 0xFF);
  SIM900.print(str);
  SIM900.write(0x1A);
}
void SendSubscribePacket(void) {
  SIM900.print("\r\nAT+CIPSEND\r\n");
  delay(3000);
  memset(str, 0, 250);
  topiclength = strlen(MQTTTopic);
  datalength = 2 + 2 + topiclength + 1;
  delay(1000);
  SIM900.write(0x82);
  X = datalength;
  do {
    encodedByte = X % 128;
    X = X / 128;
    if (X > 0) {
      encodedByte |= 128;
    }
    SIM900.write(encodedByte);
  }
  while (X > 0);
  SIM900.write(MQTTPacketID >> 8);
  SIM900.write(MQTTPacketID & 0xFF);
  SIM900.write(topiclength >> 8);
  SIM900.write(topiclength & 0xFF);
  SIM900.print(MQTTTopic);
  SIM900.write(MQTTQOS);
  SIM900.write(0x1A);
}
void loop() {
  SIM900.print("AT+CIPSHUT\r\n");
  delay(2000);
  SIM900.print("AT+CSTT=\"www\",\"\",\"\"\r\n");
  delay(1000);
  SIM900.print("AT+CIPMODE=0\r\n");
  delay(1000);
  SIM900.print("AT+CIICR\r\n");
  delay(9000);
  SIM900.print("AT+CIPSTART=\"TCP\",\"broker.hivemq.com\",\"1883\"\r\n");
  delay(6000);
  SendConnectPacket();
  delay(5000);
  SendSubscribePacket();
  delay(5000);
  while (1) {
    if (Serial.available() > 0) {
      str[0] = Serial.read();
      SIM900.write(str[0]);
      if (str[0] == 'N')
        digitalWrite(led, HIGH);
      if (str[0] == 'F')
        digitalWrite(led, LOW);
    }
  }
}
