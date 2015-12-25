#include "HAL/CanTalonSRX.h"
#include <iostream>
#include "ros/ros.h"
#include <vector>

#include "can_talon_ros/CANSend.h"
//#include "std_msgs/String.h"

//#include "std_msgs/String.h"

int main(int argc, char **argv) {
	ros::init(argc, argv, "talontest");
	ros::NodeHandle n;
	
	ros::Publisher CANSend_pub = n.advertise<can_talon_ros::CANSend>("CANSend",100);

	ros::Rate loop_rate(2); //in hertz

	while(ros::ok()) {
		
		can_talon_ros::CANSend msg;

		msg.arbID = 32;
		msg.size = 255;
		std::vector<uint8_t> candata = {1,2,3,4,5,6,7,8};
		//std::cout<<candata.size()<<std::endl;
		msg.data = candata;
		
		CANSend_pub.publish(msg);

		//CanTalonSRX motor (0);
		
		//std::cout << "setprofile" << std::endl;
		//motor.SetProfileSlotSelect(1);	
		//std::cout << "setmode" << std::endl;
		//motor.SetModeSelect(4, 133);

		//std::cout << "set1" << std::endl;
		//motor.Set(.1);

		//std::cout << "set2" << std::endl;
		//motor.Set(1);

		//std::cout << "done" << std::endl;

		ros::spinOnce();
		loop_rate.sleep();
	}
}