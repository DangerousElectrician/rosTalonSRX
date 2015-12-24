#include "HAL/CanTalonSRX.h"
#include <iostream>
#include "ros/ros.h"

#include "can_talon_ros/CANSend.h"
//#include "std_msgs/String.h"

//#include "std_msgs/String.h"

int main(int argc, char **argv) {
	ros::init(argc, argv, "talontest");
	ros::NodeHandle n;
	
	ros::Publisher CANSend_pub = n.advertise<can_talon_ros::CANSend>("CANSend",100);
	//ros::Publisher chatter_pub = n.advertise<std_msgs::String>("chatter", 1000);

	ros::Rate loop_rate(100);

	while(ros::ok()) {

		CanTalonSRX motor (0);
		
		//std::cout << "setprofile" << std::endl;
		//motor.SetProfileSlotSelect(1);	
		std::cout << "setmode" << std::endl;
		motor.SetModeSelect(4, 133);

		std::cout << "set1" << std::endl;
		motor.Set(.1);

		std::cout << "set2" << std::endl;
		motor.Set(1);

		std::cout << "done" << std::endl;
	}
}
