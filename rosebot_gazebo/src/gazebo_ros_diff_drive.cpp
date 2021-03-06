/*
    Copyright (c) 2010, Daniel Hewlett, Antons Rebguns
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY Antons Rebguns <email> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Antons Rebguns <email> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * \file  gazebo_ros_diff_drive.cpp
 *
 * \brief A differential drive plugin for gazebo. Based on the diffdrive plugin
 * developed for the erratic robot (see copyright notice above). The original
 * plugin can be found in the ROS package gazebo_erratic_plugins.
 *
 * \author  Piyush Khandelwal (piyushk@gmail.com)
 *
 * $ Id: 06/21/2013 11:23:40 AM piyushk $
 */


/*
 *
 * The support of acceleration limit was added by
 * \author   George Todoran <todorangrg@gmail.com>
 * \author   Markus Bader <markus.bader@tuwien.ac.at>
 * \date 22th of May 2014
 */

#include <algorithm>
#include <assert.h>

#include "gazebo_ros_diff_drive.h"

#include <gazebo/math/gzmath.hh>
#include <sdf/sdf.hh>

#include <ros/ros.h>

namespace gazebo
{

enum {
    RIGHT,
    LEFT,
};

// Load the controller
void GazeboRosDiffDrive::Load ( physics::ModelPtr model, sdf::ElementPtr _sdf )
{    
	// Make sure the ROS node for Gazebo has already been initalized
	if (!ros::isInitialized())
	{
	   ROS_FATAL_STREAM("A ROS node for Gazebo has not been initialized, unable to load plugin. "
	   << "Load the Gazebo system plugin 'libgazebo_ros_api_plugin.so' in the gazebo_ros package)");
	   return;
	}

	this->parent = model;
    gazebo_ros_ = GazeboRosPtr ( new GazeboRos ( model, _sdf, "GazeboRosDiffDrive" ) );

    this->robot_namespace_ = gazebo_ros_->getNameSpace();

    //gazebo_ros_->getParameter<std::string> ( command_topic_, "commandTopic", "cmd_vel" );
    //gazebo_ros_->getParameter<std::string> ( odometry_topic_, "odometryTopic", "odom" );

    ros::param::param<std::string> ( this->robot_namespace_ + "canal_cmd", command_topic_,  "cmd_vel");
    ros::param::param<std::string> ( this->robot_namespace_ + "canal_odom", odometry_topic_,  "odom");

    gazebo_ros_->getParameter<std::string> ( odometry_frame_, "odometryFrame", odometry_topic_ );
    gazebo_ros_->getParameter<std::string> ( robot_base_frame_, "robotBaseFrame", "base_footprint" );
    gazebo_ros_->getParameterBoolean ( publishWheelTF_, "publishWheelTF", false );
    gazebo_ros_->getParameterBoolean ( publishWheelJointState_, "publishWheelJointState", false );

    //gazebo_ros_->getParameter<double> ( wheel_separation_, "wheelSeparation", 0.34 );
    ros::param::param<double> (this->robot_namespace_ + "wheelRadius", wheel_radius_, 0.035 );
	ROS_INFO("Read wheel radius %lf",wheel_radius_);

    //gazebo_ros_->getParameter<double> ( wheel_accel_, "wheelAcceleration", 0.0 );
    ros::param::param<double> (this->robot_namespace_ + "wheelAcceleration", wheel_accel_, 0. );

	wheel_accel_ /= wheel_radius_;// from m/s to rad/s
	ROS_INFO("Calc'ed Wheel accel %lf",wheel_accel_);
	//gazebo_ros_->getParameter<double> ( wheel_torque, "wheelTorque", 5.0 );
    ros::param::param<double> ( this->robot_namespace_ + "wheelTorque", wheel_torque,  5. );
	ROS_INFO((this->robot_namespace_ + " Wheel Torque %lf").c_str(),wheel_torque);

    ros::param::param<double> (this->robot_namespace_ + "updateRateOdom", update_rate_odom_, 5.0 );
    ros::param::param<double> (this->robot_namespace_ + "updateRateCmdVel", update_rate_cmdvel_, 20.0 );
	ROS_INFO("Update rate odom    %lf",update_rate_odom_);
	ROS_INFO("Update rate cmd vel %lf",update_rate_cmdvel_);
	
    // Get the right and left wheels and compute the separation from their pose.
    physics::LinkPtr rightWheel = model->GetLink("right_wheel");
    physics::LinkPtr leftWheel  = model->GetLink("left_wheel");
    double wheel_sep = (rightWheel->GetWorldCoGPose().pos - leftWheel->GetWorldCoGPose().pos).GetLength();

    ROS_INFO("Calc'ed Wheel Separation %lf",wheel_sep);

    ros::param::param<double> ( this->robot_namespace_ + "wheelSeparation", wheel_separation_, .252 );
	ROS_INFO("Read wheel separation %lf", wheel_separation_);

	if((wheel_sep - wheel_separation_)/wheel_sep > .0001 )
		ROS_ERROR("wheel separation does not match the one calc'ed from sdf file: %lf vs %lf", wheel_separation_, wheel_sep);


    std::map<std::string, OdomSource> odomOptions;
    odomOptions["encoder"] = ENCODER;
    odomOptions["world"] = WORLD;
    gazebo_ros_->getParameter<OdomSource> ( odom_source_, "odometrySource", odomOptions, WORLD );


    joints_.resize ( 2 );
    joints_[LEFT]  = gazebo_ros_->getJoint ( parent, "leftJoint",  "left_joint" );
    joints_[RIGHT] = gazebo_ros_->getJoint ( parent, "rightJoint", "right_joint" );
    joints_[LEFT] ->SetParam ( "max_force", 0, wheel_torque );
    joints_[RIGHT]->SetParam ( "max_force", 0, wheel_torque );

    this->publish_tf_ = true;
    if (!_sdf->HasElement("publishTf")) {
      ROS_WARN("GazeboRosDiffDrive Plugin (ns = %s) missing <publishTf>, defaults to %d",
          this->robot_namespace_.c_str(), this->publish_tf_);
    } else {
      this->publish_tf_ = _sdf->GetElement("publishTf")->Get<bool>();
    }

    // Initialize update rate stuff
    if ( this->update_rate_odom_ > 0.0 )
    	this->update_period_odom_ = 1.0 / this->update_rate_odom_;
    else
    	this->update_period_odom_ = 0.0;

    if ( this->update_rate_cmdvel_ > 0.0 )
     	this->update_period_cmdvel_ = 1.0 / this->update_rate_cmdvel_;
     else
     	this->update_period_cmdvel_ = 0.0;

    last_update_time_cmdvel_ = last_update_time_odom_ = parent->GetWorld()->GetSimTime();

    // Initialize velocity stuff
    desired_wheel_speed_[RIGHT] = 0;
    desired_wheel_speed_[LEFT]  = 0;

    x_ 	   = 0;
    rot_   = 0;
    alive_ = true;

    if (this->publishWheelJointState_)
    {
        joint_state_publisher_ = gazebo_ros_->node()->advertise<sensor_msgs::JointState>("joint_states", 1000);
        ROS_INFO("%s: Advertise joint_states", gazebo_ros_->info());
    }

    transform_broadcaster_ = boost::shared_ptr<tf::TransformBroadcaster>(new tf::TransformBroadcaster());

    // ROS: Subscribe to the velocity command topic (usually "cmd_vel")
    ros::SubscribeOptions so =
        ros::SubscribeOptions::create<geometry_msgs::Twist>(command_topic_, 1,
                boost::bind(&GazeboRosDiffDrive::cmdVelCallback, this, _1),
                ros::VoidPtr(), &queue_);

    cmd_vel_subscriber_ = gazebo_ros_->node()->subscribe(so);
    ROS_INFO("%s: Subscribe to %s", gazebo_ros_->info(), command_topic_.c_str());

    if (this->publish_tf_)
    {
      odometry_publisher_ = gazebo_ros_->node()->advertise<nav_msgs::Odometry>(odometry_topic_, 1);
      ROS_INFO("%s: Advertise odom on %s", gazebo_ros_->info(), odometry_topic_.c_str());
    }

    // start custom queue for diff drive
    this->callback_queue_thread_ =
        boost::thread ( boost::bind ( &GazeboRosDiffDrive::QueueThread, this ) );

    // listen to the update event (broadcast every simulation iteration)
    this->update_connection_ =
        event::Events::ConnectWorldUpdateBegin ( boost::bind ( &GazeboRosDiffDrive::UpdateChild, this ) );

}

void GazeboRosDiffDrive::publishWheelJointState()
{
    ros::Time current_time = ros::Time::now();

    joint_state_.header.stamp = current_time;
    joint_state_.name.resize ( joints_.size() );
    joint_state_.position.resize ( joints_.size() );

    for ( int i = 0; i < 2; i++ ) {
        physics::JointPtr joint = joints_[i];
        math::Angle angle = joint->GetAngle ( 0 );
        joint_state_.name[i] = joint->GetName();
        joint_state_.position[i] = angle.Radian () ;
    }
    joint_state_publisher_.publish ( joint_state_ );
}

void GazeboRosDiffDrive::publishWheelTF()
{
    ros::Time current_time = ros::Time::now();
    for ( int i = 0; i < 2; i++ ) {
        
        std::string wheel_frame = gazebo_ros_->resolveTF(joints_[i]->GetChild()->GetName ());
        std::string wheel_parent_frame = gazebo_ros_->resolveTF(joints_[i]->GetParent()->GetName ());
        
        math::Pose poseWheel = joints_[i]->GetChild()->GetRelativePose();

        tf::Quaternion qt ( poseWheel.rot.x, poseWheel.rot.y, poseWheel.rot.z, poseWheel.rot.w );
        tf::Vector3 vt ( poseWheel.pos.x, poseWheel.pos.y, poseWheel.pos.z );

        tf::Transform tfWheel ( qt, vt );
        transform_broadcaster_->sendTransform (
            tf::StampedTransform ( tfWheel, current_time, wheel_parent_frame, wheel_frame ) );
    }
}

// Update the controller
void GazeboRosDiffDrive::UpdateChild()
{
  
    if ( odom_source_ == ENCODER ) 
		UpdateOdometryEncoder();

    common::Time current_time = parent->GetWorld()->GetSimTime();
    double seconds_since_last_update_odom = ( current_time - last_update_time_odom_ ).Double();

	if ( seconds_since_last_update_odom > update_period_odom_ )
	{
        if ( publish_tf_) publishOdometry ( seconds_since_last_update_odom );
        if ( publishWheelTF_ ) publishWheelTF();
        if ( publishWheelJointState_ ) publishWheelJointState();

        last_update_time_odom_ += common::Time ( update_period_odom_ );
	}

	double seconds_since_last_update_cmdvel = ( current_time - last_update_time_cmdvel_ ).Double();

	if ( seconds_since_last_update_cmdvel > update_period_cmdvel_ )
	{

        //ROS_INFO("UpdateChild v %lf ", x_);

        //ROS_INFO("seconds_since_last_update %lf s", seconds_since_last_update);

		// Update robot in case new velocities have been requested
        getWheelVelocities();

        boost::mutex::scoped_lock scoped_lock ( lock );

        double current_speed[2];

        current_speed[LEFT]  = joints_[LEFT]->GetVelocity ( 0 );
        current_speed[RIGHT] = joints_[RIGHT]->GetVelocity ( 0 );

		for(int leftRight=0; leftRight<=1; leftRight++)
		{

			if ( wheel_accel_ == 0
			//	||	( desired_wheel_speed_[LEFT]  - current_speed[LEFT] ) < 0.01 ) ||
			//        ( fabs( desired_wheel_speed_[RIGHT] - current_speed[RIGHT]) < 0.01 )
				)
			{
				//if max_accel == 0, or target speed is reached
				wheel_speed_instr_[leftRight] = desired_wheel_speed_[leftRight];
				//ROS_INFO("actual wheel speed = issued wheel speed= %lf", current_speed[LEFT]);
				//ROS_INFO("actual wheel speed = issued wheel speed= %lf", current_speed[RIGHT]);

			}
			else
			{
				double dV = desired_wheel_speed_[leftRight]-current_speed[leftRight];
				const double dV_max = wheel_accel_ * seconds_since_last_update_cmdvel;

				if( fabs(dV) > dV_max)
				{
					ROS_INFO("excess wheel accel %lf / %lf rad/s", dV, dV_max);
					if ( dV > 0 )
						dV = dV_max;
					else
						dV = -dV_max;
				}

				wheel_speed_instr_[leftRight] += dV;
				//ROS_INFO("actual wheel speed = %lf, issued wheel speed= %lf", current_speed[RIGHT],desired_wheel_speed_[RIGHT]);
			}

			joints_[leftRight]->SetVelocity ( 0, wheel_speed_instr_[leftRight] );
		}

		/*
		ROS_INFO("wheel speed = %lf / %lf, desired = %lf / %lf, new = %lf / %lf rad/s",
									current_speed[LEFT], current_speed[RIGHT],
								    desired_wheel_speed_[LEFT], desired_wheel_speed_[RIGHT],
									wheel_speed_instr_[LEFT], wheel_speed_instr_[RIGHT]);
		*/
        last_update_time_cmdvel_ += common::Time ( update_period_cmdvel_ );
    }
}

// Finalize the controller
void GazeboRosDiffDrive::FiniChild()
{
    alive_ = false;
    queue_.clear();
    queue_.disable();
    gazebo_ros_->node()->shutdown();
    callback_queue_thread_.join();
}

void GazeboRosDiffDrive::getWheelVelocities()
{
    boost::mutex::scoped_lock scoped_lock ( lock );

    double vr = x_;
    double va = rot_ * wheel_separation_ * .5;

    desired_wheel_speed_[LEFT]  = (vr - va) / wheel_radius_;
    desired_wheel_speed_[RIGHT] = (vr + va) / wheel_radius_;
}

void GazeboRosDiffDrive::cmdVelCallback ( const geometry_msgs::Twist::ConstPtr& cmd_msg )
{
    boost::mutex::scoped_lock scoped_lock ( lock );
    //ROS_INFO("cmdVelCallback v %lf ", cmd_msg->linear.x);
    x_   = cmd_msg->linear.x;
    rot_ = cmd_msg->angular.z;
}

void GazeboRosDiffDrive::QueueThread()
{
    static const double timeout = 0.01;

    while ( alive_ && gazebo_ros_->node()->ok() ) {
        queue_.callAvailable ( ros::WallDuration ( timeout ) );
    }
}

void GazeboRosDiffDrive::UpdateOdometryEncoder()
{
    boost::mutex::scoped_lock scoped_lock ( lock );

    double vl = joints_[LEFT]->GetVelocity ( 0 );
    double vr = joints_[RIGHT]->GetVelocity ( 0 );
    common::Time current_time = parent->GetWorld()->GetSimTime();
    double seconds_since_last_update = ( current_time - last_odom_update_ ).Double();
    last_odom_update_ = current_time;

    double b = wheel_separation_;

    // Book: Sigwart 2011 Autonompus Mobile Robots page:337
    double sl = vl * wheel_radius_ * seconds_since_last_update;
    double sr = vr * wheel_radius_ * seconds_since_last_update;
    //double theta = ( sl - sr ) / b;


    double dx = ( sl + sr ) / 2. * cos ( pose_encoder_.theta + ( sl - sr ) / ( 2.0*b ) );
    double dy = ( sl + sr ) / 2. * sin ( pose_encoder_.theta + ( sl - sr ) / ( 2.0*b ) );
    double dtheta = ( sl - sr ) / b;

    pose_encoder_.x += dx;
    pose_encoder_.y += dy;
    pose_encoder_.theta += dtheta;

    double w = dtheta/seconds_since_last_update;
    //double v = sqrt ( dx*dx+dy*dy ) /seconds_since_last_update;

    tf::Quaternion qt;
    tf::Vector3 vt;
    qt.setRPY ( 0,0,pose_encoder_.theta );
    vt = tf::Vector3 ( pose_encoder_.x, pose_encoder_.y, 0 );

    odom_.pose.pose.position.x = vt.x();
    odom_.pose.pose.position.y = vt.y();
    odom_.pose.pose.position.z = vt.z();

    odom_.pose.pose.orientation.x = qt.x();
    odom_.pose.pose.orientation.y = qt.y();
    odom_.pose.pose.orientation.z = qt.z();
    odom_.pose.pose.orientation.w = qt.w();

    odom_.twist.twist.angular.z = w;
    odom_.twist.twist.linear.x = dx/seconds_since_last_update;
    odom_.twist.twist.linear.y = dy/seconds_since_last_update;
}

void GazeboRosDiffDrive::publishOdometry ( double /*step_time*/ )
{
    boost::mutex::scoped_lock scoped_lock ( lock );

    ros::Time current_time = ros::Time::now();
    std::string odom_frame = gazebo_ros_->resolveTF ( odometry_frame_ );
    std::string base_footprint_frame = gazebo_ros_->resolveTF ( robot_base_frame_ );

    tf::Quaternion qt;
    tf::Vector3 vt;

    if ( odom_source_ == ENCODER ) {
        // getting data from encoder integration
        qt = tf::Quaternion ( odom_.pose.pose.orientation.x, odom_.pose.pose.orientation.y, odom_.pose.pose.orientation.z, odom_.pose.pose.orientation.w );
        vt = tf::Vector3 ( odom_.pose.pose.position.x, odom_.pose.pose.position.y, odom_.pose.pose.position.z );
    }

    if ( odom_source_ == WORLD ) {
        // getting data from gazebo world
        math::Pose pose = parent->GetWorldPose();
        qt = tf::Quaternion ( pose.rot.x, pose.rot.y, pose.rot.z, pose.rot.w );
        vt = tf::Vector3 ( pose.pos.x, pose.pos.y, pose.pos.z );

        odom_.pose.pose.position.x = vt.x();
        odom_.pose.pose.position.y = vt.y();
        odom_.pose.pose.position.z = vt.z();

        odom_.pose.pose.orientation.x = qt.x();
        odom_.pose.pose.orientation.y = qt.y();
        odom_.pose.pose.orientation.z = qt.z();
        odom_.pose.pose.orientation.w = qt.w();

        // get velocity in /odom frame
        math::Vector3 linear;
        linear = parent->GetWorldLinearVel();
        odom_.twist.twist.angular.z = parent->GetWorldAngularVel().z;

        // convert velocity to child_frame_id (aka base_footprint)
        float yaw = pose.rot.GetYaw();
        odom_.twist.twist.linear.x = cosf ( yaw ) * linear.x + sinf ( yaw ) * linear.y;
        odom_.twist.twist.linear.y = cosf ( yaw ) * linear.y - sinf ( yaw ) * linear.x;
    }

    tf::Transform base_footprint_to_odom ( qt, vt );
    transform_broadcaster_->sendTransform (
        tf::StampedTransform ( base_footprint_to_odom, current_time,
                               odom_frame, base_footprint_frame ) );


    // set covariance
    odom_.pose.covariance[0] = 0.00001;
    odom_.pose.covariance[7] = 0.00001;
    odom_.pose.covariance[14] = 1000000000000.0;
    odom_.pose.covariance[21] = 1000000000000.0;
    odom_.pose.covariance[28] = 1000000000000.0;
    odom_.pose.covariance[35] = 0.001;


    // set header
    odom_.header.stamp = current_time;
    odom_.header.frame_id = odom_frame;
    odom_.child_frame_id = base_footprint_frame;

    odometry_publisher_.publish ( odom_ );
}

GZ_REGISTER_MODEL_PLUGIN ( GazeboRosDiffDrive )
}


