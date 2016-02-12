#include <LiquidCrystalShift.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

// initialize the library with the numbers of the interface pins
LiquidCrystalShift lcd(A2, A3, 13, 8, 7);

void setup() {
//  Serial.begin(115200);
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);

  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Apollo Trainer!");
  if(!bmp.begin())
  {
    lcd.setCursor(0, 1);
    /* There was a problem detecting the BMP085 ... check your connections */
    lcd.print("turn on sensor!");
    while(1);
  }
}

int lastTone = 0;

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  
  int pot1 = analogRead(A0);
  if (pot1 < 1000)
    lcd.print(" ");
  if (pot1 < 100)
    lcd.print(" ");
  if (pot1 < 10)
    lcd.print(" ");
  lcd.print(pot1);
  lcd.print("  ");
//  Serial.println(pot1);

  int pot2 = analogRead(A1);
  int p2l = pot2 - lastTone;
  p2l = p2l > 0 ? p2l : -p2l;
  if (p2l < 10 && pot2 != 0)
    pot2 = lastTone;
  tone(12, pot2, 300);
  lastTone = pot2;
  
  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure)
  {
    float temp;
    bmp.getTemperature(&temp);
//    temp = temp * 9.0 / 5.0 + 32;
    lcd.print(String(temp, 1));
    lcd.print("*C");
  }
  
//  lcd.print(millis() / 1000);
//  delay(30);
}

