#include <SD.h>

/* MFF0PLAY.ino for Arduino MEGA
 *  
 * Arduino SD Card MIDI Song Player - by Eric Rangell 
 * Adapted from open-source Arduino MIDI code.  Published in the public domain.
 *
 * Connect pin TX1 to MIDI socket pin 4, +5V to 220 ohm resistor to MIDI socket pin 5, pin 3 to GND
 * 
 * Connect DIP switches as follows:
 * +5V to all OFF positions of each switch
 * A ribbon cable connects Each ON position of each switch to the odd-numbered pins 23-37
 * Eight 470 ohm resistors from each ON position of each switch to GND
 * This allows selection of 255 directories (Directory #0 always plays silence).
 * 
 * Even numbered pins 22-36 are used to choose the starting song number within the selected directory
 * 
 * Store MIDI Format 0 files in SD directories named /001 /002 /003 etc.
 * File names must be: 001.mid, 002.mid, 003.mid, etc in the order to be played.
 * 
 * DIP switches on pins 23 through 37 (odd numbers) determine the directory number (binary with high bit on pin 37)
 * DIP swtiches are read at the beginning of the sketch, or when RESET is pressed.
 * If all DIP switches are OFF (0), then playback is stopped until a different directory number is set
 * 
 * You can wire the RESET button to a foot pedal, and make a box with switches or potentiometers to select the playlist.
 * Use your imagination and build what YOU want.
 * 
 * Set debugSong = true to debug songs that don't play.
 * For Type 1 Midi Files, program will attempt to play data from track #2.
 * It does not mix down tracks in Type 1 Midi Files, so they should be converted to Type 0.
 * 
 */
#define SD_BUFFER_SIZE 512 
#define HEADER_CHUNK_ID 0x4D546864  // MThd
#define TRACK_CHUNK_ID 0x4D54726B   // MTrk
#define DELTA_TIME_VALUE_MASK 0x7F
#define DELTA_TIME_END_MASK 0x80
#define DELTA_TIME_END_VALUE 0x80
#define EVENT_TYPE_MASK 0xF0
#define EVENT_CHANNEL_MASK 0x0F
#define NOTE_OFF_EVENT_TYPE 0x80
#define NOTE_ON_EVENT_TYPE 0x90
#define KEY_AFTERTOUCH_EVENT_TYPE 0xA0
#define CONTROL_CHANGE_EVENT_TYPE 0xB0
#define PROGRAM_CHANGE_EVENT_TYPE 0xC0
#define CHANNEL_AFTERTOUCH_EVENT_TYPE 0xD0
#define PITCH_WHEEL_CHANGE_EVENT_TYPE 0xE0
#define META_EVENT_TYPE 0xFF
#define SYSTEM_EVENT_TYPE 0xF0
#define META_SEQ_COMMAND 0x00
#define META_TEXT_COMMAND 0x01
#define META_COPYRIGHT_COMMAND 0x02
#define META_TRACK_NAME_COMMAND 0x03
#define META_INST_NAME_COMMAND 0x04
#define META_LYRICS_COMMAND 0x05
#define META_MARKER_COMMAND 0x06
#define META_CUE_POINT_COMMAND 0x07
#define META_CHANNEL_PREFIX_COMMAND 0x20
#define META_END_OF_TRACK_COMMAND 0x2F
#define META_SET_TEMPO_COMMAND 0x51
#define META_SMPTE_OFFSET_COMMAND 0x54
#define META_TIME_SIG_COMMAND 0x58
#define META_KEY_SIG_COMMAND 0x59
#define META_SEQ_SPECIFIC_COMMAND 0x7F


/* PINS */

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8

//For Arduino Mega, follow these instructions: https://learn.adafruit.com/adafruit-data-logger-shield/for-the-mega-and-leonardo
//(reverted to original SD libaray)

const int SSPIN = 53;
const int spi1 = 10;  // pins needed for SD.begin()
const int spi2 = 11;
const int spi3 = 12;
const int spi4 = 13;

//DIP SWITCHES FOR CHOOSING PLAYLIST DIRECTORY NUMBER
const int b7 = 37;
const int b6 = 35;
const int b5 = 33;
const int b4 = 31;
const int b3 = 29;
const int b2 = 27;
const int b1 = 25;
const int b0 = 23;

