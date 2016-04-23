#include "ctre/PDP.h"
#include <iostream>
#include "ros/ros.h"
#include <vector>
#include "FRC_NetworkCommunication/CANSessionMux.h"
#include <memory>

int main(int argc, char **argv) {
	ros::init(argc, argv, "pdp");
	ros::NodeHandle n;

	init_CANSend(n);

	pdp = new PDP(0);

	while(ros::ok()) {

	}
}
