#include "CanTalonSRX.h"
#include <iostream>
#include "ros/ros.h"

//#include "std_msgs/String.h"

int main(int argc, char **argv) {
	ros::init(argc, argv, "talontest");

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
