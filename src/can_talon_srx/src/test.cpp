#include "HAL/CanTalonSRX.h"
#include <iostream>
#include "ros/ros.h"
#include <vector>
#include "FRC_NetworkCommunication/CANSessionMux.h"
#include <memory>

#include "can_talon_srx/CANSend.h"
#include "can_talon_srx/CANRecv.h"
//#include "can_talon_srx/Set.h"
#include "can_talon_srx/GetParameter.h"
#include "can_talon_srx/ConfigSetParameter.h"

#include "can_talon_srx_msgs/Set.h"
#include "can_talon_srx_msgs/Status.h"

CanTalonSRX* motor;

void setCallback(const can_talon_srx_msgs::Set::ConstPtr& control) {
	motor->Set(control->set);
}

//bool set(can_talon_srx::Set::Request &req, can_talon_srx::Set::Response &res) {
//	motor->Set(req.set);
//	return true;
//}

bool getParameter(can_talon_srx::GetParameter::Request &req, can_talon_srx::GetParameter::Response &res) {
	motor->RequestParam((CanTalonSRX::param_t)req.param);
	ros::Duration(0.04).sleep();
	motor->GetParamResponse((CanTalonSRX::param_t)req.param, res.value);
	return true;
}

bool configSetParameter(can_talon_srx::ConfigSetParameter::Request &req, can_talon_srx::ConfigSetParameter::Response &res) {
	motor->SetParam((CanTalonSRX::param_t) req.param, req.value);
	return true;
}

int main(int argc, char **argv) {
	ros::init(argc, argv, "talontest");
	ros::NodeHandle n;

	ros::Publisher CANSend_pub = n.advertise<can_talon_srx::CANSend>("CANSend",100);
	ros::ServiceClient CANRecv_cli = n.serviceClient<can_talon_srx::CANRecv>("CANRecv");
	ros::Publisher status_pub = n.advertise<can_talon_srx_msgs::Status>("status", 10);
	
	//ros::ServiceServer set_srv = n.advertiseService("set", set);
	ros::ServiceServer getParameter_srv = n.advertiseService("getParameter", getParameter);
	ros::ServiceServer configSetParameter_srv = n.advertiseService("configSetParameter", configSetParameter);

	ros::Subscriber set_sub = n.subscribe("set", 10, setCallback);

	ros::Rate loop_rate(30); //in hertz

	ros::AsyncSpinner spinner(1);
	spinner.start();

	init_CANSend(CANSend_pub, CANRecv_cli);
	motor = new CanTalonSRX(1);

	double dparam; //variable declaration needs to be outside or else segfault
	int param;
	while(ros::ok()) {
		can_talon_srx_msgs::Status status_msg;

		motor->GetFault_OverTemp(param);
		status_msg.Fault_OverTemp = param;
		motor->GetFault_UnderVoltage(param);
		status_msg.Fault_UnderVoltage = param;
		motor->GetFault_ForLim(param);
		status_msg.Fault_ForLim = param;
		motor->GetFault_RevLim(param);
		status_msg.Fault_RevLim = param;
		motor->GetFault_HardwareFailure(param);
		status_msg.Fault_HardwareFailure = param;
		motor->GetFault_ForSoftLim(param);
		status_msg.Fault_ForSoftLim = param;
		motor->GetFault_RevSoftLim(param);
		status_msg.Fault_RevSoftLim = param;
		motor->GetStckyFault_OverTemp(param);
		status_msg.StckyFault_OverTemp = param;
		motor->GetStckyFault_UnderVoltage(param);
		status_msg.StckyFault_UnderVoltage = param;
		motor->GetStckyFault_ForLim(param);
		status_msg.StckyFault_ForLim = param;
		motor->GetStckyFault_RevLim(param);
		status_msg.StckyFault_RevLim = param;
		motor->GetStckyFault_ForSoftLim(param);
		status_msg.StckyFault_ForSoftLim = param;
		motor->GetStckyFault_RevSoftLim(param);
		status_msg.StckyFault_RevSoftLim = param;
		motor->GetAppliedThrottle(param);
		status_msg.AppliedThrottle = param;
		motor->GetCloseLoopErr(param);
		status_msg.CloseLoopErr = param;
		motor->GetFeedbackDeviceSelect(param);
		status_msg.FeedbackDeviceSelect = param;
		motor->GetModeSelect(param);
		status_msg.ModeSelect = param;
		motor->GetLimitSwitchEn(param);
		status_msg.LimitSwitchEn = param;
		motor->GetLimitSwitchClosedFor(param);
		status_msg.LimitSwitchClosedFor = param;
		motor->GetLimitSwitchClosedRev(param);
		status_msg.LimitSwitchClosedRev = param;
		motor->GetSensorPosition(param);
		status_msg.SensorPosition = param;
		motor->GetSensorVelocity(param);
		status_msg.SensorVelocity = param;
		motor->GetCurrent(dparam);
		status_msg.Current = dparam;
		motor->GetBrakeIsEnabled(param);
		status_msg.BrakeIsEnabled = param;
		motor->GetEncPosition(param);
		status_msg.EncPosition = param;
		motor->GetEncVel(param);
		status_msg.EncVel = param;
		motor->GetEncIndexRiseEvents(param);
		status_msg.EncIndexRiseEvents = param;
		motor->GetQuadApin(param);
		status_msg.QuadApin = param;
		motor->GetQuadBpin(param);
		status_msg.QuadBpin = param;
		motor->GetQuadIdxpin(param);
		status_msg.QuadIdxpin = param;
		motor->GetAnalogInWithOv(param);
		status_msg.AnalogInWithOv = param;
		motor->GetAnalogInVel(param);
		status_msg.AnalogInVel = param;
		motor->GetTemp(dparam);
		status_msg.Temp = dparam;
		motor->GetBatteryV(dparam);
		status_msg.BatteryV = dparam;
		motor->GetResetCount(param);
		status_msg.ResetCount = param;
		motor->GetResetFlags(param);
		status_msg.ResetFlags = param;
		motor->GetFirmVers(param);
		status_msg.FirmVers = param;
		motor->GetPulseWidthPosition(param);
		status_msg.PulseWidthPosition = param;
		motor->GetPulseWidthVelocity(param);
		status_msg.PulseWidthVelocity = param;
		motor->GetPulseWidthRiseToFallUs(param);
		status_msg.PulseWidthRiseToFallUs = param;
		motor->GetPulseWidthRiseToRiseUs(param);
		status_msg.PulseWidthRiseToRiseUs = param;
		motor->IsPulseWidthSensorPresent(param);
		status_msg.IsPulseWidthSensorPresent = param;

		status_pub.publish(status_msg);
		//ros::spin();
		//ros::spinOnce();
		loop_rate.sleep();
	}
}
