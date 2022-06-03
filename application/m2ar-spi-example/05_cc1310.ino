// *********************************************
// Raw accesss to CC1310 (Radio receiver)
// Was planned to use SPI, but code available using UART.
// Hardware routing will change in next PCB
// *********************************************
uint8_t useCC1310 = 1;
// until CC1310 is ready, use dummy functions

#define OUTPUT_ASCII_MODE   0

#define SPI_MODE    1
#define UART_MODE   0   /* Not supported */

#define COM_MODE    SPI_MODE

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
  
  Serial.println("CC1310 present");
  /* CC1310 SYNCHRO END PROCESS */
  
  Led4_Off();
#endif
  Serial.println("CC1310 ready");
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

#if (OUTPUT_ASCII_MODE == 1)
  Serial.print("Rx > [");
  Serial.print(l_u16FrameSize);
  Serial.print("] : ");
  for(l_u16BufferSize = 0u; l_u16BufferSize < l_u16FrameSize; l_u16BufferSize++)
  {
    Serial.print(l_au8Buffer[l_u16BufferSize], HEX);
  }
  Serial.println();
#else
  Serial.write(l_au8Buffer, l_u16FrameSize);
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
