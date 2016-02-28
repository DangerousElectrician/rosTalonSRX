#!/usr/bin/env python
import rospy
from can_talon_srx_msgs.msg import control
from sensor_msgs.msg import Joy

def joyCallback(joy):
	print(joy.axes[1])
	msg = control()
	msg.set = joy.axes[1]
	pub.publish(msg)


def can_talon_srx_test():
	rospy.init_node('can_talon_srx_test')
	rospy.Subscriber("joy", Joy, joyCallback)
	global pub
	pub = rospy.Publisher('control', control)
	rospy.spin()

if __name__ == '__main__':
	try:
		can_talon_srx_test()
	except rospy.ROSInterruptException:
		pass
