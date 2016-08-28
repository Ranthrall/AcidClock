/*
  AcidClock - A simple Arduino-based clock with MAX7219 e DS3231
  Copyright (C) 2016 - Acidhub <contact@acidhub.click>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

  =======================================================================

*/

#include <Arduino.h>
#include <Wire.h>            // To RTS/DS3231 I2C communication
#include <LedControl.h>      // MAX7219 controller

#define DS_ADDRESS      0x68 // DS3231 I2C address

                  // ADDRESS |  DATA  (ALL data address from Datasheet)
#define SEC_REG         0x00 // 0xf0 = 10 seconds + 0x0f = seconds
#define MIN_REG         0x01 // 0xf0 = 10 minutes + 0x0f = minutes
#define HOUR_REG        0x02 // 0x40 = 12/24 + 0x20 = AM/PM||20 hour + 0x10 = 10hour + 0x0f=hour
#define WDAY_REG        0x03 // Week day from B00000001 to B00000111 (1 - 7)
#define MDAY_REG        0x04 // 0x30 = 10 day + 0x0f = day
#define MONTH_REG       0x05 // 0x80 = Century + 0x10 = 10 month + 0x0f = month
#define YEAR_REG        0x06 // 0xf0 = 10 year + 0x0f = year

#define AL1SEC_REG      0x07 // 0x80 = AL1M1 + 0x70 = 10 seconds + 0x0f = seconds
#define AL1MIN_REG      0x08 // 0x80 = AL1M2 + 0x70 = 10 minutes + 0x0f = minutes
#define AL1HOUR_REG     0x09 // 0x80 = AL1M3 + 0x40 = 12/24 + 0x20 = AM/PM||20 hour + 0x10 = 10hour + 0x0f=hour
#define AL1WDAY_REG     0x0A // 0x80 = AL1M4 + 0x40 = wday/day + 0x30 = 10 day + 0x0f = wday/day

#define AL2MIN_REG      0x0B // 0x80 = AL2M2 + 0x70 = 10 minutes + 0x0f = minutes
#define AL2HOUR_REG     0x0C // 0x80 = AL2M3 + 0x40 = 12/24 + 0x20 = AM/PM||20 hour + 0x10 = 10hour + 0x0f=hour
#define AL2WDAY_REG     0x0D // 0x80 = AL2M4 + 0x40 = wday/day + 0x30 = 10 day + 0x0f = wday/day

#define CONTROL_REG     0x0E // EOSC BBSQW CONV RS2  RS1     INTCN A2IE A1IE
#define STATUS_REG      0x0F // OSF  0     0    0    EN32kHz BSY   A2F  A1F
#define AGING_OFFSET_REG 0x10 //SIGN DATA  DATA DATA DATA    DATA  DATA DATA
#define TMP_UP_REG      0x11 // SIGN DATA  DATA DATA DATA    DATA  DATA DATA (temperature integer)
#define TMP_LOW_REG     0x12 // DATA DATA  0    0    0       0     0    0    (temperature float)

#define mxClock 12           // 
#define mxCS 11              // MAX7219 pinout
#define mxDin 10             //

byte algarism[10][3] = {B00011111, B00010001, B00011111, // 0
                        B00000000, B00011111, B00000000, // 1
                        B00011101, B00010101, B00010111, // 2
                        B00010001, B00010101, B00011011, // 3
                        B00000111, B00000100, B00011111, // 4
                        B00010111, B00010101, B00011101, // 5
                        B00011111, B00010100, B00011100, // 6
                        B00000001, B00000001, B00011111, // 7
                        B00011111, B00010101, B00011111, // 8
                        B00000111, B00000101, B00011111};// 9

LedControl mx=LedControl(mxDin,mxClock,mxCS,2);

void setup() {
    Wire.begin();
    Serial.begin(9600);
    mxConfig();

    // Uncomment bellow lines to set your clock
    //DSsetDate(20, 10, 7, 27, 8, 16);
    // Syntax: DSsetDate(hour, minute, weekDay, day, month, year);
}

void mxConfig(void) {
  int devices=mx.getDeviceCount();
  for(int matrix=0;matrix<devices;matrix++) {
    mx.shutdown(matrix,false);  // Wake up
    mx.setIntensity(matrix,2);  // set luminosity and
    mx.clearDisplay(matrix);    // clear.
  }
}

// DEC to BIN (DS3231 style) and BIN (DS3231 style) to DEC functions
byte dtob(byte data) { return ((data / 10 * 16) + (data % 10)); }
byte btod(byte data) { return ((data / 16 * 10) + (data % 16)); }

