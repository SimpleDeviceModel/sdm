byte virtualRegisters[16];

void setup() {
  for(byte i=0;i<16;i++) virtualRegisters[i]=i;
  Serial.begin(115200);
// Toggle the LED on to indicate that we are ready
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
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
}

void writeVirtualRegister(byte addr,byte data) {
  virtualRegisters[addr&0x0F]=data;
}

byte readVirtualRegister(byte addr) {
  return virtualRegisters[addr&0x0F];
}
