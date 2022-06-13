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

#define CANBUS_PORT0   0  // Arduino MKR Canbus shield
#define CANBUS_PORT1   1  // Accessoire Miroobox Can 1
#define CANBUS_PORT2   2  // Accessoire Miroobox Can 2

#define CANBUS_LOOPBACK true
#define CANBUS_REALMODE false

bool canBusAvailable[3] = {false, false, false};

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

typedef void (*canbusIsrHandler_t)(void);

CanBusMCP2515_asukiaaa::Driver can0(CAN0_PIN_CS, CAN0_PIN_INT); // ChipsSelect, Interrupt
CanBusMCP2515_asukiaaa::Settings can0_settings(QUARTZ_FREQUENCY, BITRATE);

CanBusMCP2515_asukiaaa::Driver can1(CAN1_PIN_CS, CAN1_PIN_INT); // ChipsSelect, Interrupt
CanBusMCP2515_asukiaaa::Settings can1_settings(QUARTZ_FREQUENCY, BITRATE);

CanBusMCP2515_asukiaaa::Driver can2(CAN2_PIN_CS, CAN2_PIN_INT); // ChipsSelect, Interrupt
CanBusMCP2515_asukiaaa::Settings can2_settings(QUARTZ_FREQUENCY, BITRATE);

/* CAN 0 ISR function : */
void can0IsrHandle(void)
{
  can0.isr();
}
void can1IsrHandle(void)
{
  can1.isr();
}
void can2IsrHandle(void)
{
  can2.isr();
}

bool connectCanbus(bool loopback, CanBusMCP2515_asukiaaa::Driver *driver, CanBusMCP2515_asukiaaa::Settings *settings, void (*hCanBusIsrHandler)(void) )
{
  bool res = false;
    if( loopback ){
      settings->mOperationMode = CanBusMCP2515_asukiaaa::OperationMode::LoopBack;
    }else{
      settings->mOperationMode = CanBusMCP2515_asukiaaa::OperationMode::Normal;
    }
    uint16_t errorCode = 0u;  
    do{
      if(hCanBusIsrHandler)
        errorCode = driver->begin(*settings, hCanBusIsrHandler);
      else
        errorCode = driver->begin(*settings);
      
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


bool initCanbusPort(uint8_t u8Port, bool boLoopMode, void (*hCanBusIsrHandler)(void) )
{
  CanBusMCP2515_asukiaaa::Driver * hCan;
  CanBusMCP2515_asukiaaa::Settings * hCanSettings;
  
  Serial.print("Try to start CAN port ");
  Serial.print(u8Port);
  Serial.print(" : ");
  
  /* select CAN port driver */
  if(u8Port == CANBUS_PORT0)
  {
    hCan = &can0;
    hCanSettings = &can0_settings;
    
  }else if(u8Port == CANBUS_PORT1)
  {
    hCan = &can1;
    hCanSettings = &can1_settings;
    Can1_CsUnselected();
    Can1_Reset();
    delay(10);
    Can1_Run();
    
  }else if(u8Port == CANBUS_PORT2)
  {
    hCan = &can2;
    hCanSettings = &can2_settings;
    Can2_CsUnselected();
    Can2_Reset();
    delay(10);
    Can2_Run();
    
  }else{
    Serial.println("Unknown or unavailable CANBUS port");
    return false;
  }

  if( connectCanbus( boLoopMode, hCan, hCanSettings, hCanBusIsrHandler) ){
    canBusAvailable[u8Port] = true;
    Serial.println("READY");
  }else{
    Serial.print("FAILED ");
    Serial.println(hCanSettings->toString());
    canBusAvailable[u8Port] = false;
  }

  return canBusAvailable[u8Port];
}


bool connectAllCanbus(bool loopback)
{
  bool res = true;
  if(canBusAvailable[CANBUS_PORT0])
    res &= initCanbusPort(CANBUS_PORT0, loopback, can0IsrHandle);
    
  if(canBusAvailable[CANBUS_PORT1])
    res &= initCanbusPort(CANBUS_PORT1, loopback, can1IsrHandle);
    
  if(canBusAvailable[CANBUS_PORT2])
    res &= initCanbusPort(CANBUS_PORT2, loopback, can2IsrHandle);
  
  return res;
}

 void setupMainCanbus() {
   uint8_t i=0;
  //SPI.setClockDivider(SPI_CLOCK_DIV64);

  canBusAvailable[CANBUS_PORT0] = false;
  canBusAvailable[CANBUS_PORT1] = true;
  canBusAvailable[CANBUS_PORT2] = false;

  Serial.println("Init main CANBUS v001");
  connectAllCanbus(false);
  Serial.println("CANBUS init done.");
}


void loopCanbus() {
}

uint8_t sendCanbus(uint8_t u8Port, uint32_t u32FrameId, uint8_t *pu8Data, uint8_t u8DataSize)
{
    bool ok1 ;
    CanBusMCP2515_asukiaaa::Driver *driver;
    CanBusData_asukiaaa::Frame frame;
    
    /* select CAN port driver */
    if(u8Port == CANBUS_PORT0)
    {
      driver = &can0;
    }else if(u8Port == CANBUS_PORT1)
    {
      driver = &can1;
    }else if(u8Port == CANBUS_PORT2)
    {
      driver = &can2;
    }else{
      return 0u;
    }

    /* build can frame */
    frame.id = u32FrameId; //0x0CFE2300; //TODO : set last byte using Arduino BLE
    frame.ext = frame.id > 2048;
    frame.len = u8DataSize;
    
    Serial.print("Sending [0x");
    Serial.print(frame.id, HEX);
    Serial.print("] DLC= ");
    Serial.print(frame.len);
    Serial.print(" > ");
    
    for(uint8_t i=0; i<8; i++){
      if( i < u8DataSize ){
        frame.data[i] = pu8Data[i];
      }else{
        frame.data[i] = 0;
      }
      Serial.print(frame.data[i], HEX);
      Serial.print(":");
    }

    /* Call poll function to clean can port */
    /*driver->poll();  removed because CAN bus is initialized with Isr handler */
    /* Send frame on selected can */
    
    ok1 = driver->tryToSend(frame);
    
    /* add delay to wait MCP2515 IT, otherwise messages are delayed */ 
    /*delay(1); removed because CAN bus is initialized with Isr handler */
    
    /* Call poll function to manage it after sending */
    /*driver->poll(); removed because CAN bus is initialized with Isr handler */

  if(ok1)
  {
      Serial.println(">");
    return u8DataSize;
  }else{
      Serial.println("!");
    return 0;
  }
}
