volatile unsigned int adcBuffer[256];
volatile byte adcWriteIndex=0;
byte adcReadIndex=0;
int debug_cnt=0;

byte pinState[14];
byte pinPWM[14];

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
  Serial.begin(115200);
}

void loop() {
  static byte cmdBuffer[3];
  static byte cmdBytes=0;

  if(Serial.available()) {
// Process commands from the PC
    byte ch=Serial.read();
    if(cmdBytes==0) {
// Look for a start of a packet
      if(ch==0x50||ch==0x51) {
        cmdBuffer[0]=ch;
        cmdBytes++;
      }
    }
    else {
// Packet continuation
      cmdBuffer[cmdBytes]=ch;
      cmdBytes++;
    }
    
    if(cmdBuffer[0]==0x50&&cmdBytes==3) {
// Write register
      writeVirtualRegister(cmdBuffer[1],cmdBuffer[2]);
      cmdBytes=0;
    }
    else if(cmdBuffer[0]==0x51&&cmdBytes==2) {
// Read register
      byte data=readVirtualRegister(cmdBuffer[1]);
      Serial.write(0x80|(data>>4)); // upper 4 bits
      Serial.write(data&0x0F); // lower 4 bits
      cmdBytes=0;
    }
  }
  else if(adcReadIndex!=adcWriteIndex) {
// Send ADC data
    unsigned int data=adcBuffer[adcReadIndex++];
    Serial.write(0xC0|(data>>5)); // upper 5 bits
    Serial.write(data&0x1F); // lower 5 bits
  }
}

void writeVirtualRegister(byte addr,byte data) {
  if(addr>1&&addr<14) {
    pinState[addr]=data;
    setPinState(addr);
  }
  if(addr>17&&addr<30) {
    pinPWM[addr-16]=data;
    setPinState(addr-16);
  }
}

byte readVirtualRegister(byte addr) {
  if(addr>1&&addr<14) return pinState[addr];
  if(addr>17&&addr<30) return pinPWM[addr-16];
  return 0;
}

void setPinState(byte pin) {
  switch(pinState[pin]) {
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
  case 4: // PWM
    analogWrite(pin,pinPWM[pin]);
    break;
  }
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
