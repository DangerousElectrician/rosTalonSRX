
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
  unsigned char size = 0;
  unsigned char bytes[8];
  INT32U canId;
};

struct RXData {
  CANData data;
  unsigned char checksum = 42;
  unsigned char packetcount = 0;
};

struct TXData {
  CANData data;
  unsigned char checksum = 42;
  unsigned char packetcount = 0;
  long periodMs = -1;
  unsigned char index = 0;
};
  
struct TXCANData {
  CANData data;
  long periodMs = -1;
};
TXCANData txarr[50];

RXData rxData;
TXData txData;

unsigned char bytecon[8];
char bytecon2[8];

unsigned char keepalive = 0;
unsigned char sendmessages = 0;
unsigned char command = 0;

int j = 0;

void loop()
{

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&rxData.data.size, rxData.data.bytes);  

        rxData.data.canId = CAN.getCanId();
        rxData.packetcount++;

        bytecon[0] = rxData.data.canId & BYTEMASK;
        bytecon[1] = (rxData.data.canId >> 8) & BYTEMASK;
        bytecon[2] = (rxData.data.canId >> 16) & BYTEMASK;
        bytecon[3] = (rxData.data.canId >> 24) & BYTEMASK;
          
        if(sendmessages)
        { 
          sendmessages--;  
          Serial.write(rxData.data.size);
          Serial.write(rxData.packetcount);
          Serial.write(rxData.checksum);
          Serial.write(bytecon, 4); //canID after shuffled into an array
          Serial.write(rxData.data.bytes, rxData.data.size);
        }
    }
    
    if(keepalive)
    {
      for(j = 0; j < 50; j++)
      {
        if(txarr[j].periodMs > 0)
        {
          CAN.sendMsgBuf(txarr[j].data.canId, 1, txarr[j].data.size, txarr[j].data.bytes);
        }
        else if(txarr[j].periodMs == 0)
        {
          CAN.sendMsgBuf(txarr[j].data.canId, 1, txarr[j].data.size, txarr[j].data.bytes);
          txarr[j].periodMs = -1;
        }
      }
      keepalive--;
    }
          
    if(Serial.available())
    {
      command = Serial.read();
      switch(command)
      {
        case 'd':
          sendmessages = 1;
          keepalive = 255;
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
          txData.data.size = command;
          if(txData.data.size > 8) break;
          
          txData.checksum = Serial.read();
          if(txData.checksum != 42) break;
          
          txData.index = Serial.read();
          
          Serial.readBytes(bytecon2, 4);
          txData.periodMs = (unsigned long)bytecon2[0] | ((unsigned long)bytecon2[1] >> 8) | ((unsigned long)bytecon2[2] >> 16) | ((unsigned long)bytecon2[3] >> 24);
          
          //Serial.readBytes(bytecon2, 4);
          //txData.data.canId = bytecon2[0];// | ((unsigned long)bytecon2[1] >> 8) | ((unsigned long)bytecon2[2] >> 16) | ((unsigned long)bytecon2[3] >> 24);
          txData.data.canId = Serial.read();
          Serial.read();
          Serial.read();
          Serial.read();
          
          Serial.readBytes(bytecon2, txData.data.size);
          for(j = 0; j< txData.data.size; j++)  txData.data.bytes[j] = bytecon2[j];
          
          txarr[0].data.size = txData.data.size;
          txarr[0].periodMs = txData.periodMs;
          txarr[0].data.canId = txData.data.canId;
          keepalive = 255;
          Serial.println();
          Serial.print("debug ");
          Serial.println(txarr[0].data.size);
          break;
          
        case 'r':
          Serial.println(txarr[0].data.size);
          break;
      }
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
