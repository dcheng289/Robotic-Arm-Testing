#include <Wire.h>

// Gripper fan project for Grabit
/
* This program controls 12 fans to provide suction force to grip lightweight objects.
* The fans are driven with transistors soldered onto an Arduino motor shield. This 
* program takes user input through serial port, to toggle the fans being on/off
* (12 digital pins) & the fan power (1 PWM pin)
*
* Sample commands are
* f8191       // 'f' to toggle fans on/off, 0-8191 for 12 fan bits 
* p255        // 'p' to adjust power, 0-255 to adjust voltage
*
/

// Global variables
boolean fan_switch[13];
int PWM_pin = 12;
int pin0 = 30;
int pin10 = 45;
bool reading = true;

void setup() {

  for (int i = 0; i < 13; i++)    // set the 13 digital pins to output and initialize as of
  {
    pinMode(pin0 + i, OUTPUT);
    digitalWrite(pin0 + i, LOW);
  }

  // Pin 10
  pinMode(pin10, OUTPUT);
  digitalWrite(pin10, LOW);
  
  Serial.begin(38400);
  fanSwitch(0);
  setPower(0);
}

void loop() {
   while(Serial.available() == 0) ; // Do nothing until serial input is received

   while(Serial.available() != 0) // loop thru the whole serial
   {
    delay(10);

    char command = Serial.read();

    if (command == 'F' || command == 'f')    // f detected, turn fans on/off
      fanSwitch(numberCollect());
    else if (command == 'P' || command == 'p')    // p detected, adjust power
      setPower(numberCollect());
   }

   
  //Serial.println(numberCollect(), BIN);
}

int numberCollect(){        // Reads from serial, returns int
  int numberInput = 0;
  char buf;
  while (buf != 13) {         // while last character was not a carriage return
    buf = Serial.read();
    if (buf != 13) {
    numberInput *= 10;
    numberInput += (buf - '0');
    }

    // do I need to have an exception when the value entered is greater than 8191
  }
  return numberInput;
}

void fanSwitch (int fan) {

  // save the number from the serial input
   Serial.println(fan);

  // set the boolean
  for (int i = 12; i > -1; i--)
  {
    fan_switch[i] = bitRead(fan, i);
    //Serial.print(fan_switch[i]);
  }
  //Serial.println(); 

  // set the pins
  for (int i = 0; i < 13; i++)
  {
    if (fan_switch[i])
       digitalWrite(pin0 + i, HIGH);
    else
       digitalWrite(pin0 + i, LOW);
  }

  // Switching pin10 on
  if(fan_switch[9])
      digitalWrite(pin10, HIGH);
  else
      digitalWrite(pin10, LOW);

}

void setPower (int power) {   

   //Serial.print("Power: ");
   //Serial.println(power);
   
   // Analog write to PWM pin
   analogWrite(PWM_pin, power);
}


