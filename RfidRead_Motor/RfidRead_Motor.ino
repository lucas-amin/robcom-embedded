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


#define KP 0
#define KD 0
#define LINE 2000
#define BASESPD 130
#define MAXSPD 220

constexpr uint8_t RST_PIN = 2;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 53;         // Configurable, see typical pin layout above

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

constexpr uint8_t BUZZER = 12;         // Configurable, see typical pin layout above


constexpr uint8_t COPO = 15;         // Configurable, see typical pin layout above
float IMPULSE_CONSTANT = 1.3;

/* MOTOR 1 -->
  MOTOR 2  <-- 
  MOTOR 3  V
            */
int direction_number = 0;
int FRONT = 1;
int LEFT = 2;
int RIGHT = 3;
int counter = 0;
int side = 0;
float pos = LINE, error = 0, lError = 0, lPos;
bool tala = true;
bool javiro = false;
MFRC522::Uid parada1 = {4, {0x29, 0x4B, 0x0B, 0x0E}, 0xff}; // cartao1
//parada1.size = 4
//parada1.uidByte = {0x29, 0x4B, 0x0B, 0x0E} ; // uid do parada1


MFRC522::Uid parada2 = {4, {0x04, 0x1C, 0x98, 0xEB}, 0xff}; // tag
//parada2.size = 4
//parada2.uidByte = {0x04, 0x1C, 0x98, 0xEB} ; // uid do card2

//------------------------------------------------------------------IDS Cadastrados-------------------------------------------------------------------//

//(size,uid,whatevs)

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance


void checkCup()
{
  int cup = analogRead(COPO); //sensor de copo
  if (cup > 100)
    tala = true;
  else
    tala = false;
}

void turnLeft(int spd)
{
  if (direction_number != LEFT ) {
    analogWrite(MOTOR1_SPD, spd * 1.25 * IMPULSE_CONSTANT);
    analogWrite(MOTOR2_SPD, spd * IMPULSE_CONSTANT);
    analogWrite(MOTOR3_SPD, spd * IMPULSE_CONSTANT);

    //    Serial.print("IMPULSE LEFT");
  } else {
    analogWrite(MOTOR1_SPD, spd * 1.25);
    analogWrite(MOTOR2_SPD, spd * 0.6);
    analogWrite(MOTOR3_SPD, spd);

    //  Serial.print("NORMAL LEFT");
  }
  direction_number = LEFT;

  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);

  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, LOW);

  digitalWrite(MOTOR3_IN1, HIGH);
  digitalWrite(MOTOR3_IN2, HIGH);
}

void turnRight(int spd)
{
  if (direction_number != RIGHT) {
    analogWrite(MOTOR1_SPD, spd * 1.25 * IMPULSE_CONSTANT);
    analogWrite(MOTOR2_SPD, spd * IMPULSE_CONSTANT);
    analogWrite(MOTOR3_SPD, spd * IMPULSE_CONSTANT);

    Serial.print("IMPULSE RIGHT");
  } else {
    analogWrite(MOTOR1_SPD, spd * 0.6 * 1.25);
    analogWrite(MOTOR2_SPD, spd);
    analogWrite(MOTOR3_SPD, spd );

    Serial.print("NORMAL RIGHT");
  }

  direction_number = RIGHT;

  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);

  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, LOW);

  digitalWrite(MOTOR3_IN1, HIGH);
  digitalWrite(MOTOR3_IN2, HIGH);
}

void go(int spd)
{
  if (direction_number != FRONT) {
    analogWrite(MOTOR1_SPD, spd * 1.25 * IMPULSE_CONSTANT  );
    analogWrite(MOTOR2_SPD, spd * IMPULSE_CONSTANT  );

    Serial.print("IMPULSE FRONT");
  }
  else {

    analogWrite(MOTOR1_SPD, spd * 1.25);
    analogWrite(MOTOR2_SPD, spd  );
    analogWrite(MOTOR3_SPD, spd);
    Serial.print("NORMAL FRONT");

  }
  direction_number = FRONT;

  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, LOW);

  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, LOW);

  digitalWrite(MOTOR3_IN1, LOW);
  digitalWrite(MOTOR3_IN2, LOW);
}
void stop()
{
  digitalWrite(MOTOR1_IN1, HIGH);
  digitalWrite(MOTOR1_IN2, HIGH);

  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, HIGH);

  digitalWrite(MOTOR3_IN1, HIGH);
  digitalWrite(MOTOR3_IN2, HIGH);

}

void entraMesa()
{
  javiro = true;
  analogWrite(MOTOR1_SPD,130);
  analogWrite(MOTOR2_SPD,130);
  analogWrite(MOTOR3_SPD,130);

  digitalWrite(MOTOR1_IN1, LOW);
  digitalWrite(MOTOR1_IN2, HIGH);

  digitalWrite(MOTOR2_IN1, HIGH);
  digitalWrite(MOTOR2_IN2, LOW);

  digitalWrite(MOTOR3_IN1, HIGH);
  digitalWrite(MOTOR3_IN2, LOW);
  Serial.print("Entrando na curva\n");
  delay(500);
  Serial.print("Pós delay\n");
  Serial.print(mfrc522.uid.uidByte[0]);
  mfrc522.uid.uidByte[0] = 0x00;
}

