#include <EEPROM.h>

// ---------------- CONFIG ----------------
#define RELAY_START 5
#define RELAY_STOP  18

#define SIM800_TX 17
#define SIM800_RX 16
#define SIM800_RST 4

#define STARTER_DELAY 1500

#define EEPROM_SIZE 512
#define EEPROM_MOTOR_STATE 500   // ✅ motor state storage

#define MAX_NUMBERS 10
#define NUMBER_LENGTH 15

#define SMS_VALID_WINDOW 120000

HardwareSerial sim800(2);

// ---------------- ADMIN ----------------
String adminNumber = "+919443275150";

// ---------------- VARIABLES ----------------
String smsBuffer = "";
String senderNumber = "";
String callNumber = "";

bool newSMS = false;
bool incomingCall = false;
bool motorState = false;

unsigned long smsReceiveMillis = 0;

String storedNumbers[MAX_NUMBERS];

// ---------------- EEPROM STATE ----------------
void saveMotorState(bool state) {
  EEPROM.write(EEPROM_MOTOR_STATE, state ? 1 : 0);
  EEPROM.commit();
}

bool loadMotorState() {
  return EEPROM.read(EEPROM_MOTOR_STATE) == 1;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_START, OUTPUT);
  pinMode(RELAY_STOP, OUTPUT);
  pinMode(SIM800_RST, OUTPUT);

  digitalWrite(RELAY_START, HIGH);
  digitalWrite(RELAY_STOP, HIGH);
  digitalWrite(SIM800_RST, HIGH);

  sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);

  EEPROM.begin(EEPROM_SIZE);
  loadNumbers();

  // ✅ LOAD PREVIOUS STATE
  motorState = loadMotorState();

  delay(3000);

  sendAT("AT");
  sendAT("AT+CNMP=13");
  sendAT("AT+CMNB=1");

  sendAT("AT+CMGF=1");
  sendAT("AT+CNMI=1,2,0,0,0");
  sendAT("AT+CLIP=1");
  sendAT("AT+CMGD=1,4");

  Serial.println("System Ready...");

  // ✅ RESTORE MOTOR STATE AFTER BOOT
  if (motorState) {
    Serial.println("Restoring Motor ON...");
    delay(5000);   // allow power stabilize
    triggerStart();   // only pulse
  }
}

// ---------------- LOOP ----------------
void loop() {
  ensureSIMWorking();

  while (sim800.available()) {
    char c = sim800.read();
    smsBuffer += c;

    if (c == '\n') {
      smsBuffer.trim();

      Serial.println("RAW:");
      Serial.println(smsBuffer);

      if (smsBuffer.startsWith("RING")) {
        incomingCall = true;
      }
      else if (smsBuffer.startsWith("+CLIP:")) {
        extractCallNumber(smsBuffer);
        if (incomingCall) {
          handleCall();
          incomingCall = false;
        }
      }
      else if (smsBuffer.startsWith("+CMT:")) {
        extractSender(smsBuffer);
        newSMS = true;
        smsReceiveMillis = millis();
      }
      else if (newSMS) {
        processSMS(smsBuffer);
        newSMS = false;
      }

      smsBuffer = "";
    }
  }
}

// ---------------- NORMALIZE ----------------
String normalizeNumber(String num) {
  num.trim();
  num.replace("\"", "");
  num.replace(",", "");

  if (num.startsWith("91") && !num.startsWith("+91")) {
    num = "+" + num;
  }

  return num;
}

// ---------------- EEPROM NUMBERS ----------------
void loadNumbers() {
  int addr = 0;

  for (int i = 0; i < MAX_NUMBERS; i++) {
    storedNumbers[i] = "";

    for (int j = 0; j < NUMBER_LENGTH; j++) {
      char c = EEPROM.read(addr++);
      if (c >= 32 && c <= 126) storedNumbers[i] += c;
    }

    storedNumbers[i].trim();
  }
}

void saveNumbers() {
  int addr = 0;

  for (int i = 0; i < MAX_NUMBERS; i++) {
    for (int j = 0; j < NUMBER_LENGTH; j++) {
      if (j < storedNumbers[i].length())
        EEPROM.write(addr++, storedNumbers[i][j]);
      else
        EEPROM.write(addr++, 0);
    }
  }

  EEPROM.commit();
}

// ---------------- ACCESS ----------------
bool isAllowed(String number) {
  number = normalizeNumber(number);

  if (number == adminNumber) return true;

  for (int i = 0; i < MAX_NUMBERS; i++) {
    if (storedNumbers[i] == number) return true;
  }
  return false;
}

// ---------------- SMS ----------------
bool isRecentSMS() {
  return (millis() - smsReceiveMillis) <= SMS_VALID_WINDOW;
}

void processSMS(String data) {

  data.trim();
  data.toUpperCase();

  if (!isRecentSMS()) {
    sendSMS(senderNumber, "Expired SMS");
    return;
  }

  if (!isAllowed(senderNumber)) return;

  if (data == "ON") {
    if (!motorState) {
      triggerStart();
      motorState = true;
      saveMotorState(true);
      sendSMS(senderNumber, "Motor Started");
    } else {
      sendSMS(senderNumber, "Already ON");
    }
  }
  else if (data == "OFF") {
    triggerStop();
    motorState = false;
    saveMotorState(false);
    sendSMS(senderNumber, "Motor Stopped");
  }
  else {
    sendSMS(senderNumber, "Invalid Command");
  }
}

// ---------------- CALL ----------------
void handleCall() {
  sim800.println("ATH");
  delay(1000);

  if (!isAllowed(callNumber)) return;

  if (!motorState) {
    triggerStart();
    motorState = true;
    saveMotorState(true);
    sendSMS(callNumber, "Motor Started (Call)");
  } else {
    triggerStop();
    motorState = false;
    saveMotorState(false);
    sendSMS(callNumber, "Motor Stopped (Call)");
  }
}

// ---------------- RELAY CONTROL ----------------
void triggerStart() {
  digitalWrite(RELAY_START, LOW);
  // delay(STARTER_DELAY);
  // digitalWrite(RELAY_START, HIGH);
}

void triggerStop() {
  digitalWrite(RELAY_STOP, LOW);
  delay(STARTER_DELAY);
  digitalWrite(RELAY_STOP, HIGH);
}

// ---------------- SMS SEND ----------------
void sendSMS(String number, String msg) {
  sim800.println("AT+CMGF=1");
  delay(500);

  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");

  delay(500);
  sim800.print(msg);
  delay(500);
  sim800.write(26);
  delay(3000);
}

// ---------------- SIM ----------------
bool checkSIM() {
  sim800.println("AT");
  delay(1000);
  return sim800.available();
}

void resetSIM800() {
  digitalWrite(SIM800_RST, LOW);
  delay(200);
  digitalWrite(SIM800_RST, HIGH);
  delay(5000);
}

void ensureSIMWorking() {
  static unsigned long lastCheck = 0;

  if (millis() - lastCheck > 30000) {
    lastCheck = millis();
    if (!checkSIM()) resetSIM800();
  }
}

// ---------------- AT ----------------
void sendAT(String cmd) {
  sim800.println(cmd);
  delay(1000);

  while (sim800.available()) Serial.write(sim800.read());
}
