// *********************************************
// Raw accesss to CC1310 (Radio receiver)
// Was planned to use SPI, but code available using UART.
// Hardware routing will change in next PCB
// *********************************************
uint8_t useCC1310 = 1;
// until CC1310 is ready, use dummy functions
#define OUTPUT_ASCII  0
#define OUTPUT_USB    1
#define OUTPUT_CAN    2

#define OUTPUT_MODE   OUTPUT_CAN

#define SPI_MODE    1
#define UART_MODE   0   /* Not supported */

#define COM_MODE    SPI_MODE
#define ESP_MAGIC_AA_IDX      0
#define ESP_MAGIC_55_IDX      1
#define ESP_SC_IDX            2
#define ESP_DS_IDX            3
#define ESP_LEN_IDX           4
#define ESP_OPCODE_IDX        6
#define ESP_PAYLOAD_IDX       7


#define CAN_ADDR       0xACC4F

typedef enum _CAN_COMMANDS_ {
  CAN_CMD_TAG_INFO            = 0x20u,
  CAN_CMD_TAG_DATA            = 0x21u,
  CAN_CMD_TAG_SETTINGS        = 0x22u,
  CAN_CMD_TAG_MSG             = 0x23u,
  CAN_CMD_DEBUG               = 0x7Fu,
} eCanbus_Commands_t;

typedef struct __attribute__((__packed__)) _CANBUS_TPMS_INFO_MSG_
{
  uint32_t u32TagId;      // 4
  uint8_t u8TagCnt;       // 1
  uint8_t u8TagType;      // 1
  uint8_t u8TagRssi;      // 1
  /*uint8_t u8MsgId;*/        // 1
} sCanbusTpmsInfoMsg_t;

typedef struct __attribute__((__packed__)) _CANBUS_TPMS_DATA_MSG_
{
  uint8_t au8TagData[8];          // 8 > data counter
} sCanbusTpmsDataMsg_t;

void setupCC1310(void)
{
  if( !useCC1310 ){
    return;
  }
  Led3_Green();
  
  Daughter_PowerOff();
  delay(1000);
  Daughter_PowerOn();
  Serial.println("Init CC1310 v001");
  CC1310_Reset();
  delay(100);
  CC1310_Run();
  Led3_Off();
  

#if COM_MODE == SPI_MODE

  Led4_Yellow();
  /* CC1310 SYNCHRO START PROCESS */
  /* Do not modify this sequence */
  while(digitalRead(2) == 1);
  CC1310_CsUnSelected();
  while(digitalRead(2) == 0);
  CC1310_CsSelected();
  while(digitalRead(2) == 1);
  CC1310_CsUnSelected();
  
#if (OUTPUT_MODE == OUTPUT_ASCII)
  Serial.println("CC1310 present");
#endif
  /* CC1310 SYNCHRO END PROCESS */
  
  Led4_Off();
#endif
#if (OUTPUT_MODE == OUTPUT_ASCII)
  Serial.println("CC1310 ready");
#endif

#if (OUTPUT_MODE == OUTPUT_CAN)
  sendCanbus(CANBUS_PORT1, (CAN_ADDR << 9) | CAN_CMD_DEBUG, (uint8_t*) "CC1310", 6u);
#endif
}

void readCC1310(void)
{
    uint8_t l_au8Buffer[120] = {0};
    uint16_t l_u16FrameSize = 0u;
    uint16_t l_u16BufferSize = 0u;
    
#if COM_MODE == SPI_MODE
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

    l_u16BufferSize = 0u;
    l_u16FrameSize = 120;

    while(l_u16BufferSize < l_u16FrameSize)
    {
      CC1310_CsSelected();
      l_au8Buffer[l_u16BufferSize] = SPI.transfer(l_u16BufferSize&0xff);
      ++l_u16BufferSize;
      CC1310_CsUnSelected();

      if(l_au8Buffer[0] != 0xAA)
      { /* Frame start error */
        l_u16BufferSize = 0u;
      }

      if(l_u16BufferSize == 6u)
      {
        /* Frame header received */
        l_u16FrameSize = (l_au8Buffer[5] << 8) | l_au8Buffer[4];
      }
    }
    
    SPI.endTransaction();
    delay(10);
#endif

#if (OUTPUT_MODE == OUTPUT_ASCII)
  Serial.print("Rx > [");
  Serial.print(l_u16FrameSize);
  Serial.print("] : ");
  for(l_u16BufferSize = 0u; l_u16BufferSize < l_u16FrameSize; l_u16BufferSize++)
  {
    Serial.print(l_au8Buffer[l_u16BufferSize], HEX);
  }
  Serial.println();
#elif (OUTPUT_MODE == OUTPUT_USB)
  Serial.write(l_au8Buffer, l_u16FrameSize);
  
#elif (OUTPUT_MODE == OUTPUT_CAN)
  if(l_au8Buffer[ESP_OPCODE_IDX] == 0x40)
  {
    uint8_t * l_pu8Payload = &(l_au8Buffer[ESP_PAYLOAD_IDX]);
    sCanbusTpmsInfoMsg_t l_sCanTpmsInfoMsg;
    uint16_t l_u16TagLen = 0;
    uint32_t l_u32CanFrameId = 0;
  
    l_sCanTpmsInfoMsg.u8TagType = l_pu8Payload[0];
    l_u16TagLen = l_pu8Payload[1] + (l_pu8Payload[2] << 8);
    l_sCanTpmsInfoMsg.u8TagCnt = l_pu8Payload[3+l_u16TagLen-2];
  
    if(l_pu8Payload[3+l_u16TagLen-1] >= 128)
    {
      l_sCanTpmsInfoMsg.u8TagRssi = (uint8_t) (((l_pu8Payload[3+l_u16TagLen-1] - 256) / 2) - 74);
    }else{
      l_sCanTpmsInfoMsg.u8TagRssi = (uint8_t) (((l_pu8Payload[3+l_u16TagLen-1]      ) / 2) - 74);
    }
    l_sCanTpmsInfoMsg.u32TagId = (l_pu8Payload[3] << 0) + (l_pu8Payload[4] << 8) + (l_pu8Payload[5] << 16) + (l_pu8Payload[6] << 24);

    Serial.print("Rx > Type= ");
    Serial.print(l_sCanTpmsInfoMsg.u8TagType);
    Serial.print(", Id=0x");
    Serial.print(l_sCanTpmsInfoMsg.u32TagId, HEX);
    Serial.print(", Rssi= ");
    Serial.print((int8_t)l_sCanTpmsInfoMsg.u8TagRssi);
    Serial.print(", Cnt= ");
    Serial.println(l_sCanTpmsInfoMsg.u8TagCnt);
    
    l_u32CanFrameId = (CAN_ADDR << 9) | CAN_CMD_TAG_INFO;
    sendCanbus(CANBUS_PORT1, l_u32CanFrameId, (uint8_t*) &l_sCanTpmsInfoMsg, 8u);
    
    l_u32CanFrameId = (CAN_ADDR << 9) | CAN_CMD_TAG_DATA;
    sendCanbus(CANBUS_PORT1, l_u32CanFrameId, &(l_pu8Payload[3]), 8u);
  }
  
#endif
}

void loopCC1310(void)
{
  if( ! useCC1310 ){
    return;
  }
  
  if(digitalRead(2))
  {
    readCC1310();
  }
}
