
#include "FRC_NetworkCommunication/CANSessionMux.h"
#include "can_talon_srx/CANSend.h" //don't put this in the header file

#include <iostream>
#include <vector>

#ifdef __cplusplus
extern "C"
{
#endif  
	ros::Publisher *CANSend_pub;
        
	void init_CANSend(ros::Publisher CANSend_ros_pub) {
		CANSend_pub = &CANSend_ros_pub;
	}

	void FRC_NetworkCommunication_CANSessionMux_sendMessage(uint32_t messageID, const uint8_t *data, uint8_t dataSize, int32_t periodMs, int32_t *status) {
		can_talon_srx::CANSend msg;

		msg.data.arbID = messageID;
		msg.data.size = dataSize;
		std::vector<uint8_t> candata(data, data+dataSize);
		msg.data.bytes = candata;
		msg.periodMs = periodMs;
		CANSend_pub->publish(msg);

		std::cout << "sendCAN " << messageID << "\t";
		for(int i = 0; i < dataSize; i++) {
			std::cout << unsigned(data[i]) << "\t";
		}
		std::cout << "datsize:" << unsigned(dataSize) << "\tper:" << periodMs << std::endl;
	}

	void FRC_NetworkCommunication_CANSessionMux_receiveMessage(uint32_t *messageID, uint32_t messageIDMask, uint8_t *data, uint8_t *dataSize, uint32_t *timeStamp, int32_t *status) {
		std::cout << "recvCAN " << *messageID << std::endl;
		data[0] = 0;
		data[1] = 0;
		data[2] = 91;
		data[3] = 0;
		data[4] = 0;
		data[5] = 105;
		data[6] = 162;
		data[7] = 0;
	}

	void FRC_NetworkCommunication_CANSessionMux_openStreamSession(uint32_t *sessionHandle, uint32_t messageID, uint32_t messageIDMask, uint32_t maxMessages, int32_t *status) {
		std::cout << "openStreamCAN" << std::endl;
	}

	void FRC_NetworkCommunication_CANSessionMux_closeStreamSession(uint32_t sessionHandle) {
		std::cout << "closeStreamCAN" << std::endl;
	}

	void FRC_NetworkCommunication_CANSessionMux_readStreamSession(uint32_t sessionHandle, struct tCANStreamMessage *messages, uint32_t messagesToRead, uint32_t *messagesRead, int32_t *status) {
		std::cout << "readStreamCAN" << std::endl;
	}

	//void FRC_NetworkCommunication_CANSessionMux_getCANStatus(float *percentBusUtilization, uint32_t *busOffCount, uint32_t *txFullCount, uint32_t *receiveErrorCount, uint32_t *transmitErrorCount, int32_t *status);

#ifdef __cplusplus
}
#endif
