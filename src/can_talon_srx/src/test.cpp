#include "HAL/CanTalonSRX.h"
#include <iostream>
#include "ros/ros.h"
#include <vector>
#include "FRC_NetworkCommunication/CANSessionMux.h"

#include "can_talon_srx/CANSend.h"
#include "can_talon_srx/CANRecv.h"
//#include "std_msgs/String.h"

//#include "std_msgs/String.h"

int main(int argc, char **argv) {
	ros::init(argc, argv, "talontest");
	ros::NodeHandle n;

	ros::Publisher CANSend_pub = n.advertise<can_talon_srx::CANSend>("CANSend",100);
	ros::ServiceClient CANRecv_cli = n.serviceClient<can_talon_srx::CANRecv>("CANRecv");

	ros::Rate loop_rate(2); //in hertz

	init_CANSend(CANSend_pub, CANRecv_cli);
	CanTalonSRX motor (0);

	double batteryV;
	while(ros::ok()) {
		
		motor.GetBatteryV(batteryV);

		std::cout << "battV " << batteryV << std::endl;
		motor.Set(.1);

		motor.Set(1);
		
		ros::spinOnce();
		loop_rate.sleep();
	}
}
