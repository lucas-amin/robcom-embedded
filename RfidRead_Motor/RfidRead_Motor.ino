   /*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read data from a PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
   Reader on the Arduino SPI interface.

   When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
   then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
   you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
   will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
   when removing the PICC from reading distance too early.

   If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
   So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
   details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
   keep the PICCs at reading distance until complete.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             4         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <MFRC522.h>


#define LDRpin A0 // pin where we connected the LDR and the resistor
#define KP 0
#define KD 0
#define LINE 2000
#define BASESPD 170
#define MAXSPD 230

constexpr uint8_t RST_PIN = 2;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 53;         // Configurable, see typical pin layout above
constexpr uint8_t BUZZER = 12;         // Configurable, see typical pin layout above

int state = 0;         // Configurable, see typical pin layout above
int bluetooth_counter = 0;         // Configurable, see typical pin layout above

constexpr uint8_t DISPENSER_STATE = 1;               // Configurable, see typical pin layout above
constexpr uint8_t LIQUID_DROP_STATE = 3;             // Configurable, see typical pin layout above
constexpr uint8_t FIRST_CURVE_RFID_STATE = 5;        // Configurable, see typical pin layout above
constexpr uint8_t SECOND_CURVE_RFID_STATE = 7;       // Configurable, see typical pin layout above
constexpr uint8_t THIRD_CURVE_RFID_STATE = 9;        // Configurable, see typical pin layout above

constexpr uint8_t START_STATE = 0;                   // Configurable, see typical pin layout above
constexpr uint8_t CUP_DEPLOYED_STATE = 2;            // Configurable, see typical pin layout above

constexpr uint8_t TABLE_ONE_STATE = 4;               // Configurable, see typical pin layout above
constexpr uint8_t TABLE_TWO_STATE = 6;               // Configurable, see typical pin layout above
constexpr uint8_t TABLE_THREE_STATE = 8;             // Configurable, see typical pin layout above

constexpr uint8_t WAITING_CLIENT_STATE = 10;         // Configurable, see typical pin layout above
constexpr uint8_t GOING_BACK_STATE = 12;             // Configurable, see typical pin layout above

//------------------------------------------------------------------IDS Cadastrados-------------------------------------------------------------------//
MFRC522::Uid dispenserRFID = {4, {0x29, 0x4B, 0x0B, 0x0E}, 0xff}; // cartao1
MFRC522::Uid dropLiquidRFID = {4, {0x49, 0x8F, 0x39, 0x06}, 0xff}; // cartao2

MFRC522::Uid firstCurveRFID = {4, {0xB9, 0x64, 0x90, 0x01}, 0xff}; // cartao3
MFRC522::Uid firstStopRFID = {4, {0x70, 0xD7, 0x5F, 0xA8}, 0xff}; // cartao6

MFRC522::Uid secondCurveRFID = {4, {0x59, 0x0F, 0xCB, 0x01}, 0xff}; // cartao4
MFRC522::Uid secondStopRFID = {4, {0xD1, 0xFD, 0x7D, 0x6D}, 0xff}; // cartao7

MFRC522::Uid thirdCurveRFID = {4, {0x11, 0xC3, 0x40, 0x00}, 0xff}; // cartao5
MFRC522::Uid thirdStopRFID = {4, {0x30, 0xA2, 0x57, 0xA8}, 0xff}; // cartao8

// MFRC522::Uid tagRFID = {4, {0x04, 0x1C, 0x98, 0xEB}, 0xff}; // tag

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

bool reverse = false;
bool curveCompleted = false;

// ---------------------------------------------------------  Bluetooth variables  ----------------------------------------
char data = 0; //Variable for storing received data

//----------------------------------------------------------  Check cups  ----------------------------------------------//
bool haveCup = true;
int LDRValue = 0;     // result of reading the analog pin

void updateCupState(){
  LDRValue = analogRead(LDRpin); // read the value from the LDR
  
  Serial.println(LDRValue);
  if (LDRValue > 900) {
    haveCup = false;
    // Serial.println("Dont have cup");      // print the value to the serial port
  } else {
    haveCup = true;
    // Serial.println("Has cup");      // print the value to the serial port
  }
}

void buzz() {
  tone(BUZZER,500);  
  delay(400);
  noTone(BUZZER);
  delay(400);
}

void buzz_start() {
  tone(BUZZER,500);  
}

void buzz_stop() {
   noTone(BUZZER);
}

//-------------------------------------------------------  Controle do movimento  -------------------------------------------------------------------
/* MOTOR 1 -->
  MOTOR 2  <-- 
  MOTOR 3  V
*/
int reverseCounter = 0;
int side = 0;

