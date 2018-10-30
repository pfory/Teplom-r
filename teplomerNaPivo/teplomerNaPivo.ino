#include<CountUpDownTimer.h>

CountUpDownTimer T(DOWN);

#define verbose
#ifdef verbose
 #define DEBUG_PRINT(x)         Serial.print (x)
 #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
 #define DEBUG_PRINTHEX(x)      Serial.print(x, HEX);

 #define DEBUG_PRINTLN(x)       Serial.println (x)
 #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
 #define PORTSPEED 115200
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTF(x, y)
#endif 

//SW name & version
#define     VERSION                      "0.1"
#define     SW_NAME                      "Teplomer na pivo"

//display
#define LCDADDRESS   0x20
#define LCDROWS      2
#define LCDCOLS      16

#define PRINT_SPACE           lcd.print(F(" "));
#define PRINT_ZERO            lcd.print(F("0"));

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCDADDRESS,LCDCOLS,LCDROWS);  // set the LCD

#define SERIAL_SPEED    115200

float         temp                          = 15.5;
unsigned int  minOld                        = 0;
float         tempOld                       = temp;

void setup() {
  T.SetTimer(0,1,0);
  T.StartTimer();
  Serial.begin(SERIAL_SPEED);
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));
  lcd.begin();               // initialize the lcd 
  lcd.home();                   
  lcd.print(SW_NAME);  
  lcd.setCursor(0,1);
  lcd.print("verze ");
  lcd.print(VERSION);
  delay(2000);
  lcd.clear();
}

void loop() {
  T.Timer(); // run the timer
  displayTimeUp(12,0);
  displayTemp();
  if (T.TimeHasChanged()) { // this prevents the time from being constantly shown.
    displayTimeDown(12,1);
  }
}

void displayTimeUp(byte col, byte row) {
  unsigned int min = millis() / 1000 / 60;
  byte sec = (millis() / 1000) % 60;
  if (min<10) 
    lcd.setCursor(col,row);
  else if (min<100)
    lcd.setCursor(col-1,row);
  lcd.print(min);
  lcd.print(":");
  if (sec<10) PRINT_ZERO
  lcd.print(sec);
}

void displayTimeDown(byte col, byte row) {
  if (T.ShowMinutes()==0 && T.ShowSeconds() == 0) {
    lcd.setCursor(0,1);
    lcd.print("Nasyp 2 chmel!!!");
  } else {
    lcd.setCursor(col-1,row);
    if (T.ShowMinutes()<10) {
      PRINT_SPACE
    } else {
    }
    lcd.print(T.ShowMinutes());
    lcd.print(":");
    if (T.ShowSeconds()<10) PRINT_ZERO
    lcd.print(T.ShowSeconds());
  }
}


void displayTemp() {
  lcd.setCursor(0,0);
  lcd.print(" T=");
  lcd.print(temp);

  unsigned int min = millis() / 1000 / 60;
  if (min>minOld) {
    minOld = min;
    lcd.setCursor(0,1);
    lcd.print("dT=");
    lcd.print(temp - tempOld);
  }
}