#include <EEPROM.h>

// ---------------- CONFIG ----------------
#define RELAY_START 5
#define RELAY_STOP  18

#define SIM800_TX 17
#define SIM800_RX 16
#define SIM800_RST 4

#define STARTER_DELAY 1500

#define EEPROM_SIZE 512
#define MAX_NUMBERS 10
#define NUMBER_LENGTH 15

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

String storedNumbers[MAX_NUMBERS];

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
  loadNumbers();   // ✅ Load stored numbers (no clearing)

  delay(3000);

  // -------- SIM INIT --------
  sendAT("AT");
  sendAT("AT+CNMP=13");
  sendAT("AT+CMNB=1");

  sendAT("AT+CMGF=1");
  sendAT("AT+CNMI=1,2,0,0,0");
  sendAT("AT+CLIP=1");

  sendAT("AT+CMGD=1,4");

  Serial.println("System Ready...");
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

      // -------- CALL --------
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

      // -------- SMS --------
      else if (smsBuffer.startsWith("+CMT:")) {
        extractSender(smsBuffer);
        newSMS = true;
      }
      else if (newSMS) {
        processSMS(smsBuffer);
        newSMS = false;
      }

      smsBuffer = "";
    }
  }
}

// ---------------- NORMALIZE NUMBER ----------------
String normalizeNumber(String num) {
  num.trim();
  num.replace("\"", "");
  num.replace(",", "");

  if (num.startsWith("91") && !num.startsWith("+91")) {
    num = "+" + num;
  }

  return num;
}

// ---------------- EEPROM LOAD ----------------
void loadNumbers() {
  int addr = 0;

  for (int i = 0; i < MAX_NUMBERS; i++) {
    storedNumbers[i] = "";

    for (int j = 0; j < NUMBER_LENGTH; j++) {
      char c = EEPROM.read(addr++);

      if (c >= 32 && c <= 126) {
        storedNumbers[i] += c;
      }
    }

    storedNumbers[i].trim();

    Serial.println("Slot " + String(i) + ": " + storedNumbers[i]);
  }
}

// ---------------- EEPROM SAVE ----------------
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

// ---------------- ACCESS CONTROL ----------------
bool isAllowed(String number) {
  number = normalizeNumber(number);

  if (number == adminNumber) return true;

  for (int i = 0; i < MAX_NUMBERS; i++) {
    if (storedNumbers[i] == number) return true;
  }
  return false;
}

// ---------------- ADD ----------------
bool addNumber(String num) {
  num = normalizeNumber(num);

  for (int i = 0; i < MAX_NUMBERS; i++) {
    if (storedNumbers[i] == num) return false;
  }

  for (int i = 0; i < MAX_NUMBERS; i++) {
    if (storedNumbers[i].length() == 0) {
      storedNumbers[i] = num;
      saveNumbers();
      return true;
    }
  }

  return false;
}

// ---------------- DELETE ----------------
bool deleteNumber(String num) {
  num = normalizeNumber(num);

  for (int i = 0; i < MAX_NUMBERS; i++) {
    if (storedNumbers[i] == num) {
      storedNumbers[i] = "";
      saveNumbers();
      return true;
    }
  }

  return false;
}

// ---------------- EXTRACT ----------------
void extractSender(String data) {
  int f = data.indexOf("\"");
  int s = data.indexOf("\"", f + 1);

  if (f != -1 && s != -1) {
    senderNumber = normalizeNumber(data.substring(f + 1, s));
    Serial.println("SMS From: " + senderNumber);
  }
}

void extractCallNumber(String data) {
  int f = data.indexOf("\"");
  int s = data.indexOf("\"", f + 1);

  if (f != -1 && s != -1) {
    callNumber = normalizeNumber(data.substring(f + 1, s));
    Serial.println("Call From: " + callNumber);
  }
}

// ---------------- SMS PROCESS ----------------
void processSMS(String data) {

  data.trim();
  data.toUpperCase();

  // -------- ADMIN --------
  if (senderNumber == adminNumber) {

    if (data.startsWith("ADD,")) {
      String num = data.substring(4);
      sendSMS(senderNumber, addNumber(num) ? "Added" : "Add Failed");
      return;
    }

    if (data.startsWith("DEL,")) {
      String num = data.substring(4);
      sendSMS(senderNumber, deleteNumber(num) ? "Deleted" : "Delete Failed");
      return;
    }

    if (data == "LIST") {
      String msg = "Numbers:\n";

      for (int i = 0; i < MAX_NUMBERS; i++) {
        if (storedNumbers[i] != "")
          msg += storedNumbers[i] + "\n";
      }

      sendSMS(senderNumber, msg);
      return;
    }
  }

  if (!isAllowed(senderNumber)) {
    Serial.println("Unauthorized SMS");
    return;
  }

  if (data == "ON") {
    startMotor();
    sendSMS(senderNumber, "Motor Started");
  }
  else if (data == "OFF") {
    stopMotor();
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
    startMotor();
    sendSMS(callNumber, "Motor Started (Call)");
  } else {
    stopMotor();
    sendSMS(callNumber, "Motor Stopped (Call)");
  }
}

// ---------------- MOTOR ----------------
void startMotor() {
  digitalWrite(RELAY_START, LOW);
  delay(STARTER_DELAY);
  digitalWrite(RELAY_START, HIGH);
  motorState = true;
}

void stopMotor() {
  digitalWrite(RELAY_STOP, LOW);
  delay(STARTER_DELAY);
  digitalWrite(RELAY_STOP, HIGH);
  motorState = false;
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

// ---------------- SIM HEALTH ----------------
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

  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}
