
#include <SPI.h>
#include "mcp_can.h"
#include "crc8_table.h"

#define BYTEMASK B11111111

INT8U registers[] = {
MCP_RXF0SIDH,
MCP_RXF0SIDL,
MCP_RXF0EID8,
MCP_RXF0EID0,
MCP_RXF1SIDH,
MCP_RXF1SIDL,
MCP_RXF1EID8,
MCP_RXF1EID0,
MCP_RXF2SIDH,
MCP_RXF2SIDL,
MCP_RXF2EID8,
MCP_RXF2EID0,
MCP_CANSTAT,
MCP_CANCTRL,
MCP_RXF3SIDH,
MCP_RXF3SIDL,
MCP_RXF3EID8,
MCP_RXF3EID0,
MCP_RXF4SIDH,
MCP_RXF4SIDL,
MCP_RXF4EID8,
MCP_RXF4EID0,
MCP_RXF5SIDH,
MCP_RXF5SIDL,
MCP_RXF5EID8,
MCP_RXF5EID0,
MCP_TEC  ,
MCP_REC  ,
MCP_RXM0SIDH,
MCP_RXM0SIDL,
MCP_RXM0EID8,
MCP_RXM0EID0,
MCP_RXM1SIDH,
MCP_RXM1SIDL,
MCP_RXM1EID8,
MCP_RXM1EID0,
MCP_CNF3 ,
MCP_CNF2 ,
MCP_CNF1 ,
MCP_CANINTE,
MCP_CANINTF,
MCP_EFLG ,
MCP_TXB0CTRL,
MCP_TXB1CTRL,
MCP_TXB2CTRL,
MCP_RXB0CTRL,
MCP_RXB0SIDH,
MCP_RXB1CTRL,
MCP_RXB1SIDH
};

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
const int LED = 8;
boolean ledON = 1;
MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

unsigned char stmp0[8] = {0, 0, 0, 0, 0, 0, 0, 0};
void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

START_INIT:

  if (CAN_OK == CAN.begin(CAN_1000KBPS))                  // init can bus : baudrate = 500k
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

  CAN.init_Mask(0, 1, 0);
  CAN.init_Filt(0, 1, 0);

  //CAN.sendMsgBuf( (INT32U)0x2040000, 1, 8, stmp0); //some random activity to make the talon go into CAN mode
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

RXData RXDataBuffer[10];
unsigned char RXDataBufferWriteIndex = 0;
unsigned char RXDataBufferReadIndex = 0;
unsigned char prevRXDataBufferReadIndex = 0;

unsigned char keepalive = 0;
unsigned char sendmessages = 0;
unsigned char command = 0;

unsigned char stream = 0;

int j = 0;

void loop()
{
  if (CAN_MSGAVAIL == CAN.checkReceive())           // check if data coming
  {
    CAN.readMsgBuf(&rxData.size, rxData.bytes);

    rxData.canId = CAN.getCanId();
    rxData.checksum = crc_update(0, &rxData.canId, 12);
    rxData.packetcount++;

    RXDataBuffer[RXDataBufferWriteIndex].size = rxData.size;
    RXDataBuffer[RXDataBufferWriteIndex].packetcount = rxData.packetcount;
    RXDataBuffer[RXDataBufferWriteIndex].canId = rxData.canId;
    for (j = 0; j < 8; j++) RXDataBuffer[RXDataBufferWriteIndex].bytes[j] = rxData.bytes[j];
    RXDataBuffer[RXDataBufferWriteIndex].checksum = rxData.checksum;

    if (RXDataBufferWriteIndex < 9) RXDataBufferWriteIndex++;
    else RXDataBufferWriteIndex = 0;


  }

  if (keepalive)
  {
    for (j = 0; j < 50; j++)
    {
      if (txarr[j].periodMs >= 0)
      {
        CAN.sendMsgBuf(txarr[j].canId, 1, txarr[j].size, txarr[j].bytes);
        if (txarr[j].periodMs == 0) txarr[j].periodMs = -1;
      }
    }
    keepalive--;
  }
  else
  {
    for (j = 0; j < 50; j++)
    {
      txarr[j].periodMs = -1;
    }
  }

  if (stream && RXDataBufferReadIndex != RXDataBufferWriteIndex) //if indexes are the same, there are no unread messages in buffer
  {
    Serial.write((unsigned char*) &RXDataBuffer[RXDataBufferReadIndex], 15);
    prevRXDataBufferReadIndex = RXDataBufferReadIndex;
    if (RXDataBufferReadIndex < 9) RXDataBufferReadIndex++;
    else RXDataBufferReadIndex = 0;
  }

  if (Serial.available())
  {
    command = Serial.read();
    switch (command)
    {
      case 'd':
        if (RXDataBufferReadIndex != RXDataBufferWriteIndex) //if indexes are the same, there are no unread messages in buffer
        {
          Serial.write((unsigned char*) &RXDataBuffer[RXDataBufferReadIndex], 15);
          prevRXDataBufferReadIndex = RXDataBufferReadIndex;
          if (RXDataBufferReadIndex < 9) RXDataBufferReadIndex++;
          else RXDataBufferReadIndex = 0;
        }
        keepalive = 255;
        break;

      case 'r': //resend message
        if (RXDataBufferReadIndex < 1)
        {
          Serial.write((unsigned char*) &RXDataBuffer[9 + RXDataBufferReadIndex], 15);
        }
        else
        {
          Serial.write((unsigned char*) &RXDataBuffer[RXDataBufferReadIndex - 1], 15);
        }
        break;

      case 'q':
        stream = 1;
        break;

      case 'w':
        stream = 0;
        break;

      case 'g':
	CAN.mcp2515_setCANCTRL_Mode(MODE_CONFIG);
	delay(10);
	for(int i = 0; i < sizeof(registers); i++)
	{
		Serial.print(i);
		Serial.print("\t");
		Serial.print(CAN.mcp2515_readRegister(registers[i]),HEX);
		Serial.print("\t");
		Serial.println(CAN.mcp2515_readRegister(registers[i]),BIN);
	}
	CAN.mcp2515_setCANCTRL_Mode(MODE_NORMAL);
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
        while (Serial.available() < 18);
        Serial.readBytes((char*)&txData.index, 18);

        if (txData.size > 8) break;
        if (txData.checksum != 42) break;

        if (txData.periodMs == 0)
        {
          CAN.sendMsgBuf(txData.canId, 1, txData.size, txData.bytes);
        }
        else
        {
          txarr[txData.index].size = txData.size;
          txarr[txData.index].periodMs = txData.periodMs;
          txarr[txData.index].canId = txData.canId;
          for (j = 0; j < txData.size; j++) txarr[txData.index].bytes[j] = txData.bytes[j];
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
        for (j = 0 ; j < txarr[txData.index].size; j++)
        {
          Serial.print(txData.bytes[j]);
          Serial.print("\t");
        }
        Serial.println();
#endif

        break;

	case '?':
		Serial.println("CANBRIDGE");
		break;
    }
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
