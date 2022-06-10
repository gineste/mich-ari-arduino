// *********************************************
// Accesss to CANBUS using CanBusMCP2515_asukiaaa library
// Features :
// - listen
// - write to C4
// - handled commands from C4 (todo)
// - firmware upgrade from C4 (todo)
// - handle canbus errors nicely (use component shutdown if needed)  (todo)
// - autodetect canbus 2 speed (canbus 1 is at 500K by default) (todo)
//
// *********************************************

#include <CanBusMCP2515_asukiaaa.h>

void processMasterCan(CanBusData_asukiaaa::Frame *frame) ;

// CAN 0 targets the CANBUS shield
#ifndef CAN0_PIN_CS
#define CAN0_PIN_CS 3
#endif
#ifndef CAN0_PIN_INT
#define CAN0_PIN_INT 7
#endif
// CAN 1 targets MCP2515 on mainboard used to communicate with the C4 Dongle
#ifndef CAN1_PIN_CS
#define CAN1_PIN_CS 16
#endif
#ifndef CAN1_PIN_INT
#define CAN1_PIN_INT 17
#endif
// CAN 2 targets MCP2515 on mainboard, used to communicate with other external devices.
// Should be used by FPGA for performance reasons... later
#ifndef CAN2_PIN_CS
#define CAN2_PIN_CS 19
#endif
#ifndef CAN2_PIN_INT
#define CAN2_PIN_INT 18
#endif

#define CANBUS_LOOPBACK true
#define CANBUS_REALMODE false

uint8_t usePcbCan0 = 0; // Arduino MKR Canbus shield
uint8_t usePcbCan1 = 1; // Accessoire Miroobox Can 1
uint8_t usePcbCan2 = 1; // Accessoire Miroobox Can 2

//BLE may receive data asynchronously. It stores data to be sent here until canbus loop is called.
uint8_t canDataToSend = 0;
uint8_t canDataBuffer[256];


static const auto QUARTZ_FREQUENCY = 20UL * 1000UL * 1000UL; // 20MHz
static const auto BITRATE = CanBusMCP2515_asukiaaa::BitRate::Kbps500;
// IDS are used to send data on canbus
#ifndef CAN0_ID
#define CAN0_ID 3000
#endif
#ifndef CAN1_ID
#define CAN1_ID 3100
#endif
#ifndef CAN2_ID
#define CAN2_ID 3200
#endif

CanBusMCP2515_asukiaaa::Driver can0(CAN0_PIN_CS, CAN0_PIN_INT); // ChipsSelect, Interrupt
CanBusMCP2515_asukiaaa::Settings can0_settings(QUARTZ_FREQUENCY, BITRATE);

CanBusMCP2515_asukiaaa::Driver can1(CAN1_PIN_CS, CAN1_PIN_INT); // ChipsSelect, Interrupt
CanBusMCP2515_asukiaaa::Settings can1_settings(QUARTZ_FREQUENCY, BITRATE);

CanBusMCP2515_asukiaaa::Driver can2(CAN2_PIN_CS, CAN2_PIN_INT); // ChipsSelect, Interrupt
CanBusMCP2515_asukiaaa::Settings can2_settings(QUARTZ_FREQUENCY, BITRATE);


bool connectCanbus(bool loopback, CanBusMCP2515_asukiaaa::Driver *driver, CanBusMCP2515_asukiaaa::Settings *settings)
{
  bool res = false;
    if( loopback ){
      settings->mOperationMode = CanBusMCP2515_asukiaaa::OperationMode::LoopBack;
      Serial.print("Canbus loopback requested ");
    }else{
      settings->mOperationMode = CanBusMCP2515_asukiaaa::OperationMode::Normal;
      Serial.print("Canbus Normal mode requested ");
    }
    uint16_t errorCode = 0u;  
    do{
      errorCode = driver->begin(*settings);
      // uint16_t errorCode = can.begin(settings, [] { can.isr(); }); // attachInterrupt to INT pin
      if (errorCode == 0){
        Serial.println("Open CANBUS success");
        res = true;
      }else{
        Serial.print("Canbus Configuration error: ");
        Serial.println(CanBusMCP2515_asukiaaa::Error::toString(errorCode));
      }
    } while(errorCode != 0);

    return res;
}
bool connectAllCanbus(bool loopback)
{
  bool res = true;
  Serial.println(usePcbCan0);
  Serial.println(usePcbCan1);
  Serial.println(usePcbCan2);
  
  if( usePcbCan0 && res){
    Serial.print("Try to start CAN 0 : ");
    if( connectCanbus( loopback, &can0, &can0_settings) ){
    }else{
      Serial.print("FAILED ");
      Serial.println(can0_settings.toString());
      usePcbCan0 = 0;
      res = false;
    }
  }
  if( usePcbCan1 && res){
    Can1_CsUnselected();
    Can1_Reset();
    delay(10);
    Can1_Run();
    Serial.print("Starting CAN 1 : ");
    if( connectCanbus( loopback, &can1, &can1_settings) ){
    }else{
      Serial.print("FAILED ");
      Serial.println(can1_settings.toString());
      usePcbCan1 = 0;
      res = false;
    }
  }
  if( usePcbCan2 && res){
    Can2_CsUnselected();
    Can2_Reset();
    delay(10);
    Can2_Run();
    Serial.print("Starting CAN 2 : ");
    if( connectCanbus( loopback, &can2, &can2_settings) ){
    }else{
      Serial.print("FAILED ");
      Serial.println(can2_settings.toString());
      usePcbCan2 = 0;
      res = false;
    }
  }
  
  return res;
}

 void setupMainCanbus() {
   uint8_t i=0;
  //SPI.setClockDivider(SPI_CLOCK_DIV64);

  Serial.println("Init main CANBUS v001");
  Can1_CsUnselected();
  Can1_Reset();
  delay(10);
  Can1_Run();
  Serial.print("Starting CAN 1 : ");
  if( connectCanbus( false, &can1, &can1_settings) ){
  }else{
    Serial.print("FAILED ");
    Serial.println(can1_settings.toString());
    usePcbCan1 = 0;
  }
Serial.println("Init CANBUS done.");
}


void loopCanbus() {
}

uint8_t sendCanbus(uint32_t canId, uint8_t *buffer8, uint8_t size)
{
    CanBusData_asukiaaa::Frame frame;
    frame.id = canId; //0x0CFE2300; //TODO : set last byte using Arduino BLE
    frame.ext = frame.id > 2048;
    for(uint8_t i=0; i<8; i++){
      if( i < size ){
        frame.data[i] = buffer8[i];
      }else{
        frame.data[i] = 0;
      }
    }
    //frame.data64 = millis();
    
    bool ok1 ;
    Serial.print(millis());
    Serial.print(" Send ");
    Serial.print(size);
    Serial.print(" bytes on ");
    if( usePcbCan0 ){
      
      Serial.print("CAN0>");
      ok1 = can0.tryToSend(frame);
    }else{
      Serial.print("CAN1>");
      ok1 = can1.tryToSend(frame);
    }
    
    Serial.print(" [0x");
    Serial.print(frame.id, HEX);
    Serial.print("] ");
    for (int i = 0; i < frame.len; ++i) {
      Serial.print(frame.data[i], HEX);
      Serial.print(":");
    }
    
    if( ok1 ){
      can1.poll(); /* do not forget it ! */
      Serial.println(">");
    }else{
      Serial.println("!");
    }
    
    //delay(20);
    if( ok1 ){
      return size;
    }else{
      return 0;
    }
}
