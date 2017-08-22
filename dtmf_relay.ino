/*
 *  Copyright 2017 Michael Brown
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
 *  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, 
 *  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the 
 *  Software is furnished to do so, subject to the following conditions:
   
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the 
 *  Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
 *  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


/* ==================================================================================================================================================
   KG9DW DTMF Decoder and Lock, based on code from Scott C from website: http://arduinobasics.blogspot.com/p/arduino-basics-projects-page.html
===================================================================================================================================================== */

/* To use, send the 4 digit passcode and then:
 * 1 turns on relay 1
 * 2 turns off relay 1
 * 4 turns on relay 2
 * 5 turns off relay 2
 * 7 turns on relay 3
 * 8 turns off relay 3
 * 3 turns on relay 4
 * 6 turns off relay 4
 * 
 * To set passcode, take pin 7 to 3V before powering up and use keyboard. Send a newline after entering 4 digits.
 */



/*
 * TODO:
 * 1. Provide feedback by sending cw back through the pin 13 and PTT with pin 6
 * 2. Provide way to set passcode over the air
 */

#include <EEPROM.h>
#include "Timer.h"

//Global variables-----------------------------------------------------------------------------------------
int stqStatus = LOW;

boolean wasHigh = false;
boolean hasAccess = false;
const int codeLength = 4;
byte accessCode[codeLength];
const int turnOn = 1;
const int turnOff = 10; // DTMF 0 comes through as 10 as this is a 10 based code
byte received[codeLength];
byte DTMFread;           // The DTMFread variable will be used to interpret the output of the DTMF module.
int count = 0;
const int STQ = 8;       // Attach DTMF Module STQ Pin to Arduino Digital Pin 3
const int Q4 = 9;        // Attach DTMF Module Q4  Pin to Arduino Digital Pin 4
const int Q3 = 10;        // Attach DTMF Module Q3  Pin to Arduino Digital Pin 5
const int Q2 = 11;        // Attach DTMF Module Q2  Pin to Arduino Digital Pin 6
const int Q1 = 12;        // Attach DTMF Module Q1  Pin to Arduino Digital Pin 7
const int P1 = 7;         // Put pin 7 to 3V if you want to force change of the passcode
const int R1 = 2;
const int R2 = 3;
const int R3 = 4;
const int R4 = 5;

unsigned long previousMillis = 0; // last time update
long interval = 5000; // interval at which to do something (milliseconds)
unsigned long currentMillis;

void setup() {
  // start serial port
  Serial.begin(9600);
  Serial.println("DTMF Relay Control - KG9DW");

  //Setup the INPUT pins on the Arduino
  
  pinMode(STQ, INPUT);
  pinMode(Q4, INPUT);
  pinMode(Q3, INPUT);
  pinMode(Q2, INPUT);
  pinMode(Q1, INPUT);
  pinMode(P1, INPUT);

  //led indicator on arduino
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  //control wires for relay board
  pinMode(R1, OUTPUT);
  digitalWrite(R1, HIGH);
  pinMode(R2, OUTPUT);
  digitalWrite(R2, HIGH);
  pinMode(R3, OUTPUT);
  digitalWrite(R3, HIGH);
  pinMode(R4, OUTPUT);
  digitalWrite(R4, HIGH);

    //Read accessCode from EEPROM
  byte eepromAccessCode[codeLength];  
  EEPROM.get(0, eepromAccessCode);
  //EEPROM.write(0,0);
  
  if ((eepromAccessCode[0] == 0) || (digitalRead(P1)==HIGH) ) { 
    Serial.println("Access Code is not set!");
    Serial.print("Enter 4 digit access code: ");
    Serial.setTimeout(60000);
    String keyedCode = Serial.readStringUntil('\n');
    Serial.println("");
    
    if (keyedCode.length()<4) {
      Serial.println("Invalid access code, using 1234");
      eepromAccessCode[0] = 1;
      eepromAccessCode[1] = 2;
      eepromAccessCode[2] = 3;
      eepromAccessCode[3] = 4;
    } else {
      eepromAccessCode[0] = int(keyedCode[0]) - 48;
      eepromAccessCode[1] = int(keyedCode[1]) - 48;
      eepromAccessCode[2] = int(keyedCode[2]) - 48;
      eepromAccessCode[3] = int(keyedCode[3]) - 48;
      EEPROM.put(0,eepromAccessCode[0]);
      EEPROM.put(1,eepromAccessCode[1]);
      EEPROM.put(2,eepromAccessCode[2]);
      EEPROM.put(3,eepromAccessCode[3]);
    }
  }
  
  //Serial.println(eepromAccessCode[0]);Serial.println(eepromAccessCode[1]);Serial.println(eepromAccessCode[2]);Serial.println(eepromAccessCode[3]);

  Serial.print("Access code: ");
  for(int x = 0; x < codeLength; x++) {
    accessCode[x] = eepromAccessCode[x];
    Serial.print(accessCode[x]);
    Serial.print(" ");
  }
  Serial.println("");

}

