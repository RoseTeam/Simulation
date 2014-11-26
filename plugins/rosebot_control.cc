/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <boost/bind.hpp>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>
#include <stdio.h>

namespace gazebo
{
  class ModelDrive : public ModelPlugin
  {
    public: void Load(physics::ModelPtr _parent, sdf::ElementPtr /*_sdf*/)
    {
      // Store the pointer to the model
      this->model = _parent;

      // Listen to the update event. This event is broadcast every
      // simulation iteration.
      this->updateConnection = event::Events::ConnectWorldUpdateBegin(
          boost::bind(&ModelDrive::OnUpdate, this, _1));

      // Get the right and left doors, and the lift
      rightWheel_ = model->GetLink("right_wheel");
      leftWheel_  = model->GetLink("left_wheel");
 	/*
      // Get the first contact sensor
      contact_ = (Geom*)World::Instance()->GetEntityByName("contact::body::COM_Entity::geom");
      contact_->SetContactsEnabled(true);
      contact_->ConnectContactCallback(boost::bind(&Elevator::ContactCB, this));
 
 
      // Get the second contact sensor
      contact2_ = (Geom*)World::Instance()->GetEntityByName("lift::body::COM_Entity::contact");
      contact2_->SetContactsEnabled(true);
      contact2_->ConnectContactCallback(boost::bind(&Elevator::Contact2CB, this));
 
      // Get an update callback
      World::Instance()->ConnectWorldUpdateStartSignal(boost::bind(&Elevator::UpdateCB, this));
	*/
    }

    // Called by the world update start event
    public: void OnUpdate(const common::UpdateInfo & /*_info*/)
    {
      // Apply a small linear velocity to the model.
    	math::Vector3 tor(0, 0, -0.2);
    	this->leftWheel_->AddRelativeTorque(tor);
    	this->rightWheel_->AddRelativeTorque(-tor);
    	forward_ = true;
    }

    private: 
		bool forward_;
		physics::LinkPtr rightWheel_, leftWheel_;
		physics::ModelPtr model;    // Pointer to the model


		// Pointer to the update event connection
		event::ConnectionPtr updateConnection;
	//Geom *contact_, *contact2_;
  };

  // Register this plugin with the simulator
  GZ_REGISTER_MODEL_PLUGIN(ModelDrive)
}

