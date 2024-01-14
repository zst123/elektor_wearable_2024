
// timing is everything here
#define DELAY     1u // width a pulse on the wire (read: 1 unit!), period is 2 units
#define T_CYCLE_0 4u // time to wait for the cycle to complete (before the next bit can be sent) if a 0 was send (4 units)
#define T_CYCLE_1 1u // time to wait for the cycle to complete (before the next bit can be sent) if a 1 was send (1 unit)

void TLC59731_writeZero(int mcuPin) {
  // start next cycle
  digitalWrite(mcuPin, HIGH);   
  delayMicroseconds(DELAY); // the pulse on the wire does not really have a width of 1us, since the write operation needs time too
  digitalWrite(mcuPin, LOW);   
  delayMicroseconds(DELAY);
  // for "0" data bit simply let timeout the cylce
  delayMicroseconds(T_CYCLE_0);
}

void TLC59731_writeOne(int mcuPin) {
  // start next cycle
  digitalWrite(mcuPin, HIGH);   
  delayMicroseconds(DELAY); // the pulse on the wire does not really have a width of 1us, since the write operation needs time too
  digitalWrite(mcuPin, LOW);   
  delayMicroseconds(DELAY);
  // write "1" data bit
  digitalWrite(mcuPin, HIGH);   
  delayMicroseconds(DELAY); 
  digitalWrite(mcuPin, LOW);
  delayMicroseconds(T_CYCLE_1);
}

void TLC59731_endOfSequence() {
  //28 * 5 us, based on datasheet: 3.5x tCycle - 5.5 tCycle 
  delayMicroseconds(140); //140       //112
}

void TLC59731_gsDataLatchSequence() {
  //28 * 10 us, based on datasheet: minimum 8.0x tCycle
  delayMicroseconds(280);  
}

void TLC59731_writeData(int mcuPin, byte data) {
  for (byte i = 0; i<8; i++) {
    if(data & B10000000) {
      TLC59731_writeOne(mcuPin);
    } else { 
      TLC59731_writeZero(mcuPin);
    }
    data <<= 1;
  }
}

void TLC59731_setLed(int mcuPin, byte out0, byte out1, byte out2) {
  //Start sequence + 0x3A 
  TLC59731_writeZero(mcuPin);
  TLC59731_writeZero(mcuPin);
  TLC59731_writeOne(mcuPin);
  TLC59731_writeOne(mcuPin);
  TLC59731_writeOne(mcuPin);
  TLC59731_writeZero(mcuPin);
  TLC59731_writeOne(mcuPin);
  TLC59731_writeZero(mcuPin);

  //RGB (OUT0-2) values
  TLC59731_writeData(mcuPin,out0);
  TLC59731_writeData(mcuPin,out1);
  TLC59731_writeData(mcuPin,out2);

  TLC59731_endOfSequence();
}