//DIP SWITCHES FOR CHOOSING STARTING SONG NUMBER
const int s7 = 36;
const int s6 = 34;
const int s5 = 32;
const int s4 = 30;
const int s3 = 28;
const int s2 = 26;
const int s1 = 24;
const int s0 = 22;

boolean file_opened = false;
boolean last_block = false;
boolean file_closed = false;

char currentSong[18];

uint16_t bufsiz=SD_BUFFER_SIZE;
uint8_t buf1[SD_BUFFER_SIZE];
uint16_t bytesread1;
uint16_t bufIndex;
uint8_t currentByte;
uint8_t previousByte;

int format;
int trackCount;
int timeDivision;

unsigned long deltaTime;
int eventType;
int eventChannel;
int parameter1;
int parameter2;

// The number of microseconds per quarter note (i.e. the current tempo)
long microseconds = 500000;
int index = 0;
unsigned long accDelta = 0;

boolean firstNote = true;
int currFreq = -1;
unsigned long lastMillis;

int inbyte = 1;
int currentSongNum = 1;

boolean debugSong = false;

/************ SDCARD STUFF ************/
Sd2Card card;
File thefile;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
 PgmPrint("error: ");
 SerialPrintln_P(str);
 if (card.errorCode()) {
   PgmPrint("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
 }
 while(1);
}
/************ SDCARD STUFF ************/

void setPinMode (int pin, int firstpin, int secondpin)
{
    if ((pin == firstpin) || (pin == secondpin))
    {
      pinMode(pin, OUTPUT);           
    }
    else
    {
      pinMode(pin, INPUT);
    }

}

void setup() {  
     
  if (debugSong)
  {
    Serial.begin(9600);
  }
  else
  {
    Serial1.begin(31250);
  }

  pinMode(b7, INPUT);
  pinMode(b6, INPUT);
  pinMode(b5, INPUT);
  pinMode(b4, INPUT);
  pinMode(b3, INPUT);
  pinMode(b2, INPUT);
  pinMode(b1, INPUT);
  pinMode(b0, INPUT);

  pinMode(s7, INPUT);
  pinMode(s6, INPUT);
  pinMode(s5, INPUT);
  pinMode(s4, INPUT);
  pinMode(s3, INPUT);
  pinMode(s2, INPUT);
  pinMode(s1, INPUT);
  pinMode(s0, INPUT);

  Serial.print("\nInitializing SD card...");

  pinMode(SSPIN, OUTPUT);
 
  if (!SD.begin(spi1, spi2, spi3, spi4)) {
    Serial.println("initialization failed!  Check wiring, if card inserted, and pin to use for your SD shield");
    // don't do anything more:
    while (1) ;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }
  
  //READ DIP SWITCHES TO GET DIRECTORY NUMBER
  int n=0;
  n = digitalRead(b7);
  n = (n*2) + digitalRead(b6);
  n = (n*2) + digitalRead(b5);
  n = (n*2) + digitalRead(b4);
  n = (n*2) + digitalRead(b3);
  n = (n*2) + digitalRead(b2);
  n = (n*2) + digitalRead(b1);
  inbyte = (n*2) + digitalRead(b0);

  //READ DIP SWITCHES TO GET STARTING SONG NUMBER
  int m=0;
  m = digitalRead(s7);
  m = (m*2) + digitalRead(s6);
  m = (m*2) + digitalRead(s5);
  m = (m*2) + digitalRead(s4);
  m = (m*2) + digitalRead(s3);
  m = (m*2) + digitalRead(s2);
  m = (m*2) + digitalRead(s1);
  currentSongNum = (m*2) + digitalRead(s0);

  // processing resumes with loop()  
 }

void SendAllNotesOff() 
{
  for (int channel = 0; channel < 16; channel++)
  {
    midiShortMsg(0xB0+channel,120,0); // all sounds off
    delay(1);
    midiShortMsg(0xB0+channel,123,0); // all notes off
    delay(1);
    for (int note = 00; note < 128; note++) 
    {
      midiShortMsg(0x90+channel, note, 0x00);  
      delay(1); 
    }
  }
}

