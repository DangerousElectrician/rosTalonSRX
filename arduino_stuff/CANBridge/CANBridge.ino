
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
        if(txarr[j].periodMs >= 0)
        {
          CAN.sendMsgBuf(txarr[j].data.canId, 1, txarr[j].data.size, txarr[j].data.bytes);
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
        case 8:
          txData.data.size = command;
          if(txData.data.size > 8) break;
          
          while(Serial.available() < 2);
          txData.checksum = Serial.read();
          if(txData.checksum != 42) break;
          
          txData.index = Serial.read();
          
          while(Serial.available() < 4);        
          Serial.readBytes(bytecon2, 4);
          txData.periodMs = (unsigned long)bytecon2[0] | ((unsigned long)bytecon2[1] << 8) | ((unsigned long)bytecon2[2] << 16) | ((unsigned long)bytecon2[3] << 24);

          while(Serial.available() < 4);
          Serial.readBytes(bytecon2, 4);
          txData.data.canId = (unsigned long)bytecon2[0] | ((unsigned long)bytecon2[1] << 8) | ((unsigned long)bytecon2[2] << 16) | ((unsigned long)bytecon2[3] << 24);

          
          while(Serial.available() < txData.data.size);
          Serial.readBytes(bytecon2, txData.data.size);
          for(j = 0; j< txData.data.size; j++)  txData.data.bytes[j] = bytecon2[j];
          
          txarr[txData.index].data.size = txData.data.size;
          txarr[txData.index].periodMs = txData.periodMs;
          txarr[txData.index].data.canId = txData.data.canId;
          for(j = 0; j< txData.data.size; j++) txarr[txData.index].data.bytes[j] = txData.data.bytes[j];
          
          keepalive = 255;
          
          #ifdef DEBUG
            Serial.println();
            Serial.print("size:\t");
            Serial.println(txarr[txData.index].data.size);
            Serial.print("index:\t");
            Serial.println(txData.index);
            Serial.print("chksum:\t");
            Serial.println(txData.checksum);
            Serial.print("period:\t");
            Serial.println(txarr[txData.index].periodMs);
            Serial.print("arbID:\t");
            Serial.println(txarr[txData.index].data.canId);
            
            Serial.print("bytes:\t");
            for(j= 0 ; j< txarr[txData.index].data.size; j++)
            {
              Serial.print(txData.data.bytes[j]);
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
