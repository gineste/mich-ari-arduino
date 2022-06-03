


#include <Wire.h>
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if (!Serial){
    delay(1000);
    Serial.begin(115200);
    Serial.println("Serial opened");
  }
    
  initIOs();
  Serial.println("Begin I2C");
  Wire.begin();
  if(initI2CExpander())
  {
   Led2_Red();
    setupCC1310();
   Led2_Off();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  loopCC1310();
}
