#include "HAL/CanTalonSRX.h"
#include <iostream>
#include "ros/ros.h"
#include <vector>
#include "FRC_NetworkCommunication/CANSessionMux.h"
#include <memory>

#include <sensor_msgs/Joy.h>
#include "can_talon_srx/CANSend.h"
#include "can_talon_srx/CANRecv.h"
#include "can_talon_srx_msgs/control.h"
//#include "std_msgs/String.h"

//#include "std_msgs/String.h"

CanTalonSRX* motor;

void joyCallback(const sensor_msgs::Joy::ConstPtr& joy) {
	//std::cout << "joy: " << joy->axes[1] << std::endl;
	motor->Set(joy->axes[1]);
}

int main(int argc, char **argv) {
	ros::init(argc, argv, "talontest");
	ros::NodeHandle n;

	ros::Publisher CANSend_pub = n.advertise<can_talon_srx::CANSend>("CANSend",100);
	ros::ServiceClient CANRecv_cli = n.serviceClient<can_talon_srx::CANRecv>("CANRecv");

	ros::Subscriber joy_sub = n.subscribe("joy", 10, joyCallback);

	ros::Rate loop_rate(2); //in hertz

	init_CANSend(CANSend_pub, CANRecv_cli);
	motor = new CanTalonSRX(1);
	CanTalonSRX motor2 (2);

	double batteryV;
	double current;
	int throttle;
	int limitswitchfor;
	int sensorpos;
	int fsoftenable;
	while(ros::ok()) {
		
		//motor.Set(.3);
		//motor2.Set(.3);
		motor->GetBatteryV(batteryV);
		motor->GetAppliedThrottle(throttle);
		motor->GetLimitSwitchClosedFor(limitswitchfor);
		motor->GetAnalogInWithOv(sensorpos);
		//motor.SetForwardSoftEnable(1);
		//motor.GetForwardSoftEnable(fsoftenable);

		std::cout << "BatteryV\t" << batteryV << std::endl;
		//std::cout << "current\t" << current << std::endl;
		std::cout << "throttle\t" << throttle << std::endl;
		std::cout << "limitswitchfor\t" << limitswitchfor << std::endl;
		std::cout << "sensorpos\t" << sensorpos << std::endl;
		std::cout << "fsoftenable\t" << fsoftenable << std::endl;
		//motor.Set(.1);
		//ros::Duration(1).sleep();
		//motor.Set(.3);
		//ros::Duration(1).sleep();
		ros::spinOnce();
		//loop_rate.sleep();
	}
}