void DSset(byte reg, byte data) {
    Wire.beginTransmission(DS_ADDRESS);
    Wire.write(reg);            // Move to registrer
    Wire.write(dtob(data));     // Write data
    Wire.endTransmission();
}

byte DSread(byte reg) {
    Wire.beginTransmission(DS_ADDRESS);
    Wire.write(reg);                        // Move to register
    Wire.endTransmission();

    Wire.requestFrom(DS_ADDRESS, 1);        // Request 1 byte
    return Wire.read();                     // Return with response
}

void DSsetDate(byte hour, byte minute, byte wday,
               byte day, byte month, byte year) {
    DSset(HOUR_REG, dtob(hour));
    DSset(MIN_REG, dtob(minute));
    DSset(WDAY_REG, dtob(wday));
    DSset(MDAY_REG, dtob(day));
    DSset(MONTH_REG, dtob(month));
    DSset(YEAR_REG, dtob(year));
}

void writeNumber(byte pos, byte number) {
    switch(pos) {
        case 1:
            mx.setRow(0, 0, algarism[number][0]);
            mx.setRow(0, 1, algarism[number][1]);
            mx.setRow(0, 2, algarism[number][2]);
            break;
        case 2:
            mx.setRow(0, 5, algarism[number][0]);
            mx.setRow(0, 6, algarism[number][1]);
            mx.setRow(0, 7, algarism[number][2]);
            break;
        case 3:
            mx.setRow(1, 0, algarism[number][0]);
            mx.setRow(1, 1, algarism[number][1]);
            mx.setRow(1, 2, algarism[number][2]);
            break;
        case 4:
            mx.setRow(1, 5, algarism[number][0]);
            mx.setRow(1, 6, algarism[number][1]);
            mx.setRow(1, 7, algarism[number][2]);
            break;
    }
}

void displayTime(void) {
    byte hour, minute, second, wday, day, month, year;

    hour = DSread(HOUR_REG);
    minute = DSread(MIN_REG);
    second = DSread(SEC_REG);
    wday = DSread(WDAY_REG);
    day = DSread(MDAY_REG);
    month = DSread(MONTH_REG);
    year = DSread(YEAR_REG);
    
    writeNumber(1, btod((hour >> 4) & ((1 << 4) - 1)));   // Get first digit
    writeNumber(2, btod((hour & 0x0f)));                  // Get second digit
    writeNumber(3, btod((minute >> 4) & ((1 << 4) - 1)));
    writeNumber(4, btod((minute & 0x0f)));

    Serial.print("+=-=-=-=-=-=-=-=-=-=-=-=+\n|");
    
    if(btod(hour)<10)Serial.print("0");
    Serial.print(btod(hour));
    Serial.print(":");
    if(btod(minute)<10)Serial.print("0");
    Serial.print(btod(minute));
    Serial.print(":");
    if(btod(second)<10)Serial.print("0");
    Serial.print(btod(second));

    Serial.print("\t\t|\n|");

    Serial.print(btod(day));
    Serial.print("/");
    Serial.print(btod(month));
    Serial.print("/");
    Serial.print(btod(year));

    Serial.print("\t\t|\n|");
    
    switch (wday) {
    case 1:
      Serial.print("Sunday\t");
      break;
    case 2:
      Serial.print("Monday\t");
      break;
    case 3:
      Serial.print("Tuesday");
      break;
    case 4:
      Serial.print("Wednesday");
      break;
    case 5:
      Serial.print("Thursday");
      break;
    case 6:
      Serial.print("Friday\t");
      break;
    case 7:
      Serial.print("Saturday");
      break;
    }
    Serial.print("\t\t|\n|");
}

void displayTemp(void) {
    byte temp;
    byte tbyte = DSread(TMP_UP_REG);
  
    if(tbyte & B10000000) {             // check if -ve number
       tbyte ^= B11111111;  
       tbyte += 0x1;
       temp = (tbyte *- 1);
    } else {
        temp = tbyte;
    }

    Serial.print("Temperature is: ");
    Serial.print(btod(temp));
    Serial.print(char(176));            // EASCII char 176 (°)
    Serial.print("C\t");

    Serial.println("|\n+=-=-=-=-=-=-=-=-=-=-=-=+");
}

void loop() {
    displayTime();
    displayTemp();

    byte working = random(0x00, 0xff);
    mx.setColumn(0,0, working);
    mx.setColumn(0,1, ~working);
    mx.setColumn(1,0, ~working);
    mx.setColumn(1,1, working);
    
    delay(1000);
}
