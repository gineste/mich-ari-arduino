// *********************************************
// Handle low level hardware access
// Features :
// - Init io pins
// - Init I2C extender that manages power and enable for several chips
// - Low level shared functions for I2C (mainly user for LSM6 and LIS2)
// *********************************************
// To use CAN2 on MRK1010, or without FPGA programming, connect
// D9  - SPI CLK  => D0
// D10 - SPI MISO => D20
// D8  - SPI MOSI => D21
#include "arduino.h"
#include <Wire.h>
#include <SPI.h>

//#define ACC_USE_BLE 1

#define PCA_LED2_GREEN 7
#define PCA_LED2_RED   3

#define PCA_LED3_GREEN 4
#define PCA_LED3_RED   5

#define PCA_LED4_GREEN 6
#define PCA_LED4_RED   7

#define PCA_LOW  2
#define PCA_HIGH 3

uint8_t pcaLow;
uint8_t pcaHigh;

void initIOs()
{
  Serial.println("Init IOs start");
  //AREF = LIS2_INT  ?? Comment configurer AREF ?
  //MKR_RESET = TP1

  //A0=D15, A1=D16, A2=D17, A3=D18, A4=D19, A5=D20, A6=D21
  pinMode(0, INPUT); //SPI_FPGA_CLK see D19, D20 an D21
  pinMode(1, OUTPUT); //CC1310_CSn  => Con20.4
  digitalWrite(1, 1); // Not selected
  pinMode(2, INPUT_PULLUP); //CC1310-INT  => Con20.2
  pinMode(3, INPUT_PULLUP); //S2LP_GPIO1  => Con20.1
  pinMode(4, OUTPUT); // LSM6 + LIS2 Power supply
  digitalWrite(4, 1);
  pinMode(5, OUTPUT); //TP3 
  digitalWrite(5, 0);
  pinMode(6, OUTPUT); //S2LP_CSn     => Con20.14
  digitalWrite(6, 1); // Not selected MUST BE 1
  pinMode(7, INPUT_PULLUP); //S2LP_GPIO0   => Con20.10
//  pinMode(8, OUTPUT);   // COPI=MOSI => Con20.3
  // Set High drive strength 
//  PORT->Group[g_APinDescription[8].ulPort].PINCFG[g_APinDescription[8].ulPin].bit.DRVSTR = 1;
//  digitalWrite(8, 0);
//  pinMode(9, OUTPUT);   // SPI CLOCK => Con20.11
  // Set High drive strength 
//  PORT->Group[g_APinDescription[9].ulPort].PINCFG[g_APinDescription[9].ulPin].bit.DRVSTR = 1;
//  digitalWrite(9, 0);

//  pinMode(10, INPUT_PULLUP);  // CIPO=MISO => Con20.7
  pinMode(11, INPUT);  // SDA => Reconfigured at I2C init time. Force 0 to drain possible powering though IOs
  //digitalWrite(11, 0); 
  pinMode(12, INPUT);  // SCL => Reconfigured at I2C init time
  //digitalWrite(12, 0); 

//  pinMode(13, INPUT_PULLUP);  // RX   => Con20.18
//  pinMode(14, OUTPUT);  // TX   => Con20.19
  //digitalWrite(14, 1); 
  pinMode(15, INPUT);  // LSM6_INT1.
  //digitalWrite(15, 0); 

  pinMode(16, OUTPUT);  // CAN1_CSn   
  digitalWrite(16, 1);   // Keep unselected

  pinMode(17, INPUT_PULLUP);  // CAN1 INTn  

  pinMode(18, INPUT_PULLUP);  // CAN2_INTn 
  pinMode(19, OUTPUT);  // CAN2_CSn (should be FPGA)
  digitalWrite(19, 1); // Keep unselected
  pinMode(20, INPUT); // FPGA_SPI_MISO
  pinMode(21, INPUT); // FPGA_SPI_MOSI  (keep input until FPGA is ready)  
  //digitalWrite(21, 0); //  (keep input until FPGA is ready) 

  SPI.begin(); // <<<<<<<<<<<< AJ
  Spi1_CsOff();
  // 
  // MCP2515 *2 : SPI
  // LSM6DSL    : I2C
  // LIS2MDL    : I2C
  // PCA9535DBQ : I2C

// GND  => Con20.5,9,16,17,12,16,20
// 3.3V => Con20.15
  Serial.println("Init IOs end");
}

