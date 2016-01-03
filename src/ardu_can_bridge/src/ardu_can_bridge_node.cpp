#include "ros/ros.h"
#include "can_talon_srx/CANRecv.h"
#include "can_talon_srx/CANSend.h"
#include "can_talon_srx/CANData.h"
#include <vector>
#include <map>

std::map<uint32_t, can_talon_srx::CANData> receivedCAN;

bool recvCAN(can_talon_srx::CANRecv::Request &req, can_talon_srx::CANRecv::Response &res) {
	ROS_INFO("request arbID: %ld", (long int)req.arbID);
	res.data.arbID = req.arbID;
	res.data.size = 8;
	std::vector<uint8_t> candata(8);
	candata = {0,0,91,0,0,105,162,0};
	res.data.bytes = candata;
	//res.data.bytes[0] = 0;
	//res.data.bytes[1] = 0;
	//res.data.bytes[2] = 91;
	//res.data.bytes[3] = 0;
	//res.data.bytes[4] = 0;
	//res.data.bytes[5] = 105;
	//res.data.bytes[6] = 162;
	//res.data.bytes[7] = 0;
	//ROS_INFO("sending back response: [%ld]", (long int)res.sum);
	return true;
}

void CANSendCallback(const can_talon_srx::CANSend::ConstPtr& msg) {
	ROS_INFO("send arbID: %ld", (long int) msg->data.arbID);
}

int main(int argc, char **argv) {
	ros::init(argc, argv, "ardu_can_bridge");
	ros::NodeHandle n;

	ros::Subscriber sub = n.subscribe("CANSend", 100, CANSendCallback);

	ros::ServiceServer service = n.advertiseService("CANRecv", recvCAN);
	ros::spin();

	return 0;
}
