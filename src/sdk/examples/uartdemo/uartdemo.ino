volatile unsigned int adcBuffer[256];
volatile byte adcWriteIndex=0;
byte adcReadIndex=0;
int debug_cnt=0;

byte pinState[14];

void setup() {
/* 
 * Configure the ADC 
 * ADMUX:  REFS=01 (AVCC reference), MUX=0000 (ADC0 input)
 * ADCSRB: ADTS=000 (free running mode)
 * ADCSRA: ADEN=1 (enable ADC), ADSC=1 (start conversion),
 *         ADATE=1 (enable auto trigger), ADIE=1 (enable ADC interrupt),
 *         ADPS=111 (prescaler=128)
 */
  ADMUX=0x40;
  ADCSRB=0;
  ADCSRA=0xEF;
// Set initial pin states, but don't touch UART pins
  for(byte i=2;i<14;i++) setPinState(i,0);
  Serial.begin(115200);
}

void loop() {
  if(Serial.available()) {
    byte ch=Serial.read();
    if(ch==0x50) { // Write register
      while(!Serial.available());
      byte addr=Serial.read();
      while(!Serial.available());
      byte data=Serial.read();
      writeVirtualRegister(addr,data);
    }
    else if(ch==0x51) { // Read register
      while(!Serial.available());
      byte addr=Serial.read();
      byte data=readVirtualRegister(addr);
      Serial.write(0x80|(data>>4)); // upper 4 bits
      Serial.write(data&0x0F); // lower 4 bits
    }
  }
  else if(adcReadIndex!=adcWriteIndex) {
    unsigned int data=adcBuffer[adcReadIndex++];
    Serial.write(0xC0|(data>>5)); // upper 5 bits
    Serial.write(data&0x1F); // lower 5 bits
  }
}

void writeVirtualRegister(byte addr,byte data) {
  if(addr>1&&addr<14) setPinState(addr,data);
}

byte readVirtualRegister(byte addr) {
  if(addr==0) return adcWriteIndex;
  if(addr>1&&addr<14) return pinState[addr];
  return 0;
}

void setPinState(byte pin,byte state) {
  switch(state) {
  case 0: // input
    pinMode(pin,INPUT);
    break;
  case 1: // input_pullup
    pinMode(pin,INPUT_PULLUP);
    break;
  case 2: // force low
    pinMode(pin,OUTPUT);
    digitalWrite(pin,LOW);
    break;
  case 3: // force high
    pinMode(pin,OUTPUT);
    digitalWrite(pin,HIGH);
    break;
  }
  pinState[pin]=state;
}

ISR(ADC_vect) {
// Write every second sample to adcBuffer
  static byte cnt=0;
  cnt=(cnt+1)&0x01;
  if(cnt==0) {
    adcBuffer[adcWriteIndex++]=ADCL|(ADCH<<8);
/*    adcBuffer[adcWriteIndex++]=debug_cnt;
    debug_cnt++;
    if(debug_cnt==1000) debug_cnt=0;*/
  }
}