float pos = LINE, error = 0, lError = 0, lPos;

constexpr uint8_t MOTOR3_SPD = 4;         // Configurable, see typical pin layout above
constexpr uint8_t MOTOR3_IN1 = 5;         // Configurable, see typical pin layout above
constexpr uint8_t MOTOR3_IN2 = 3;         // Configurable, see typical pin layout above

constexpr uint8_t MOTOR1_SPD = 6;         // Configurable, see typical pin layout above
constexpr uint8_t MOTOR1_IN1 = 8;         // Configurable, see typical pin layout above
constexpr uint8_t MOTOR1_IN2 = 7;         // Configurable, see typical pin layout above

constexpr uint8_t MOTOR2_IN1 = 9;         // Configurable, see typical pin layout above
constexpr uint8_t MOTOR2_IN2 = 10;         // Configurable, see typical pin layout above
constexpr uint8_t MOTOR2_SPD = 11;         // Configurable, see typical pin layout above

constexpr uint8_t S1 = 38;
constexpr uint8_t S2 = 40;
constexpr uint8_t S3 = 42;
constexpr uint8_t S4 = 44;
constexpr uint8_t S5 = 46;


//------------------------------------------------------ Turn left  -----------------------------------------------------
void turnLeft() {
  analogWrite(MOTOR1_SPD, BASESPD);
  analogWrite(MOTOR2_SPD, BASESPD);
  analogWrite(MOTOR3_SPD, BASESPD);
  
  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);
  
  digitalWrite(MOTOR2_IN1, LOW);
  digitalWrite(MOTOR2_IN2, HIGH);
  
  digitalWrite(MOTOR3_IN1, LOW);
  digitalWrite(MOTOR3_IN2, HIGH);
  Serial.print("Virando 90 graus para esquerda\n");
  delay(400);
}


//------------------------------------------------------ Turn right -----------------------------------------------------
void turnRight() {
  analogWrite(MOTOR1_SPD,BASESPD);
  analogWrite(MOTOR2_SPD,BASESPD);
  analogWrite(MOTOR3_SPD,BASESPD);
  
  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, HIGH);
  
  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, LOW);
  
  digitalWrite(MOTOR3_IN1, HIGH);
  digitalWrite(MOTOR3_IN2, LOW);
  Serial.print("Virando 90 graus para direita\n");
  delay(400);
}

// -----------------------------------------------  Forward  ---------------------------------------------------
void stop() {
  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, HIGH);

  // Keep current flow to avoid power bank disable
  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, HIGH);
      
  digitalWrite(MOTOR3_IN1, LOW);
  digitalWrite(MOTOR3_IN2, HIGH);
  analogWrite(MOTOR3_SPD, 5);
}

void turnBackwards() {
  analogWrite(MOTOR1_SPD, BASESPD);
  analogWrite(MOTOR2_SPD, BASESPD);
  analogWrite(MOTOR3_SPD, BASESPD);
  
  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);
  
  digitalWrite(MOTOR2_IN1, LOW);
  digitalWrite(MOTOR2_IN2, HIGH);
  
  digitalWrite(MOTOR3_IN1, LOW);
  digitalWrite(MOTOR3_IN2, HIGH);
  Serial.print("Virando 180 graus para esquerda\n");
  delay(500);
}


//------------------------------------------------------ Control loop    -----------------------------------------------------

