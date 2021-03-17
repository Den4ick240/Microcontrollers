char codes[] = {0x48, 0x90, 0x94, 0x70, 0x20, 
                0x84, 0x78, 0x80, 0x40, 0x8e, 
                0x74, 0x88, 0x58, 0x50, 0x7c, 
                0x8c, 0x9a, 0x68, 0x60, 0x30, 
                0x64, 0x82, 0x6c, 0x92, 0x96, 
                0x98, 0xbf, 0xaf, 0xa7, 0xa3, 
                0xa1, 0xa0, 0xb0, 0xb8, 0xbc};
const int bufferSize = 256;
char buffer[bufferSize] = {0};
int readPos = 0, writePos = 0;

const int outputBufferSize = 16;
char outputBuffer[outputBufferSize] = {0};
int outputReadPos = 0, outputWritePos = 0;

int timeUnit = 100;
int dotTime = timeUnit;
int longTime = timeUnit * 3;
int timeBetweenWords = timeUnit * 7;
int timeBetweenChars = timeUnit * 3;
int timeBetweenSignals = timeUnit;

int timeError = (longTime - dotTime) / 2;

char outputState = 0
   , code = 0
   , codePos = 0
   , codeLen = 0;

const int outputPin = 4;
const int soundPin = 3;
const int frequency = 1300;

const int interruptPin = 2;

char buttonState, lastButtonState;
int lastChangeTime;
char currentCode = 0;
char pauseState = 2;

void dropChar() {
  outputBuffer[outputWritePos++] = currentCode;
  currentCode = 0;
 // Serial.println("dropped");
}

void onPause() {
  if (pauseState == 2) return;
  int currentTime = millis();
  int deltaTime = currentTime - lastChangeTime;
  if (pauseState == 1 && deltaTime > timeBetweenWords - timeError) { 
    dropChar();
    pauseState = 2;
  } else if (pauseState == 0 && deltaTime > timeBetweenChars - timeError) {
    dropChar();
    pauseState = 1;
  }
}

void changeInterrupt() {
  int currentTime = millis();
  int deltaTime = currentTime - lastChangeTime;
  if (digitalRead(interruptPin) == HIGH) {
    pauseState = 0;
    char len = (currentCode >> 5) & 7;
	if (len >= 5) {
      dropChar();
      len = 0;
    }
    char b = (((longTime - timeError) > deltaTime) ? 0 : 1);
    currentCode &= 0x1f;
    currentCode |= (len + 1) << 5;
    currentCode |= b << (4 - len);
   // Serial.print((char)b);
    //Serial.print(" len: ");
    //Serial.println((int)len);
    
  } else { 
  	onPause();
    pauseState = 2;
  }
  lastChangeTime = currentTime;
}

void setup()
{
  Serial.begin(9600);
  pinMode(outputPin, OUTPUT);
  pinMode(soundPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), changeInterrupt, CHANGE);
}

void loop()
{
  char ch;
  if (Serial.available() > 0) {
    ch = Serial.read();
    if (ch != -1)
      buffer[writePos++] = ch;
    if (writePos == bufferSize) writePos = 0;
  }
  if (outputState == 0 && readPos != writePos) {
    ch = buffer[readPos++];
    if ('a' <= ch && ch <= 'z') {
      code = codes[ch - 'a'];
    } else if ('A' <= ch && ch <= 'Z') {
      code = codes[ch - 'A'];
    } else if ('0' <= ch && ch <= '9') {
      code  = codes[ch + 26 - '0'];
    }
    codeLen = 0x07 & (code >> 5);
    codePos = 0;
    if (ch == ' ') {
      delay(timeBetweenWords);
    } else {
      delay(timeBetweenChars);
      outputState = 1;
    }
  }
  if (outputState == 1) {
    digitalWrite(outputPin, HIGH);
    //analogWrite(soundPin, 100);
    tone(soundPin, frequency);
    if (1 & (code >> (4 - codePos++))) {
      //Serial.println("-");
      delay(longTime);
    } else {
      delay(dotTime);
      //Serial.println("*");
    }
    digitalWrite(outputPin, LOW);
    noTone(soundPin);
    if (codePos == codeLen){
      outputState = 0;
    } else {
      delay(timeBetweenSignals);
    }
  }
  
  
  cli();
  if (digitalRead(interruptPin) == HIGH) {
    onPause();
  }
  if (outputReadPos != outputWritePos) {
    char code = outputBuffer[outputReadPos++];
    if (code == 0) {
      Serial.print('\n');
    } else {
      char i, len = (sizeof(codes) / sizeof(char));
      for (i = 0; i < len; i++) {
        if (codes[i] == code) break;
      }
      if (i == len) {
       	Serial.print('?'); 
      } else if (i <= ('z' - 'a')) {
        Serial.print((char)('a' + i));
      } else {
        Serial.print((char)('0' + i));
      }
    }
  }
  sei();
}
