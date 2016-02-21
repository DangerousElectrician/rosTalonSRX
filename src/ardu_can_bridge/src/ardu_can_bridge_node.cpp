#include "ros/ros.h"
#include "can_talon_srx/CANRecv.h"
#include "can_talon_srx/CANSend.h"
#include "can_talon_srx/CANData.h"
#include "crc8_table.h"
#include <vector>
#include <map>
#include <iterator>

//serial port stuff
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#define BAUD B115200
#define PORT "/dev/ttyACM0"

std::map<uint32_t, can_talon_srx::CANData> receivedCAN;

struct txCANData {
	can_talon_srx::CANData data;
	uint8_t checksum = 42;
	uint8_t index = 0;
	int32_t periodMs = -1;
};

std::map<uint32_t, txCANData> transmittingCAN;
char usedIndex[50] = {0};

/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int fd = 0;

int open_port(std::string dev) {
	int fd; /* File descriptor for the port */


	fd = open(dev.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		/*
		* Could not open the port.
		*/

		ROS_ERROR("open_port: Unable to open %s", dev.c_str());
	}
	else fcntl(fd, F_SETFL, 0);

	struct termios options;

	tcgetattr(fd, &options); //Get the current options for the port...

	cfsetispeed(&options, BAUD); //Set the baud rates
	cfsetospeed(&options, BAUD);

	//options.c_cflag |= (CLOCAL | CREAD); //Enable the receiver and set local mode...
	//options.c_iflag &= ~(IXON | IXOFF | IXANY); //disable software flow control
	//options.c_oflag &= ~OPOST;
	//options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //enable raw input

	//cfmakeraw(&options);

	// 8N1
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	// no flow control
	options.c_cflag &= ~CRTSCTS;

	//toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

	options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	options.c_oflag &= ~OPOST; // make raw

	tcsetattr(fd, TCSANOW, &options); //Set the new options for the port...

	return (fd);
}

ssize_t serialread(int fd, void *buf, size_t count, long int timeoutus) {
        // Initialize file descriptor sets
        fd_set read_fds, write_fds, except_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        FD_SET(fd, &read_fds);

        // Set timeout to 1.0 seconds
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = timeoutus;

        // Wait for input to become ready or until the time out; the first parameter is
        // 1 more than the largest file descriptor in any of the sets
        if (select(fd + 1, &read_fds, &write_fds, &except_fds, &timeout) == 1) {
                // fd is ready for reading
                return read(fd, buf, count);
        }
        else {
                // timeout or error
                //std::cout << "timeout " << std::flush;
                return -1;
        }
}

bool recvCAN(can_talon_srx::CANRecv::Request &req, can_talon_srx::CANRecv::Response &res) {
	//ROS_INFO("request arbID: %ld", (long int)req.arbID);

	std::map<uint32_t, can_talon_srx::CANData>::iterator i = receivedCAN.find(req.arbID);
	if(i == receivedCAN.end()) {
		//no message with requested arbID

		can_talon_srx::CANData data;
		res.status = 1; //status is 1 if there is no CAN frame with requested arbID

	} else {
		res.data = receivedCAN[req.arbID];
	}
	return true;
}

int lockIndex() {
	for(int i = 0; i < (int) sizeof(usedIndex); i++) { //ycm says that comparing an int and an unsigned long is bad
		if(usedIndex[i]) {
			
		} else {
			usedIndex[i] = 1;
			return i;
		}
	}
	return -1;
}

void releaseIndex(int index) {
	usedIndex[index] = 0;
}

struct TXData {
	unsigned char size = 0;
	unsigned char index = 0;
	long periodMs = -1;
	unsigned long arbID;
	unsigned char bytes[8];
	unsigned char checksum = 42;
};

void CANSendCallback(const can_talon_srx::CANSend::ConstPtr& msg) {
	txCANData txdata;
	txdata.data = msg->data;
	txdata.periodMs = msg->periodMs;
	
	//ROS_INFO("send arbID: %ld", (long int) txdata.data.arbID);

	std::map<uint32_t, txCANData>::iterator i = transmittingCAN.find(txdata.data.arbID);
	if( i == transmittingCAN.end() ) {
		int tmpindex = lockIndex();
		if(tmpindex == -1) {
			ROS_ERROR("Cannot transmit periodic CAN message");
			return ;
		}
		txdata.index  = tmpindex;
		transmittingCAN[txdata.data.arbID] = txdata;
	} else {
		txCANData oldData = transmittingCAN[txdata.data.arbID];
		txdata.index = oldData.index;
		if(txdata.periodMs > 0) {
			transmittingCAN[txdata.data.arbID] = txdata;
		} else {
			releaseIndex(txdata.index);
			transmittingCAN.erase(txdata.data.arbID);
		}
	}

	//ROS_INFO("send size: %ld index: %ld", (long int) txdata.data.size, (long int)txdata.index);
	write(fd, &txdata.data.size, 1);
	write(fd, &txdata.index, 1);
	write(fd, &txdata.periodMs, 4);
	write(fd, &txdata.data.arbID, 4); 
	write(fd, &txdata.data.bytes[0], 8);
	write(fd, &txdata.checksum, 1);	
}

struct RXData {
	//unsigned char size;
	uint8_t size;
	//unsigned char packetcount;
	uint8_t packetcount;
	unsigned long arbID:32; //needs to be 32 bits
	//uint32_t arbID:32; //this doesn't work
	unsigned char bytes[8];
	unsigned char checksum;
};

int main(int argc, char **argv) {


	ros::init(argc, argv, "ardu_can_bridge");
	ros::NodeHandle n;
	ros::NodeHandle nh("~");

	std::string device = "/dev/ttyACM0";
	nh.getParam("port", device);
	fd = open_port(device);

	ros::Subscriber sub = n.subscribe("CANSend", 100, CANSendCallback);

	ros::ServiceServer service = n.advertiseService("CANRecv", recvCAN);

	ros::Rate r(1000);

	ROS_INFO("Waiting for arduino bootloader to finish");
	ros::Duration(2).sleep(); //THIS DELAY IS IMPORTANT the arduino bootloader has a tendency to obliterate its memory

	ROS_INFO("Starting communications");
	while(ros::ok()) {
		RXData rxData;
		write(fd, "d", 1);
		if(serialread(fd, &rxData, 15, 100) != -1) {
			if(rxData.checksum == crc_update(0, &rxData.packetcount+1, 12) ) { //can't get address of bitfield

				std::cout << "size:" << unsigned(rxData.size) << " pcktcnt:" << unsigned(rxData.packetcount) << "\tchksum:" << unsigned(rxData.checksum) << "\tarbID:"<< unsigned(rxData.arbID) << "\tbytes:";
				for(int j = 0; j < rxData.size; j++) {
					std::cout << unsigned(rxData.bytes[j]) << " ";
				}
				std::cout << std::endl;

				can_talon_srx::CANData data;
				data.arbID = rxData.arbID;
				data.size = rxData.size;
				data.bytes = std::vector<uint8_t> (rxData.bytes, rxData.bytes + rxData.size);

				receivedCAN[data.arbID] = data;

			} else {std::cout << "cksm err " << unsigned(rxData.checksum) << std::endl;}
		} //else {std::cout << "timeout " << std::flush;}
		r.sleep();  //sending stuff over serial too quickly is bad. figure out a better way for flow control
		ros::spinOnce();
	}

	return 0;
}