void control()
{
  // Setup control variables
  int sensor1 = digitalRead(S1); //sensor1
  int sensor2 = digitalRead(S2); //sensor2
  int sensor3 = digitalRead(S3); //sensor3
  int sensor4 = digitalRead(S4); //sensor4
  int sensor5 = digitalRead(S5); //sensor5
  int ps1 = !sensor1*0;
  int ps2 = !sensor2*1000;
  int ps3 = !sensor3*2000;
  int ps4 = !sensor4*3000;
  int ps5 = !sensor5*4000;
  
  // Check if robot is waiting bluetooth response on top of dispenser or liquid drop RFID
  if (state  == DISPENSER_STATE or state == LIQUID_DROP_STATE) {
    stop();
    
  // Check if robot is waiting for client to pickup cup and activates buzzer if positive.
  } else if (state == WAITING_CLIENT_STATE) {
    updateCupState();

    // Check cup state, make noise if the client didn`t catch the cup
    if (haveCup) {
      stop();
      buzz();

    // Check cup state, go back to dispenser if cup is not present anymore
    } else {
      state = GOING_BACK_STATE;
      turnBackwards();
    } 
  }
  
  // Check all sensors and apply PD algorithm to follow line
  else if(!sensor1 or !sensor2 or !sensor3 or !sensor4 or !sensor5) {
    lPos = pos;
    pos = (ps1 + ps2+ ps3 + ps4 + ps5) / (!sensor1 + !sensor2 + !sensor3 + !sensor4 + !sensor5);
    lError = error;
    error = pos - LINE;
    float k = 0.015;
    float motorSpd = k * error; //+ Kd * (error - lError);
    int spd1 = (BASESPD - motorSpd) * 1.25;
    int spd2 = BASESPD + motorSpd;
    
    if(spd1 > MAXSPD) {
      spd1 = MAXSPD;
    }
    if(spd2 > MAXSPD) {
      spd2 = MAXSPD;
    }
    
    // Move backwards to leave dispenser
    if (reverse) {
      Serial.println("Reverse");
      
      buzz_start();
      
      // Reverse during 65 iterations
      if (reverseCounter < 65) {
        Serial.println("Reverse");
        analogWrite(MOTOR1_SPD, spd2 * 1.1);
        analogWrite(MOTOR2_SPD, spd1 * 1.1);
        
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, HIGH);
      
        digitalWrite(MOTOR2_IN1, LOW);
        digitalWrite(MOTOR2_IN2, HIGH);

        reverseCounter++;

      // Turn 180 degrees after leaving dispenser to continue its delivery
      } else {
        Serial.println("Turning 180 degrees");
        reverse = false;
        reverseCounter = 0;
        turnBackwards();
      }

    } else {
      buzz_stop();
      
      analogWrite(MOTOR1_SPD, spd1);
      analogWrite(MOTOR2_SPD, spd2);
          
      digitalWrite(MOTOR1_IN1, HIGH);
      digitalWrite(MOTOR1_IN2, LOW);
    
      digitalWrite(MOTOR2_IN1, HIGH);
      digitalWrite(MOTOR2_IN2, LOW);

      reverseCounter = 0;
    }
   
    digitalWrite(MOTOR3_IN1, HIGH);
    digitalWrite(MOTOR3_IN2, HIGH);
  }
}

// ------------------------------------- Write and read from bluetooth -----------------------------------

// Read from bluetooth when information is available
char readBluetooth()
{
   data = ',';
   if(Serial3.available()> 0) { // Send data only when you receive data:
      data = Serial3.read();        //Read the incoming data & store into data
      Serial.print(data);          //Print Value inside data in Serial monitor
      Serial.print("   received\n");
   }
   return data;
}

// Write to bluetooth serial
void writeBluetooth(char writeData) {
    Serial.println("Sent ");
    Serial3.write(writeData);  
}

// ------------------------------------------------------ Change states -----------------------------------------------------

// -----------------------------  RFID change of states  -----------------------------------------

// Changes of state related to RFID stops
void checkRFID()
{
  if (mfrc522.uid.uidByte[0] == dispenserRFID.uidByte[0] && (state == START_STATE || state == GOING_BACK_STATE)) {
    state = DISPENSER_STATE;
  }
  
  if (mfrc522.uid.uidByte[0] == dropLiquidRFID.uidByte[0] && state == CUP_DEPLOYED_STATE) {
    state = LIQUID_DROP_STATE;
    curveCompleted = false; ;
  }
  
  
  if (mfrc522.uid.uidByte[0] == firstCurveRFID.uidByte[0] && (state == TABLE_ONE_STATE || state == GOING_BACK_STATE)) {
    if (state == GOING_BACK_STATE) {
      turnRight();
      state = START_STATE;
    } else if (!curveCompleted) {
      turnLeft();
      curveCompleted = true;
    } 
  }
  
  if (mfrc522.uid.uidByte[0] == secondCurveRFID.uidByte[0] && (state == TABLE_TWO_STATE || state == GOING_BACK_STATE)) {
    if (state == GOING_BACK_STATE) {
      turnRight();
      state = START_STATE;
    } else if (!curveCompleted) {
      turnLeft();
      curveCompleted = true;
    }
  }
    
  if (mfrc522.uid.uidByte[0] == thirdCurveRFID.uidByte[0] && (state == TABLE_THREE_STATE || state == GOING_BACK_STATE)) {
    if (state == GOING_BACK_STATE) {
      turnRight();
      state = START_STATE;
    } else if (!curveCompleted) {
      turnLeft();
      curveCompleted = true;
    }
  }

  if (mfrc522.uid.uidByte[0] == firstStopRFID.uidByte[0] && (state == TABLE_ONE_STATE)) {
    state = WAITING_CLIENT_STATE;  
  }

  if (mfrc522.uid.uidByte[0] == secondStopRFID.uidByte[0] && (state == TABLE_TWO_STATE) ) {
      state = WAITING_CLIENT_STATE;
  }

  if (mfrc522.uid.uidByte[0] == thirdStopRFID.uidByte[0] && (state == TABLE_THREE_STATE)) {
      state = WAITING_CLIENT_STATE;
  }
}

