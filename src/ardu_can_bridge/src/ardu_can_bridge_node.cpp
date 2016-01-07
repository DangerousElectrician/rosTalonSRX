#include "ros/ros.h"
#include "can_talon_srx/CANRecv.h"
#include "can_talon_srx/CANSend.h"
#include "can_talon_srx/CANData.h"
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
#define PORT "/dev/ttyS0"

std::map<uint32_t, can_talon_srx::CANData> receivedCAN;


/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */

int fd = 0;

int open_port(void) {
	int fd; /* File descriptor for the port */


	fd = open(PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		/*
		* Could not open the port.
		*/

		perror("open_port: Unable to open /dev/ttyS0 - ");
	}
	else fcntl(fd, F_SETFL, 0);

	struct termios options;

	tcgetattr(fd, &options); //Get the current options for the port...

	cfsetispeed(&options, BAUD); //Set the baud rates
	cfsetospeed(&options, BAUD);

	options.c_cflag |= (CLOCAL | CREAD); //Enable the receiver and set local mode...
	//options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

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
                std::cout << "timeout " << std::flush;
                return -1;
        }
}

bool recvCAN(can_talon_srx::CANRecv::Request &req, can_talon_srx::CANRecv::Response &res) {
	ROS_INFO("request arbID: %ld", (long int)req.arbID);

	std::map<uint32_t, can_talon_srx::CANData>::iterator i = receivedCAN.find(req.arbID);
	if(i == receivedCAN.end()) {
		//no message with requested arbID

		can_talon_srx::CANData data;
		res.status = 1; //status is 1 if there is no CAN frame with requested arbID

		//below is testing data
		data.arbID = req.arbID;
		data.size = 8;
		std::vector<uint8_t> candata{0,0,91,0,0,105,162,0};
		data.bytes = candata;
		receivedCAN[req.arbID] = data;
	} else {
		res.data = receivedCAN[req.arbID];
	}
	return true;
}

void CANSendCallback(const can_talon_srx::CANSend::ConstPtr& msg) {
	ROS_INFO("send arbID: %ld", (long int) msg->data.arbID);
	write(fd, &msg->data.size, 1);
	write(fd, "*", 1);	//checksum placeholder
	write(fd, &msg->periodMs, 4);
	write(fd, &msg->data.arbID, 4); 
	write(fd, &msg->data.bytes[0], msg->data.size);
}

#define DATTIME 0

int main(int argc, char **argv) {
	
	fd = open_port();
	uint8_t serbuf[14];

	ros::init(argc, argv, "ardu_can_bridge");
	ros::NodeHandle n;

	ros::Subscriber sub = n.subscribe("CANSend", 100, CANSendCallback);

	ros::ServiceServer service = n.advertiseService("CANRecv", recvCAN);

	unsigned int buf;
        unsigned char size;
        unsigned char packetcount;
        unsigned char checksum;
        unsigned int arbID;
        unsigned char bytes[8];

	while(ros::ok()) {
		write(fd, " ", 1);
                if(serialread(fd, &size, 1, 15000) != -1 && size <= 8) {
                        if(serialread(fd, &packetcount, 1, DATTIME) != -1) {
                                if(serialread(fd, &checksum, 1, DATTIME) != -1 && checksum == 42) {
                                        if(serialread(fd, &arbID, 4, DATTIME) != -1 && arbID < 536870912) {
                                                if(serialread(fd, &bytes, size, DATTIME) != -1 ) {
                                                        std::cout << "size:" << unsigned(size) << " pcktcnt:" << unsigned(packetcount) << "\tchksum:" << unsigned(checksum) << "\tarbID:"<< unsigned(arbID) << "\tbytes:";
                                                        for(int j = 0; j < size; j++) {
                                                                std::cout << unsigned(bytes[j]) << " ";
                                                        }
                                                        std::cout << std::endl;
                                                } else {std::cout << "byte err " << std::endl;}
                                        } else {std::cout << "arID err " << unsigned(arbID) << std::endl;}
                                } else {std::cout << "cksm err " << unsigned(checksum) << std::endl;}
                        } else {std::cout << "pcnt err " << std::endl;}
                } else {std::cout << "size err " << unsigned(size) << std::endl;}

		ros::spinOnce();
	}

	return 0;
}