void logs(String thestring) {
  if(!debugSong)
    return;
  
  Serial.println(thestring);
}

void logi(String label, int data) {
  if(!debugSong)
    return;
  
  Serial.print(label);
  Serial.print(": ");
  Serial.println(data);
}

void logl(String label, long data) {
  if(!debugSong)
    return;
  
  Serial.print(label);
  Serial.print(": ");
  Serial.println(data,HEX);
}


void logDivision(boolean major) {
  if(!debugSong)
    return;
  
  if(major) {
    Serial.println("===========================");    
  }
  else {
    Serial.println("----------------------");
  }
}


int readInt()
{
  return readByte() << 8 | readByte();
}


long readLong() {
  return (long) readByte() << 24 | (long) readByte() << 16 | (long)readByte() << 8 | (long)readByte();
}


byte getLastByte()
{
  return currentByte;
}

byte readByte()
{
  ReadMidiByte();
  return currentByte;
}


void ReadMidiByte()
{
  if (!file_opened)
  {
     if (thefile = SD.open(currentSong,FILE_READ))
     {
        logs("Opened file");
        file_opened = true;
        ReadNextBlock();
     }
  }
  previousByte = currentByte;
  currentByte = buf1[bufIndex];
  bufIndex++;
  if (bufIndex >= bytesread1)
  {
       if (last_block)
       {
          thefile.close();   
          file_closed = true;
          logs("\nDone");         
       }
       else
       {
           ReadNextBlock();
       }     
  }
}

void ReadNextBlock()
{
        bytesread1 = thefile.read(buf1,bufsiz);
        bufIndex = 0;
        if (bytesread1 < bufsiz)
        {
           logs("Last Block");
           last_block = true;
        }
        else
        {
          logi("BUF1 Bytes read",bytesread1);
        }
}

void processByte(uint8_t b)
{
  if (debugSong)
  {
    Serial.print(b,HEX);
    Serial.print(" ");
  }
}

void processChunk() {
  boolean valid = true;
  
  long chunkID = readLong();
  long size = readLong();
  
  logDivision(true);
  logi("Chunk ID", chunkID);
  logl("Chunk Size", size);
  
  if(chunkID == HEADER_CHUNK_ID) {
    processHeader(size);
    
    logi("File format", getFileFormat());
    logi("Track count", getTrackCount());
    logi("Time division", getTimeDivision());
  }
  else if(chunkID == TRACK_CHUNK_ID) {
    processTrack(size);
  }
}


/*
 * Parses useful information out of the MIDI file header.
 */
void processHeader(long size) {
  // size should always be 6
  // do we want to bother checking?
  
  format = readInt();
  trackCount = readInt();
  timeDivision = readInt();
  
  //logs("Processed header info.");
}

int getFileFormat() {
  return format;
}

int getTrackCount() {
  return trackCount;
}

int getTimeDivision() {
  return timeDivision;
}

/*
 * Loops through a track of the MIDI file, processing the data as it goes.
 */

void processTrack(long size) {
  long counter = 0;

  while(counter < size) {
    //logl("Track counter", counter);
    counter += processEvent();
  }
  
  
}


/*
 * Reads an event type code from the currently open file, and handles it accordingly.
 */
int processEvent() {
  //logDivision(false);
  
  int counter = 0;
  deltaTime = 0;
  
  int b;
  
  do {
    b = readByte();
    counter++;
    
    deltaTime = (deltaTime << 7) | (b & DELTA_TIME_VALUE_MASK);
  } while((b & DELTA_TIME_END_MASK) == DELTA_TIME_END_VALUE);
  
  //logi("Delta", deltaTime);
  
  b = readByte();
    counter++;

  boolean runningMode = true;
  // New events will always have a most significant bit of 1
  // Otherwise, we operate in 'running mode', whereby the last
  // event command is assumed and only the parameters follow
  if(b >= 128) {
    eventType = (b & EVENT_TYPE_MASK) >> 4;
    eventChannel = b & EVENT_CHANNEL_MASK;
    runningMode = false;
  }
  
  //logi("Event type", eventType);
  //logi("Event channel", eventChannel);
  
  // handle meta-events and track events separately
  if(eventType == (META_EVENT_TYPE & EVENT_TYPE_MASK) >> 4
     && eventChannel == (META_EVENT_TYPE & EVENT_CHANNEL_MASK)) {
    counter += processMetaEvent();
  }
  else {
    counter += processTrackEvent(runningMode, b);
  }
  
  return counter;
}