// ------------------------------------------ Bluetooth change of states --------------------------------
// Changes of state related to bluetooth
void changeState(){
  // Check if data received is other than default
  if (data != ',' && data != 'a')  {
  
    // Check if data received as char is equal to the state converted to char
    // Change to cup deployed state when base station sends character '2'
    updateCupState();
    if ((data == (char) (CUP_DEPLOYED_STATE + 0x30)) && haveCup) {
      Serial.println("Changed to state 2");
      state = CUP_DEPLOYED_STATE;

    // Change to table one state when base station sends character '4'
    Serial.println(data == (char) (TABLE_ONE_STATE + 0x30));
    } else if (data == (char) (TABLE_ONE_STATE + 0x30) && state == LIQUID_DROP_STATE) {
      Serial.println("Changed to state 4");
      state = TABLE_ONE_STATE;

      reverse = true;
      
    // Change to table two state when base station sends character '6'
    }  else if (data == (char) (TABLE_TWO_STATE + 0x30) && state == LIQUID_DROP_STATE) {
      Serial.println("Changed to state 6");
      state = TABLE_TWO_STATE;
      
      reverse = true;
      
    // Change to table three state when base station sends character '8'
    }  else if (data == (int) (TABLE_THREE_STATE + 0x30) and state == LIQUID_DROP_STATE) {
      Serial.println("Changed to state 8");
      state = TABLE_THREE_STATE;

      reverse = true;
    }
  }
}

// ------------------------------- Send state through bluetooth --------------------------------------------
void sendStateBluetooth() {
  // Only sends information every 50th loop iteration
  if ((bluetooth_counter % 50) == 0) {
  
    // Base station only needs to receive information to drop cups and drop liquid
    if ((state == 1 || state == 3)) {
      Serial.println("Data sent to bluetooth");
      Serial.println((char) (state + 0x30));
      writeBluetooth((char) (state + 0x30));

      // Bluetooth delay time for synchronization with base station
      delay(1500);
    }
  }
  
  bluetooth_counter++;
}

// -------------------------------------------------- Setup ----------------------------------------------------
void setup() {
  // Inicializar bluetoothj
  //Initialize Bluetooth Serial Port
  Serial3.begin(9600);

  //Sensor de 5 vias
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);

  //Pinos dos motores
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR2_IN1, OUTPUT);
  pinMode(MOTOR2_IN2, OUTPUT);
  pinMode(MOTOR3_IN1, OUTPUT);
  pinMode(MOTOR1_SPD, OUTPUT);
  pinMode(MOTOR2_SPD, OUTPUT);
  pinMode(MOTOR3_SPD, OUTPUT);

  //Pino do Buzzer
  pinMode(BUZZER, OUTPUT);

  Serial.begin(9600);		// Initialize serial communications with the PC
  while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();			// Init SPI bus
  
  mfrc522.PCD_Init();		// Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  digitalWrite(BUZZER, LOW);
}

// ------------------------------------------------- MAIN LOOP ---------------------------------------------
void loop() {
  // RFID state changes
  checkRFID();

  // Update state based on RFID and Bluetooth updates
  changeState();

  // Control routine
  control();
  
  // Bluetooth state changes
  sendStateBluetooth();
  
  // Write data from HC06 to Serial Monitor
  data = readBluetooth();

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return; 
  }
}