void saiMesa();
{
  // voltando pra faixa
  analogWrite(MOTOR1_SPD,130);
  analogWrite(MOTOR2_SPD,130);
  analogWrite(MOTOR3_SPD,130);

  digitalWrite(MOTOR1_IN2, LOW);
  digitalWrite(MOTOR1_IN1, HIGH);

  digitalWrite(MOTOR2_IN2, HIGH);
  digitalWrite(MOTOR2_IN1, LOW);

  digitalWrite(MOTOR3_IN2, HIGH);
  digitalWrite(MOTOR3_IN1, LOW);
  delay(600);
}

void waiting();
{
    analogWrite(MOTOR1_SPD,130);
    analogWrite(MOTOR2_SPD,130);
    analogWrite(MOTOR3_SPD,130);
  
    digitalWrite(MOTOR1_IN1, HIGH);
    digitalWrite(MOTOR1_IN2, HIGH);
  
    digitalWrite(MOTOR2_IN1, HIGH);
    digitalWrite(MOTOR2_IN2, HIGH);
  
    digitalWrite(MOTOR3_IN1, HIGH);
    digitalWrite(MOTOR3_IN2, HIGH);
    for(int i=0; i<20; i++)
    {
      if(!tala)
        break;
      delay(1000);
      
    }
    tala = false;
}
void control()
{
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
  Serial.print(mfrc522.uid.uidByte[0]);

  if(!sensor1 or !sensor2 or !sensor3 or !sensor4 or !sensor5)
  {
    lPos = pos;
    pos = (ps1 + ps2+ ps3 + ps4 + ps5)/(!sensor1 + !sensor2 + !sensor3 + !sensor4 + !sensor5);
    lError = error;
    error = pos - LINE;
    //Serial.print(error);
    //Serial.print("\n");
    float k = 0.015;
    float motorSpd = k * error; //+ Kd * (error - lError);
    int spd1 = (BASESPD - motorSpd) * 1.25;
    int spd2 = BASESPD + motorSpd;
    if(spd1>MAXSPD)
      spd1 = MAXSPD;
    if(spd2>MAXSPD)
      spd2 = MAXSPD;
   
    analogWrite(MOTOR1_SPD,spd1);
    analogWrite(MOTOR2_SPD,spd2);
  
    digitalWrite(MOTOR1_IN1, HIGH);
    digitalWrite(MOTOR1_IN2, LOW);
  
    digitalWrite(MOTOR2_IN1, HIGH);
    digitalWrite(MOTOR2_IN2, LOW);
  
    digitalWrite(MOTOR3_IN1, HIGH);
    digitalWrite(MOTOR3_IN2, HIGH);
    
  }
  else if(mfrc522.uid.uidByte[0] == parada2.uidByte[0])
  {
    waiting();
  }
  else
  {
    analogWrite(MOTOR1_SPD,130);
    analogWrite(MOTOR2_SPD,130);
    analogWrite(MOTOR3_SPD,130);
  
    digitalWrite(MOTOR1_IN1, LOW);
    digitalWrite(MOTOR1_IN2, HIGH);
  
    digitalWrite(MOTOR2_IN1, HIGH);
    digitalWrite(MOTOR2_IN2, LOW);
  
    digitalWrite(MOTOR3_IN1, HIGH);
    digitalWrite(MOTOR3_IN2, LOW);
    if(javiro)
      tala = false;
  }
  if (mfrc522.uid.uidByte[0] == parada1.uidByte[0])
    {
      if(tala)
      {
        // virando pra mesa
        entraMesa();
        
      }
        
      else
      {
        saiMesa();
      }
    }
}

void checkRFID()
{
  Serial.print("Checking RFID");

  if (mfrc522.uid.uidByte[0] == parada1.uidByte[0])
  {
    for (int i = 0; i < 5; i++)
    {
      //Serial.print("FIRST CARD");
      /*digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
        delay(1000);*/
      analogWrite(BUZZER, 20);

    }

  }
  else if (mfrc522.uid.uidByte[0] == parada2.uidByte[0])
  {
    for (int i = 0; i < 25; i++)
    {
      //Serial.print("SECOND CARD");
      /*digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(200);*/
      analogWrite(BUZZER, 250);

    }
  }
}
void setup() {

  //Sensor de 5 vias
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);
  direction_number = 0;

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

  //Pino do sensor de copo
  //pinMode(COPO, INPUT);

  Serial.begin(9600);    // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  digitalWrite(BUZZER, LOW);

  turnLeft(80);
}

void loop() {
  while(0)
  {
    analogWrite(MOTOR1_SPD,150);
    analogWrite(MOTOR2_SPD,100);
    digitalWrite(MOTOR3_IN1, HIGH);
    digitalWrite(MOTOR3_IN2, HIGH);
    
    digitalWrite(MOTOR2_IN1, HIGH);
    digitalWrite(MOTOR2_IN2, LOW);
    
    digitalWrite(MOTOR1_IN1, HIGH);
    digitalWrite(MOTOR1_IN2, LOW);
    
    Serial.print("DEBUG\n");
  }
  
  /* while(1){
     analogWrite(MOTOR1_SPD, 173);
     analogWrite(MOTOR2_SPD, 138);

     digitalWrite(MOTOR2_IN1, HIGH);
     digitalWrite(MOTOR2_IN2, LOW);

     digitalWrite(MOTOR1_IN1, HIGH);
     digitalWrite(MOTOR1_IN2, LOW);

     digitalWrite(MOTOR3_IN1, LOW);
     digitalWrite(MOTOR3_IN2, LOW);
    }*/
  //checkCup();
  control();

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  checkRFID();

  // Dump debug info about the card; PICC_HaltA() is automatically called
 // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

  if (parada1.uidByte)
    Serial.print("AE");

}