void clearCount() {
  count = 0;
  previousMillis = currentMillis;
  hasAccess = false;
  //Serial.println("   cleared count");
}

void loop() {

  currentMillis = millis();

  if(currentMillis - previousMillis > interval) {     
     clearCount();
  }
  
  stqStatus = digitalRead(STQ);
  if (stqStatus == LOW) {
    // this is the debounce logic that requires a LOW state between tones
    wasHigh = false;
  } else if (wasHigh == false) {    //Only read tone if there was a silence before
    DTMFread=0;
    wasHigh = true;
    previousMillis = currentMillis; // reset 5 sec timer since we got a tone
    
    if(digitalRead(Q1)==HIGH){      //If Q1 reads HIGH, then add 1 to the DTMFread variable
      DTMFread=DTMFread+1;
    }
    if(digitalRead(Q2)==HIGH){      //If Q2 reads HIGH, then add 2 to the DTMFread variable
      DTMFread=DTMFread+2;
    }
    if(digitalRead(Q3)==HIGH){      //If Q3 reads HIGH, then add 4 to the DTMFread variable
      DTMFread=DTMFread+4;
    }
    if(digitalRead(Q4)==HIGH){      //If Q4 reads HIGH, then add 8 to the DTMFread variable
      DTMFread=DTMFread+8;
    }  
    Serial.print("decoded ");
    Serial.println(DTMFread);
    
    if (hasAccess) {
      Serial.print("   has access and entered ");
      Serial.println(DTMFread);
      processRelay(DTMFread);

    } else {

      received[count] = DTMFread;
      count++;
      if (count > (codeLength - 1)) {
        clearCount();      
        Serial.print ("Checking code...");
        if (codeMatched()) {
          Serial.println("access code match!"); 
          hasAccess = true;            
        } else {
          Serial.println("invalid access code!");
        }
      }             
    }
  }
}

void processRelay(byte rCode) {
  Serial.print("   in processRelay, and rcode is ");
  Serial.println(rCode);
  switch (rCode) {
    case 1:
      Serial.println("in case 1");
      digitalWrite(R1, LOW);
      break;
    case 2:
      digitalWrite(R1, HIGH);
      break;
    case 4:
      digitalWrite(R2, LOW);
      break;
    case 5:
      digitalWrite(R2, HIGH);
      break;
    case 7:
      digitalWrite(R3, LOW);
      break;
    case 8:
      digitalWrite(R3, HIGH);
      break;
    case 3:
      digitalWrite(R4, LOW);
      break;
    case 6:
      digitalWrite(R4, HIGH);    
      break;
  }
}

boolean codeMatched() {
  boolean rc = true;
  for(int x = 0; x<codeLength; x++) {
    if (received[x] != accessCode[x]) {
      rc = false;
    }
  }
  return rc;
}

