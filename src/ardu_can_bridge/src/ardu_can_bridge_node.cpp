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
#include <sys/ioctl.h>

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

	options.c_cc[VMIN] = 100;
	options.c_cc[VTIME] = 2;

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

void flushSerial(int fd) {
	unsigned char buf [100];
	int n;
	int bytes_avail;
	ioctl(fd, TIOCINQ, &bytes_avail);
	//std::cout << "flushed "; 
	while(bytes_avail > 0) {
		//std::cout << "flush" << std::endl;
		n = serialread (fd, buf, bytes_avail, 0);
		ioctl(fd, TIOCINQ, &bytes_avail);
		//std::cout << unsigned(buf[0]) << "\t" << std::flush;
	}
	//std::cout << std::endl;
}


int main(int argc, char **argv) {


	ros::init(argc, argv, "ardu_can_bridge");
	ros::NodeHandle n;
	ros::NodeHandle nh("~");

	std::string device = "/dev/ttyACM0";
	nh.getParam("port", device);
	fd = open_port(device);

	ros::Subscriber sub = n.subscribe("CANSend", 100, CANSendCallback);

	ros::ServiceServer service = n.advertiseService("CANRecv", recvCAN);

	ros::AsyncSpinner spinner(2);
	spinner.start();

	ros::Rate r(500);

	ROS_INFO("Waiting for arduino bootloader to finish");
	ros::Duration(2).sleep(); //THIS DELAY IS IMPORTANT the arduino bootloader has a tendency to obliterate its memory

	ROS_INFO("Starting communications");

	int missedPackets = 0; //for gathering stats
	int goodPackets = 0;
	int prevPacketCount = 0;

	char datagood = 1;
	char skip = 0;
	//write(fd, "q" ,1);
	while(ros::ok()) {
		RXData rxData;

		//datagood = 1;
		if(datagood) write(fd, "d", 1);
		else { 
		}

		//if(!datagood) {
		//	write(fd, "w", 1); //stop stream and ask for repeat
		//	write(fd, "r", 1);
		//}

		//unsigned char header=255;
		if(true) { //serialread(fd, &header, 1, 100) != 0) {
			if(!skip) {
				if(serialread(fd, &rxData.size, 15, 800) != -1) {
					//rxData.size = header;
					if(rxData.size <= 8 &&rxData.checksum == crc_update(0, &rxData.packetcount+1, 12) ) { //can't get address of bitfield

						std::cout << "size:" << unsigned(rxData.size) << " pcktcnt:" << unsigned(rxData.packetcount) << "\tchksum:" << unsigned(rxData.checksum) << "\tarbID:"<< unsigned(rxData.arbID) << "\tbytes:";
						for(int j = 0; j < rxData.size; j++) {
							std::cout << unsigned(rxData.bytes[j]) << " ";
						}
						std::cout << std::endl << std::flush;

						can_talon_srx::CANData data;
						data.arbID = rxData.arbID;
						data.size = rxData.size;
						data.bytes = std::vector<uint8_t> (rxData.bytes, rxData.bytes + rxData.size);

						receivedCAN[data.arbID] = data;
						if(!datagood) {
							datagood = 1;
							//write(fd, "q", 1); //restart stream
						}

						//for(int j = 0; j < 15; j++) {
						//	std::cout << unsigned(*((unsigned char*)(&rxData)+j)) << "\t" << std::flush;
						//}
						//std::cout << std::endl<< std::flush;

						if(goodPackets != 0 && rxData.packetcount - 1 != prevPacketCount && rxData.packetcount != prevPacketCount) {
							int tmppacketcount = rxData.packetcount - prevPacketCount;
							if(tmppacketcount > 0) {
								missedPackets += (unsigned char)(tmppacketcount - 1);
							}
							//std::cout<< "prevPacketCount: " << unsigned(prevPacketCount) << std::endl;
							//std::cout << "missed: " << unsigned((unsigned char)(rxData.packetcount - prevPacketCount -1 )) << std::endl;
						}
						prevPacketCount = rxData.packetcount;
						goodPackets++;

					} else {
						for(int j = 0; j < 15; j++) {
							std::cout << unsigned(*((unsigned char*)(&rxData)+j)) << "\t" << std::flush;
						}
						std::cout << "cksm err " << unsigned(crc_update(0, &rxData.packetcount+1, 12));// << unsigned(rxData.size) << std::endl << std::flush;
						std::cout << std::endl<< std::flush;
						datagood = 0;
						//skip = 1;
						//flushSerial(fd);
			flushSerial(fd);
			write(fd, "r", 1);
					}
				} //else flushSerial(fd);
			} else skip = 0;
		}
		//int bytes_avail;
		//ioctl(fd, TIOCINQ, &bytes_avail);
		//std::cout << "bytes_avail " << bytes_avail << std::endl;
		//r.sleep();  //sending stuff over serial too quickly is bad. figure out a better way for flow control
		//ros::spinOnce();
	}
	write(fd, "w", 1);

	std::cout << std::endl << "goodPackets:\t" << goodPackets << std::endl;
	std::cout << "missedPackets:\t" << missedPackets << std::endl << std::endl;
	std::cout << "totalPackets:\t" << goodPackets+missedPackets << std::endl;
	std::cout << "percentGood:\t" << float(goodPackets)/(goodPackets+missedPackets) << std::endl;

	return 0;
}