bool pcaWriteRegister(uint8_t reg, uint8_t data)
{
  Wire.beginTransmission(0x22);
  Wire.write(reg);
  Wire.write(data);
  uint8_t error = Wire.endTransmission();
  if( error ){
    Serial.print("I2C PCA Write error [");Serial.print(reg);Serial.println("]=/=");Serial.print(data);
  }else{
    //Serial.print("I2C PCA Write [");Serial.print(reg);Serial.println("]=");Serial.print(data);Serial.print(" done");
  }
  return error;
}

uint8_t pcaReadRegister(uint8_t reg)
{
  uint8_t ch0 = 0x00;
  // read channel 0 first
  Wire.beginTransmission(0x22);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(0x22,1); // read
  if(Wire.available())
  {
    ch0 = Wire.read(); // read channel 0
  }else{
    Serial.println("I2C PCA Read error");
  }
  Wire.endTransmission();  
  return ch0;
}


bool initI2CExpander()
{
  Serial.println("initI2CExpander start");
  pcaLow  = 0;
  pcaHigh = 0;
  bitSet  (pcaHigh,PCA_LED2_GREEN);
  bitSet  (pcaLow,PCA_LED2_RED);
  bitSet  (pcaLow,PCA_LED3_RED);
  bitSet  (pcaLow,PCA_LED3_GREEN);
  bitSet  (pcaLow,PCA_LED4_RED);
  bitSet  (pcaLow,PCA_LED4_GREEN); 

  pcaWriteRegister(PCA_LOW, pcaLow); 
  pcaWriteRegister(PCA_HIGH, pcaHigh);

  pcaWriteRegister(4, 0 ); // Polarity Inversion Registers)
  pcaWriteRegister(5, 0 );
  pcaWriteRegister(6, 3 ); // Configuration Registers
  pcaWriteRegister(7, 0 );

  uint8_t u6 = pcaReadRegister(6);
  if( u6 != 3 ){
    Serial.print("Failed to init I2C Expander ");
    Serial.print(u6);
    Serial.println(" instead of 3 ");
    return 0;
  }else{
    Serial.println("Init I2C Expander done");
    return 1;
  }
}



