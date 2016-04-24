#!/bin/bash
export ROS_HOSTNAME=localhost
export ROS_MASTER_URI=http://localhost:11311

roslaunch rosebot_gazebo simulation.launch robot:=true
