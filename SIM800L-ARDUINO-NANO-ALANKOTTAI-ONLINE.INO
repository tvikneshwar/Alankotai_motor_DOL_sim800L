// CODED BY VIKNESHWAR THANDESWARAN 
// 25-09-2021 GANAPATHIPURAM 

#include <SoftwareSerial.h>
#define STARTER_DELAY 2000

// Configure software serial port
SoftwareSerial SIM900(3, 2);

// Variable to store text message
String textMessage;

// Create a variable to store Lamp state
String lampState = "HIGH";

// Relay connected to pin 12
const int relay = A0;
const int relay2 = A1;

void setup() {
  // Automatically turn on the shield
  //digitalWrite(9, HIGH);
  //delay(1000);
  //digitalWrite(9, LOW);
   delay(5000);
  
  // Set relay as OUTPUT
  pinMode(relay, OUTPUT);
   pinMode(relay2, OUTPUT);

  // By default the relay is off
  digitalWrite(relay, HIGH);
  digitalWrite(relay2, HIGH);
  // Initializing serial commmunication
  Serial.begin(19200); 
  SIM900.begin(19200);

  // Give time to your GSM shield log on to network
  delay(20000);
  Serial.print("SIM800 ready...");

  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);
  // Set module to send SMS data to serial out upon receipt 
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);
}

void loop(){
  if(SIM900.available()>0){
    textMessage = SIM900.readString();
    Serial.print(textMessage);    
    delay(10);
  } 

  //*******************************starter on command***************************
  if(textMessage.indexOf("ON")>=0){
    // Turn on relay and save current state
    digitalWrite(relay, LOW);
    lampState = "on";
    Serial.println("Relay set to ON"); 
    delay(STARTER_DELAY); 
    digitalWrite(relay, HIGH);

        
    textMessage = "";   
  }

//****************************starter off command*******************************


  if(textMessage.indexOf("OFF")>=0){
    // Turn off relay and save current state
    digitalWrite(relay2,LOW );
    lampState = "off"; 
    Serial.println("Relay set to OFF");
    delay(STARTER_DELAY);
    digitalWrite(relay2,HIGH );

    textMessage = ""; 
  }



//******************************sTATUS***********************************************


  if(textMessage.indexOf("STATE")>=0){
    String message = "Lamp is " + lampState;
    sendSMS(message);
    Serial.println("Lamp state resquest");
    textMessage = "";
  }


//******************************************************************************


}  

// Function that sends SMS
void sendSMS(String message){
  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r"); 
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.println("AT + CMGS = \"+917810010503\""); 
  delay(100);
  // Send the SMS
  SIM900.println(message); 
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26); 
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000);  
}
