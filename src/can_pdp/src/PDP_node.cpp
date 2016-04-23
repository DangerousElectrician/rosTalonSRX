#include "ctre/PDP.h"
#include <iostream>
#include "ros/ros.h"
#include <vector>
#include "FRC_NetworkCommunication/CANSessionMux.h"
#include <memory>

PDP* pdp;

int main(int argc, char **argv) {
	ros::init(argc, argv, "pdp");
	ros::NodeHandle n;

	init_CANSend(n);

	pdp = new PDP(0);

	double current;
	pdp->GetChannelCurrent(0, current);

	while(ros::ok()) {

	}
}
