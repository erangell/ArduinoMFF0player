/*
SOLAR TEST
Objective is to determine how long an Arduino MEGA can run using a solar charger.
Uses a data logger shield with an SD card to write to the datalog.txt file every 5 seconds.
After testing, put the solar charger and Arduino in a closet and cover the solar charger so it receives no light.
At the end of the day the charger should be depleted.
Look at datalog.txt to find out how long the Arduino was able to run.
Divide the last reported millis() time by 1000 to get seconds, then by 60 to get minutes, then by 60 to get hours.
 */

#include <SPI.h>
#include <SD.h>

unsigned long waitfor = 5000;

void setup() {

  pinMode(53,OUTPUT);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(10,11,12,13)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
}

void loop() {

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(millis());
    dataFile.close();
    // print to the serial port too:
    Serial.println(millis());
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }

  //burn copute cycles until next logging time
  int dummy = 0;
  while (millis() < waitfor)
  {
    dummy++;
  }
  
  waitfor += 5000;
}