/*
 * Reads a meta-event command and size from the file, performing the appropriate action
 * for the command.  Currently, only tempo changes are handled
 */
int processMetaEvent() {
  int command = readByte();
  int size = readByte();
  
  //logi("Meta event length", size);
  
  for(int i = 0; i < size; i++) {
    byte data = readByte();
    
    switch(command) {
      case META_SET_TEMPO_COMMAND:
        processTempoEvent(i, data);
    }
  }
  
  return size + 2;
}

/*
 * Reads a track event from the file, either as a full event or in running mode (to
 * be determined automatically), and takes appropriate playback action.
 */
int processTrackEvent(boolean runningMode, int lastByte) {
  int count = 0;
  
  if(runningMode) {
    parameter1 = getLastByte();
  }
  else {
    parameter1 = readByte(); 
    count++;
  }
  
  //logi("Parameter 1", parameter1);
  
  int eventShift = eventType << 4;

  parameter2 = -2;  
  switch(eventShift) {
    case NOTE_OFF_EVENT_TYPE:
    case NOTE_ON_EVENT_TYPE:
    case KEY_AFTERTOUCH_EVENT_TYPE:
    case CONTROL_CHANGE_EVENT_TYPE:
    case PITCH_WHEEL_CHANGE_EVENT_TYPE:
    default:
      parameter2 = readByte();
      count++;

      //logi("Parameter 2", parameter2);
      
      break;
    case PROGRAM_CHANGE_EVENT_TYPE:
    case CHANNEL_AFTERTOUCH_EVENT_TYPE:
      parameter2 = -1;
      break;
  }
  
  if (parameter2 >= 0)
  {
    processMidiEvent(deltaTime, eventType*16+eventChannel, parameter1, parameter2);
  }
  else if (parameter2 == -1)
  {
    process2ByteMidiEvent(deltaTime, eventType*16+eventChannel, parameter1);
  }
  else {
    addDelta(deltaTime);
  }
  
  return count;
}


/*
 * Handles a tempo event with the given values.
 */
void processTempoEvent(int paramIndex, byte param) {
  byte bits = 16 - 8*paramIndex;
  microseconds = (microseconds & ~((long) 0xFF << bits)) | ((long) param << bits);
  
  //Serial.print("TEMPO:");
  //Serial.println(microseconds);
}
  
long getMicrosecondsPerQuarterNote() {
  return microseconds;
}

void addDelta(unsigned long delta) {
  accDelta = accDelta + delta;
}

void resetDelta() {
  accDelta = 0;
}

void processMidiEvent(unsigned long delta, int channel, int note, int velocity) {
  addDelta(delta);
  
  playback(channel, note, velocity, accDelta);
  index++;
  
  resetDelta();
}

void process2ByteMidiEvent(unsigned long delta, int channel, int value) {
  addDelta(delta);
  
  playback(channel, value, -1, accDelta);
  index++;
  
  resetDelta();
}


void playback(int channel, int note, int velocity, unsigned long delta) {
  unsigned long deltaMillis = (delta * getMicrosecondsPerQuarterNote()) / (((long) getTimeDivision()) * 1000);
  
  if(firstNote) {
    firstNote = false;
  }
  else {
    unsigned long currMillis = millis();
    
    int vel2use = 255;
    
    while(currMillis < lastMillis + deltaMillis)
    {
      //delay(lastMillis - currMillis + deltaMillis);
      int dly2use = 1;
      
      //Prevent display from interfering with timing of music
      if (lastMillis + deltaMillis - currMillis < 12)
      {
        dly2use= 0;
      }
      currMillis = millis();  
    }      
  }

  if (velocity < 0)
  {
      midi2ByteMsg (channel, note);
  }
  else
  {  
      midiShortMsg (channel, note, velocity);
  } 
  lastMillis = millis();
}

