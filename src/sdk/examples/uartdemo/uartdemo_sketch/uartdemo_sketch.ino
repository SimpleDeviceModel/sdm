/*
 * ADC sampling decimation factor. Only one of each DECIMATION_FACTOR
 * samples will be written to the buffer. Increase this value if you
 * are having problems with communication reliability.
 * 
 * The sampling frequency (in Hz) will be:
 * 
 *   Fs = 16000000 (CPU clock frequency) / 128 (ADC prescaler) /
 *        13 (cycles per conversion) / DECIMATION_FACTOR
 */

#define DECIMATION_FACTOR 2

/* 
 * A circular buffer to store up to 256 ADC samples. Indexes are of
 * "byte" type to make buffer overrun impossible.
 */

volatile unsigned int adcBuffer[256];
volatile byte adcWriteIndex=0;
byte adcReadIndex=0;
volatile bool triggered=false;

/* Digital pin states */

byte pinState[14];
byte pinPWM[14];

/* Synchronization settings */

byte syncMode=0; /* 0 - off, 1 - rising edge, 2 - falling edge */
byte syncLevel=128;
byte syncOffset=128;

void setup() {
/* 
 * Configure the ADC 
 * ADMUX:  REFS=01 (use AVCC for reference), MUX=0000 (use ADC0 input)
 * ADCSRB: ADTS=000 (free running mode, the new conversion will be started
 *         immediately after the previous one has been completed)
 * ADCSRA: ADEN=1 (enable ADC), ADSC=1 (start conversion),
 *         ADATE=1 (enable auto trigger), ADIE=1 (enable ADC interrupt),
 *         ADPS=111 (set ADC prescaler to 128)
 */
  ADMUX=0x40;
  ADCSRB=0;
  ADCSRA=0xEF;
  Serial.begin(115200);
}

void loop() {
/* Buffer for incoming data, can hold up to 3 bytes */
  static byte cmdBuffer[3];
  static byte cmdBytes=0;

  if(Serial.available()) {
/* Process incoming data */
    byte ch=Serial.read();
    if(cmdBytes==0) {
/* Look for a start of a packet */
      if(ch==0x50||ch==0x51) {
        cmdBuffer[0]=ch;
        cmdBytes++;
      }
    }
    else {
/* Packet continuation */
      cmdBuffer[cmdBytes]=ch;
      cmdBytes++;
    }
    
    if(cmdBuffer[0]==0x50&&cmdBytes==3) {
/* Write register */
      writeVirtualRegister(cmdBuffer[1],cmdBuffer[2]);
      cmdBytes=0;
    }
    else if(cmdBuffer[0]==0x51&&cmdBytes==2) {
/* Read register */
      byte data=readVirtualRegister(cmdBuffer[1]);
      byte buf[2];
      buf[0]=0x80|(data>>4); /* upper 4 bits */
      buf[1]=data&0x0F; /* lower 4 bits */
      Serial.write(buf,2);
      cmdBytes=0;
    }
  }
  else if(adcWriteIndex!=adcReadIndex) {
/* No incoming data, transmit data from the ADC circular buffer if present */
    static byte n=0;
    byte queue=adcWriteIndex-adcReadIndex;
    if(n==0&&syncMode!=0) { /* wait for trigger */
/* clear the trigger flag if there are not enough data in the buffer */
      if(queue<syncOffset) {
        triggered=false;
        return;
      }
      if(!triggered) {
        if(queue>syncOffset) adcReadIndex++;
        return;
      }
      triggered=false;
    }
    unsigned int data=adcBuffer[adcReadIndex];
    //unsigned int data=(unsigned char)(adcWriteIndex-adcReadIndex);
    adcReadIndex++;
    byte buf[2];
    buf[0]=0xC0|(data>>5); /* upper 5 bits */
    if(n==0) buf[0]|=0x20; /* start of packet mark */
    buf[1]=data&0x1F; /* lower 5 bits */
    Serial.write(buf,2);
    n++;
    triggered=false;
  }
}

void writeVirtualRegister(byte addr,byte data) {
/* ADC input channel */
  if(addr==0) ADMUX=(ADMUX&0xF0)|(data&0x0F);
/* ADC reference voltage */
  else if(addr==1) ADMUX=(ADMUX&0x3F)|(data<<6);
/* Pin mode registers */
  else if(addr>=2&&addr<=13) {
    pinState[addr]=data;
    setPinState(addr);
  }
/* PWM value registers */
  else if(addr>=18&&addr<=29) {
    pinPWM[addr-16]=data;
    setPinState(addr-16);
  }
/* Synchronization settings */
  else if(addr==32) syncMode=data;
  else if(addr==33) syncLevel=data;
  else if(addr==34) syncOffset=data;
}

byte readVirtualRegister(byte addr) {
  if(addr==0) return ADMUX&0x0F;
  else if(addr==1) return ADMUX>>6;
  else if(addr>=2&&addr<=13) return pinState[addr];
  else if(addr>=18&&addr<=29) return pinPWM[addr-16];
  else if(addr==32) return syncMode;
  else if(addr==33) return syncLevel;
  else if(addr==34) return syncOffset;
  return 0;
}

void setPinState(byte pin) {
  switch(pinState[pin]) {
  case 0: /* input */
    pinMode(pin,INPUT);
    break;
  case 1: /* input_pullup */
    pinMode(pin,INPUT_PULLUP);
    break;
  case 2: /* force low */
    pinMode(pin,OUTPUT);
    digitalWrite(pin,LOW);
    break;
  case 3: /* force high */
    pinMode(pin,OUTPUT);
    digitalWrite(pin,HIGH);
    break;
  case 4: /* PWM */
    analogWrite(pin,pinPWM[pin]);
    break;
  }
}

/* Process the ADC interrupt */

ISR(ADC_vect) {
  static byte cnt=0;
  if(++cnt==DECIMATION_FACTOR) cnt=0;
  if(cnt==0) {
    int sample=ADCL|(ADCH<<8);
    if(syncMode==1&&sample>=syncLevel*4&&adcBuffer[adcWriteIndex-1]<syncLevel*4) triggered=true;
    else if(syncMode==2&&sample<=syncLevel*4&&adcBuffer[adcWriteIndex-1]>syncLevel*4) triggered=true;
    adcBuffer[adcWriteIndex++]=sample;
  }
}
