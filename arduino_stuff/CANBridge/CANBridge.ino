
#include <SPI.h>
#include "mcp_can.h"
#include "crc8_table.h"

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

struct RXData {
  unsigned char size = 0;
  unsigned char packetcount = 0;
  INT32U canId;
  unsigned char bytes[8];
  unsigned char checksum = 42;
};

struct TXData {
  unsigned char size = 0;
  unsigned char index = 0;
  long periodMs = -1;
  INT32U canId;
  unsigned char bytes[8];
  unsigned char checksum = 42;
};
  
struct TXCANData {
  unsigned char size = 0;
  unsigned char bytes[8];
  INT32U canId;
  long periodMs = -1;
};
TXCANData txarr[50];

RXData rxData;
TXData txData;

char serInBuf[19];

unsigned char keepalive = 0;
unsigned char sendmessages = 0;
unsigned char command = 0;

int j = 0;

void loop()
{

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&rxData.size, rxData.bytes);  

        rxData.canId = CAN.getCanId();
        rxData.checksum = crc_update(0, &rxData.canId, 9);
        rxData.packetcount++;

        if(sendmessages)
        { 
          sendmessages--;
          Serial.write((unsigned char*)&rxData, 15);
        }
    }
    
    if(keepalive)
    {
      for(j = 0; j < 50; j++)
      {
        if(txarr[j].periodMs >= 0)
        {
          CAN.sendMsgBuf(txarr[j].canId, 1, txarr[j].size, txarr[j].bytes);
          if(txarr[j].periodMs == 0) txarr[j].periodMs = -1;
        }
      }
      keepalive--;
    }
    else
    {
      for(j = 0; j < 50; j++)
      {
        txarr[j].periodMs = -1;
      }
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
        case 8: // 1:size 1:index 4:period 4:arbID 8:data 1:checksum  19 total
          txData.size = command;
          while(Serial.available() < 18);
          Serial.readBytes((char*)&txData.index, 18);
          
          if(txData.size > 8) break;
          if(txData.checksum != 42) break;
          
          if(txData.periodMs == 0) 
          {
            CAN.sendMsgBuf(txData.canId, 1, txData.size, txData.bytes);
          }
          else 
          {
            txarr[txData.index].size = txData.size;
            txarr[txData.index].periodMs = txData.periodMs;
            txarr[txData.index].canId = txData.canId;
            for(j = 0; j< txData.size; j++) txarr[txData.index].bytes[j] = txData.bytes[j];
          }
          
          keepalive = 255;
          
          #ifdef DEBUG
            Serial.println();
            Serial.print("size:\t");
            Serial.println(txarr[txData.index].size);
            Serial.print("index:\t");
            Serial.println(txData.index);
            Serial.print("chksum:\t");
            Serial.println(txData.checksum);
            Serial.print("period:\t");
            Serial.println(txarr[txData.index].periodMs);
            Serial.print("arbID:\t");
            Serial.println(txarr[txData.index].canId);
            
            Serial.print("bytes:\t");
            for(j= 0 ; j< txarr[txData.index].size; j++)
            {
              Serial.print(txData.bytes[j]);
              Serial.print("\t");
            }
            Serial.println();
          #endif
          
          break;
      }
    }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