void midiShortMsg(int cmd, int pitch, int velocity) {  
  if (debugSong)
  {
    Serial.print(cmd,HEX);
    Serial.print(" ");
    Serial.print(pitch,HEX);
    Serial.print(" ");
    Serial.print(velocity,HEX);
    Serial.print(" ");
  }
  else
  {
    Serial1.write(cmd);
    Serial1.write(pitch);
    Serial1.write(velocity);
  }
}

void midi2ByteMsg(int cmd, int value) {
  Serial1.write(cmd);
  Serial1.write(value);
}

void loop()
{   
 if (debugSong)
 {
     Serial.print("TOP OF LOOP: inbyte=");
     Serial.println(inbyte);
 }
 SendAllNotesOff();
 int currentDirNum = inbyte;

 if (currentDirNum > 0)
 {   
     currentSong[0] = '/';
     currentSong[3] = currentDirNum % 10 + '0';
     int tempDirNum  = currentDirNum / 10;
     currentSong[2] = tempDirNum % 10 + '0';
     tempDirNum  = tempDirNum / 10;
     currentSong[1] = tempDirNum % 10 + '0';
     currentSong[4] = '/';  
     
     currentSong[7] = currentSongNum % 10 + '0';
     int tempSongNum = currentSongNum / 10;
     currentSong[6] = tempSongNum % 10 + '0';
     tempSongNum = tempSongNum / 10;
     currentSong[5] = tempSongNum % 10 + '0';
     
     currentSong[8] = '.';
     currentSong[9] = 'M';
     currentSong[10] = 'I';
     currentSong[11] = 'D';
     currentSong[12] = '\0';
     
     if (debugSong)
     {
       Serial.print("Current Song #: ");
       Serial.println(currentSong);
     }

     bool file_was_found = false;
     // test for existence of file - if it opens, close it - it will be reopened when chunks processed
     if (thefile = SD.open(currentSong,FILE_READ))
     {
       thefile.close();
       file_was_found = true;
       Serial.println("FILE FOUND ");
     }
     else
     {
       if (debugSong)
       {
           Serial.println("FILE NOT FOUND ");       
       }
       file_closed = 1;
     }
     
     //Phase processing
     while (!file_closed)
     {
        processChunk(); // header chunk
        
        if(getFileFormat() == 0) {
          logs("MIDI file format = 0");
          int trackCount = getTrackCount();
          logi("Track Count=",trackCount);

          // Uncomment to debug song buffer contents
          
          if (debugSong)
          {
            file_closed = true;
            for (int i=0; i<SD_BUFFER_SIZE; i++)
            {
              int b=buf1[i];
              Serial.print(b,HEX);
              Serial.print(" ");
              if (((i+1)/16)==((i+1)%16))
              {
                Serial.println();
              }
            }
          }
          else
          {
            for(int i = 0; i < getTrackCount(); i++) 
            {
              processChunk();
            }
            file_closed = true;
          }          
        }
        else 
        { 
          logs("MIDI file not format 0.");
        }
     } // while not file_closed

     if (file_was_found)
     {
        currentSongNum++; // will play next song in playlist
     }
     else
     {
        currentSongNum = 1; // will repeat playlist from first song
     }
     if (debugSong)
     {
        Serial.print("currentSongNum=");
        Serial.println(currentSongNum);
     }
     resetGlobalVars();
        
   } // if currentDirNum > 0
 } // loop()


void resetGlobalVars()
{  
  file_opened = false;
  last_block = false;
  file_closed = false;
  bufsiz=SD_BUFFER_SIZE;
  bytesread1 = 0;
  bufIndex = 0;
  currentByte = 0;
  previousByte = 0;

  format = 0;
  trackCount = 0;
  timeDivision = 0;

  deltaTime = 0;
  eventType = 0;
  eventChannel = 0;
  parameter1 = 0;
  parameter2 = 0;

  microseconds = 500000;
  index = 0;
  accDelta = 0;

  firstNote = true;
  currFreq = -1;
  lastMillis = 0;  
}


