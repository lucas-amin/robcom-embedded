#include <SoftwareSerial.h>

SoftwareSerial hc06(15,14);
char data = 0;            //Variable for storing received data

void setup(){
  //Initialize Serial Monitor
  Serial.begin(9600);
  Serial.println("ENTER AT Commands:");
  //Initialize Bluetooth Serial Port
  Serial3.begin(9600);
}

void loop(){
  //Write data from HC06 to Serial Monitor
   if(Serial3.available()> 0)      // Send data only when you receive data:
   {
      data = Serial3.read();        //Read the incoming data & store into data
      Serial.print(data);          //Print Value inside data in Serial monitor
      Serial.print("\n");        
      
   }
  
  //Write from Serial Monitor to HC06
  if (Serial.available()){
    Serial.println("Sending");
    Serial3.write(Serial.read());
  }  
}
