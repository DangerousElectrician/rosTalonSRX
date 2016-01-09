
#include <SPI.h>
#include "mcp_can.h"

#define BYTEMASK B11111111

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
const int LED=8;
boolean ledON=1;
MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

    unsigned char stmp0[8] = {0,0,0,0, 0,0,0,0};    
void setup()
{
    Serial.begin(115200);
    pinMode(LED,OUTPUT);

START_INIT:

    if(CAN_OK == CAN.begin(CAN_1000KBPS))                   // init can bus : baudrate = 500k
    {
        //Serial.println("CAN BUS Shield init ok!");
    }
    else
    {
        //Serial.println("CAN BUS Shield init fail");
        //Serial.println("Init CAN BUS Shield again");
        delay(100);
        goto START_INIT;
    }
    
    CAN.sendMsgBuf( (INT32U)0x2040000, 1, 8, stmp0); //some random activity to make the talon go into CAN mode
}

struct CANData {
  unsigned char len = 0;
  unsigned char buf[8];
  unsigned char checksum = 42;
  unsigned char packetcount = 0;
  INT32U canId;
};

struct CANData rxData;

unsigned char bytecon[8];

unsigned char keepalive = 0;
unsigned char command = 0;

void loop()
{

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&rxData.len, rxData.buf);    // read data,  len: data length, buf: data buf

        rxData.canId = CAN.getCanId();
        rxData.packetcount++;

        bytecon[0] = rxData.canId & BYTEMASK;
        bytecon[1] = (rxData.canId >> 8) & BYTEMASK;
        bytecon[2] = (rxData.canId >> 16) & BYTEMASK;
        bytecon[3] = (rxData.canId >> 24) & BYTEMASK;
          
        if(keepalive)
        { 
          keepalive--;  
          Serial.write(rxData.len);
          Serial.write(rxData.packetcount);
          Serial.write(rxData.checksum);
          Serial.write(bytecon, 4); //canID after shuffled into an array
          Serial.write(rxData.buf, rxData.len);
        }
    }
          
    if(Serial.available())
    {
      command = Serial.read();
      switch(command)
      {
        case 'd':
          keepalive = 1;
          break;

        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
          
          break;
      }
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