void Led2_Yellow(){bitClear(pcaLow,PCA_LED2_RED);bitClear(pcaHigh,PCA_LED2_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void Led2_Green (){bitSet  (pcaLow,PCA_LED2_RED);bitClear(pcaHigh,PCA_LED2_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void Led2_Red   (){bitClear(pcaLow,PCA_LED2_RED);bitSet  (pcaHigh,PCA_LED2_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void Led2_Off   (){bitSet  (pcaLow,PCA_LED2_RED);bitSet  (pcaHigh,PCA_LED2_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); pcaWriteRegister(PCA_HIGH, pcaHigh); }

void Led3_Yellow(){bitClear(pcaLow,PCA_LED3_RED);bitClear(pcaLow,PCA_LED3_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }
void Led3_Green (){bitSet  (pcaLow,PCA_LED3_RED);bitClear(pcaLow,PCA_LED3_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }
void Led3_Red   (){bitClear(pcaLow,PCA_LED3_RED);bitSet  (pcaLow,PCA_LED3_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }
void Led3_Off(){bitSet  (pcaLow,PCA_LED3_RED);bitSet  (pcaLow,PCA_LED3_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }

void Led4_Yellow(){bitClear(pcaLow,PCA_LED4_RED);bitClear(pcaLow,PCA_LED4_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }
void Led4_Green (){bitSet  (pcaLow,PCA_LED4_RED);bitClear(pcaLow,PCA_LED4_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }
void Led4_Red   (){bitClear(pcaLow,PCA_LED4_RED);bitSet  (pcaLow,PCA_LED4_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }
void Led4_Off   (){bitSet  (pcaLow,PCA_LED4_RED);bitSet  (pcaLow,PCA_LED4_GREEN); pcaWriteRegister(PCA_LOW, pcaLow); }

void Leds_Off()
{
  	Led2_Off();
		Led3_Off();
		Led4_Off();
}

void testLeds()
{
	uint8_t i = 0;
  int loopCounter = 0;
  
  	for( i=0; i<1; i++){
Serial.print("LED Loop "); Serial.println(i);
		Led2_Off();
		Led3_Off();
		Led4_Off();
		delay(500);
		
		Led2_Red();
		Led3_Green();
		Led4_Yellow();
		delay(500);


		Led2_Green();
		Led3_Yellow();
		Led4_Red();
		delay(500);	
		Led2_Yellow();
		Led3_Red();
		Led4_Green();
		delay(500);
	}
  Led2_Off();
	Led3_Off();
	Led4_Off();

/*	for( i=0; i<2; i++){
    Serial.print("Power Loop "); Serial.println(i);
    Spi1_CsOff();
		Daughter_PowerOff();
    Radio_PowerOff();
		Can1_Reset();
		Can2_Reset();
    Can1_CsUnselected();
    Can2_CsUnselected();
    CC1310_CsUnSelected();
		TP4_Off();
		TP5_Off();
		TP6_Off();
		TP7_Off();
		delay(500);
    Radio_PowerOn();

		Led2_Red();
    CC1310_CsSelected();
		delay(500);
    CC1310_CsUnSelected();
    Led2_Off();

		Led3_Green();
    S2LP_CsSelected();
		delay(500);
		S2LP_CsUnselected();
    Led3_Off();
	}
*/
}


void Can1_Run   (){bitSet  (pcaHigh,6); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void Can1_Reset (){bitClear(pcaHigh,6); pcaWriteRegister(PCA_HIGH, pcaHigh); }

void Can1_CsSelected  (){ digitalWrite(16, LOW); }
void Can1_CsUnselected(){ digitalWrite(16, HIGH); }

void Can2_Run   (){bitSet  (pcaLow,2); pcaWriteRegister(PCA_LOW, pcaLow); }
void Can2_Reset (){bitClear(pcaLow,2); pcaWriteRegister(PCA_LOW, pcaLow); }
void Can2_CsSelected  (){ digitalWrite(19, LOW); }
void Can2_CsUnselected(){ digitalWrite(19, HIGH); }

void TP4_On  (){bitSet  (pcaHigh,0); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void TP4_Off (){bitClear(pcaHigh,0); pcaWriteRegister(PCA_HIGH, pcaHigh); }

void TP5_On  (){bitSet  (pcaHigh,1); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void TP5_Off (){bitClear(pcaHigh,1); pcaWriteRegister(PCA_HIGH, pcaHigh); }

void TP6_On  (){bitSet  (pcaHigh,2); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void TP6_Off (){bitClear(pcaHigh,2); pcaWriteRegister(PCA_HIGH, pcaHigh); }

void TP7_On  (){bitSet  (pcaHigh,3); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void TP7_Off (){bitClear(pcaHigh,3); pcaWriteRegister(PCA_HIGH, pcaHigh); }

void Daughter_PowerOn (){bitSet  (pcaHigh,5); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void Daughter_PowerOff(){bitClear(pcaHigh,5); pcaWriteRegister(PCA_HIGH, pcaHigh); }


void CC1310_Run   (){bitSet  (pcaHigh,4); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void CC1310_Reset (){bitClear(pcaHigh,4); pcaWriteRegister(PCA_HIGH, pcaHigh); }
void CC1310_CsSelected  (){ digitalWrite(1, LOW); }
void CC1310_CsUnSelected(){ digitalWrite(1, HIGH); }

void LIS2_LSM6_PowerOn (){ digitalWrite(4, HIGH); }
void LIS2_LSM6_PowerOff(){ digitalWrite(4, LOW); }

void S2LP_CsSelected    (){ digitalWrite(6, LOW); }
void S2LP_CsUnselected  (){ digitalWrite(6, HIGH); }

void FLASH_CsSelected    (){  }
void FLASH_CsUnselected  (){  }


void Spi1_CsOff()
{
	S2LP_CsUnselected  ();
	CC1310_CsUnSelected();
  Can1_CsUnselected  ();
  Can2_CsUnselected  ();
}

bool CC1310_Int(){ if( digitalRead(2) ){ return 1;}else{return 0;}}
bool LSM6_Int  (){ if( digitalRead(15) ){ return 1;}else{return 0;}}

bool S2LP_Gpio0(){ if( digitalRead(7) ){ return 1;}else{return 0;}}
bool S2LP_Gpio1(){ if( digitalRead(3) ){ return 1;}else{return 0;}}
bool Tp2(){ if( digitalRead(4) ){ return 1;}else{return 0;}}
bool Tp3(){ if( digitalRead(5) ){ return 1;}else{return 0;}}

bool Sda(){ if( digitalRead(11) ){ return 1;}else{return 0;}}
bool Scl(){ if( digitalRead(12) ){ return 1;}else{return 0;}}

bool rx(){ if( digitalRead(13) ){ return 1;}else{return 0;}}
bool tx(){ if( digitalRead(14) ){ return 1;}else{return 0;}}


bool Can1_Int  (){ if( digitalRead(17) ){ return 1;}else{return 0;}}
bool Can2_Int  (){ if( digitalRead(18) ){ return 1;}else{return 0;}}

bool Spi2_Miso  (){ if( digitalRead(20) ){ return 1;}else{return 0;}}
bool Spi2_Mosi  (){ if( digitalRead(21) ){ return 1;}else{return 0;}}
bool Spi2_Clk   (){ if( digitalRead( 0) ){ return 1;}else{return 0;}}

/****************************************************************************
 *********************   I2C low level functions ****************************
 ****************************************************************************/

bool i2cWriteRegister(uint8_t device, uint8_t reg, uint8_t data)
{
  Wire.beginTransmission(device);
  Wire.write(reg);
  Wire.write(data);
  uint8_t error = Wire.endTransmission();
  if( error ){
    Serial.print("I2C Write error [");Serial.print(reg);Serial.println("]=/=");Serial.print(data);
  }else{
    //Serial.print("I2C LSM6 Write [");Serial.print(reg);Serial.println("]=");Serial.print(data);Serial.print(" done");
  }
  return error;
}

uint8_t i2cReadRegister( uint8_t device, uint8_t reg)
{
  uint8_t ch0 = 0x00;
  // read channel 0 first
  Wire.beginTransmission(device);
  Wire.write(reg);
  if( Wire.endTransmission() != 0 ){
    Serial.println("I2C Read error no answer");
  }else{
    Wire.requestFrom(device,1); // read
    if(Wire.available())
    {
ch0 = Wire.read(); // read channel 0
Serial.print("I2C Read ");Serial.println(ch0);
    }else{
Serial.println("I2C Read error");
    }
  }
  Wire.endTransmission();  
  return ch0;
}

int i2cReadBlock(uint8_t device, uint8_t address, uint8_t *buffer, uint8_t count)
{
	uint8_t got = 0x00;
	// read channel 0 first
	Wire.beginTransmission(device);
	Wire.write(address);
	if( Wire.endTransmission() == 0 ){
		Wire.requestFrom(device,count); // read
		while( Wire.available() ){
		  *buffer++ =  Wire.read();
		  //Serial.print("+");Serial.print(got);
		  got++;
		}
		//Serial.println();Serial.print("lsm6:");Serial.println(got);
		return got;
	}else{
	  Serial.println("I2C Read error");
	  Wire.endTransmission();
	  return 0;
	}
}
