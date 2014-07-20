/*
 * Software License Agreement (Apache License)
 *
 * Copyright (c) 2014, Southwest Research Institute
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <industrial_extrinsic_cal/ros_transform_interface.h>
#include <iostream>
#include <fstream>
namespace industrial_extrinsic_cal
{
  using std::string;

  ROSListenerTransInterface::ROSListenerTransInterface(const string & transform_frame) 
  {
    transform_frame_ = transform_frame;
    transform_.child_frame_id_ = transform_frame_;
    ref_frame_initialized_ = false;    // still need to initialize ref_frame_
  }				

  Pose6d  ROSListenerTransInterface::pullTransform()
  {
    if(!ref_frame_initialized_){
      Pose6d pose(0,0,0,0,0,0);
      ROS_ERROR("Trying to pull transform from interface without setting reference frame");
      return(pose);
    }
    else{
      tf::StampedTransform transform;
      ros::Time now = ros::Time::now()-ros::Duration(.5);
      while(! tf_listener_.waitForTransform(transform_frame_,ref_frame_, now, ros::Duration(1.0))){
	ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),ref_frame_.c_str());
      }
      tf_listener_.lookupTransform(transform_frame_,ref_frame_, now, transform);
      pose_.setBasis(transform.getBasis());
      pose_.setOrigin(transform.getOrigin());
      return(pose_);
    }
  }

  ROSCameraListenerTransInterface::ROSCameraListenerTransInterface(const string & transform_frame) 
  {
    transform_frame_ = transform_frame;
    transform_.child_frame_id_ = transform_frame_;
    ref_frame_initialized_ = false;    // still need to initialize ref_frame_
  }				

  Pose6d  ROSCameraListenerTransInterface::pullTransform()
  {
    if(!ref_frame_initialized_){
      Pose6d pose(0,0,0,0,0,0);
      ROS_ERROR("Trying to pull transform from interface without setting reference frame");
      return(pose);
    }
    else{
      tf::StampedTransform transform;
      ros::Time now = ros::Time::now()-ros::Duration(.5);
      while(! tf_listener_.waitForTransform(ref_frame_,transform_frame_, now, ros::Duration(1.0))){
	ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),ref_frame_.c_str());
      }
      tf_listener_.lookupTransform(ref_frame_,transform_frame_, now, transform);
      pose_.setBasis(transform.getBasis());
      pose_.setOrigin(transform.getOrigin());
      return(pose_);
    }
  }

  /** @brief this object is intened to be used for cameras not targets
   *            It simply listens to a pose from camera's optical frame to reference frame, this must be set in a urdf
   *            This is the inverse of the transform from world to camera's optical frame
   *            push does nothing
   *            store does nothing
   */
  ROSCameraHousingListenerTInterface::ROSCameraHousingListenerTInterface(const string & transform_frame, const string & housing_frame) 
  {
    transform_frame_               = transform_frame;
    housing_frame_                  = housing_frame; // note, this is not used, but maintained to be symetric with Broadcaster parameter list
    ref_frame_initialized_         = false;    // still need to initialize ref_frame_
  }				

  Pose6d  ROSCameraHousingListenerTInterface::pullTransform()
  {
    if(!ref_frame_initialized_){
      Pose6d pose(0,0,0,0,0,0);
      ROS_ERROR("Trying to pull transform from interface without setting reference frame");
      return(pose);
    }
    else{
      tf::StampedTransform transform;
      ros::Time now = ros::Time::now()-ros::Duration(.5);
      while(! tf_listener_.waitForTransform(ref_frame_,transform_frame_, now, ros::Duration(1.0))){
	ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),ref_frame_.c_str());
      }
      tf_listener_.lookupTransform(ref_frame_,transform_frame_, now, transform);
      pose_.setBasis(transform.getBasis());
      pose_.setOrigin(transform.getOrigin());
      return(pose_);
    }
  }

  ROSBroadcastTransInterface::ROSBroadcastTransInterface(const string & transform_frame, const Pose6d & pose)
  {
    transform_frame_                = transform_frame;
    transform_.child_frame_id_ = transform_frame_;
    ref_frame_initialized_         = false;    // still need to initialize ref_frame_
    pose_                                 = pose;
  }

  bool   ROSBroadcastTransInterface::pushTransform(Pose6d & pose)
  {
    pose_ = pose; 
    if(!ref_frame_initialized_){ 
      return(false);		// timer won't start publishing until ref_frame_ is defined
    }
    return(true);
  }

  bool  ROSBroadcastTransInterface::store(std::string & filePath)
  {
    std::ofstream outputFile(filePath.c_str(), std::ios::app); // open for appending
    if (outputFile.is_open())
      {
	double qx,qy,qz,qw;
	pose_.getQuaternion(qx, qy, qz, qw);
	outputFile<<"<node pkg=\"tf\" type=\"static_transform_publisher\" name=\"";
	outputFile<< transform_frame_ <<"_tf_broadcaster"<<"\" args=\"";
	outputFile<< pose_.x << ' '<< pose_.y << ' '<< pose_.z << ' ';
	outputFile<< qx << ' '<< qy << ' '<<qz << ' ' << qw ;
	outputFile<<" "<< ref_frame_;
	outputFile<<" "<<transform_frame_;
	outputFile<<" 100\" />"<<std::endl;
	outputFile.close();	
	return(true);
      }
    else
      {
	ROS_ERROR_STREAM("Unable to open file:" <<filePath);
	return false;
      }//end if writing to file
  }

  void  ROSBroadcastTransInterface::setReferenceFrame(string &ref_frame)
  {
    static ros::NodeHandle nh;
    ref_frame_              = ref_frame;
    ref_frame_defined_ = true;
    timer_                     = nh.createTimer(ros::Rate(1.0),&ROSBroadcastTransInterface::timerCallback, this);
  }

  void  ROSBroadcastTransInterface::timerCallback(const ros::TimerEvent & timer_event)
  { // broadcast current value of pose as a transform each time called
    transform_.setBasis(pose_.getBasis());
    transform_.setOrigin(pose_.getOrigin());
    transform_.child_frame_id_ = transform_frame_;
    transform_.frame_id_ = ref_frame_;
    //    ROS_INFO("broadcasting %s in %s",transform_frame_.c_str(),ref_frame_.c_str());
    tf_broadcaster_.sendTransform(tf::StampedTransform(transform_, ros::Time::now(), transform_frame_, ref_frame_));
  }

  ROSCameraBroadcastTransInterface::ROSCameraBroadcastTransInterface(const string & transform_frame, const Pose6d & pose)
  {
    transform_frame_                = transform_frame;
    transform_.child_frame_id_ = transform_frame_;
    ref_frame_initialized_         = false;    // still need to initialize ref_frame_
    pose_                                 = pose;
  }

  bool   ROSCameraBroadcastTransInterface::pushTransform(Pose6d & pose)
  {
    pose_ = pose; 
    if(!ref_frame_initialized_){ 
      return(false);		// timer won't start publishing until ref_frame_ is defined
    }
    return(true);
  }

  bool  ROSCameraBroadcastTransInterface::store(std::string & filePath)
  {
    std::ofstream outputFile(filePath.c_str(), std::ios::app); // open for appending
    if (outputFile.is_open())
      {
	double qx,qy,qz,qw;
	pose_.getInverse().getQuaternion(qx, qy, qz, qw);
	outputFile<<"<node pkg=\"tf\" type=\"static_transform_publisher\" name=\"";
	outputFile<<transform_frame_<<"_tf_broadcaster"<<"\" args=\"";
	outputFile<< pose_.getInverse().x << ' '<< pose_.getInverse().y << ' '<< pose_.getInverse().z << ' ';
	outputFile<< qx << ' '<< qy << ' '<<qz << ' ' << qw ;
	outputFile<<" "<<ref_frame_;
	outputFile<<" "<<transform_frame_;
	outputFile<<" 100\" />"<<std::endl;
	outputFile.close();	
	return(true);
      }
    else
      {
	ROS_ERROR_STREAM("Unable to open file:" <<filePath);
	return false;
      }//end if writing to file
  }

  void ROSCameraBroadcastTransInterface::setReferenceFrame(string &ref_frame)
  {
    static ros::NodeHandle nh;
    ref_frame_              = ref_frame;
    ref_frame_defined_ = true;
    timer_                     = nh.createTimer(ros::Rate(1.0),&ROSCameraBroadcastTransInterface::timerCallback, this);
  }

  void  ROSCameraBroadcastTransInterface::timerCallback(const ros::TimerEvent & timer_event)
  { // broadcast current value of pose.inverse() as a transform each time called
    transform_.setBasis(pose_.getInverse().getBasis());
    transform_.setOrigin(pose_.getInverse().getOrigin());
    transform_.child_frame_id_ = transform_frame_;
    transform_.frame_id_ = ref_frame_;
    //    ROS_INFO("broadcasting %s in %s",transform_frame_.c_str(),ref_frame_.c_str());
    tf_broadcaster_.sendTransform(tf::StampedTransform(transform_, ros::Time::now(), transform_frame_, ref_frame_));
  }

  ROSCameraHousingBroadcastTInterface::ROSCameraHousingBroadcastTInterface(const string & transform_frame, const Pose6d & pose)
  {
    transform_frame_                = transform_frame;
    transform_.child_frame_id_ = transform_frame_;
    ref_frame_initialized_         = false;    // still need to initialize ref_frame_
    pose_                                 = pose;
  }

  bool   ROSCameraHousingBroadcastTInterface::pushTransform(Pose6d & pose)
  {
    pose_ = pose; 
    if(!ref_frame_initialized_){ 
      return(false);		// timer won't start publishing until ref_frame_ is defined
    }
    return(true);
  }

  bool  ROSCameraHousingBroadcastTInterface::store(std::string & filePath)
  {
    std::ofstream outputFile(filePath.c_str(), std::ios::app); // open for appending
    if (outputFile.is_open()){
      // Camer optical frame to ref is estimated by bundle adjustment  T_co2ref
      // Camer housing to camera optical frame is specified by urdf   T_ch2co
      // Desired T_ref2ch = T_co2ref^(-1) * T_ch2co^(-1)
      // To get T_ch2co^(-1) we may use the tf listener
      
      Pose6d T_co2ch;
      tf::StampedTransform transform;
      ros::Time now = ros::Time::now()-ros::Duration(.5);
      while(! tf_listener_.waitForTransform(transform_frame_,housing_frame_, now, ros::Duration(1.0))){
	ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),housing_frame_.c_str());
      }
      tf_listener_.lookupTransform(transform_frame_, housing_frame_, now, transform);
      T_co2ch.setBasis(transform.getBasis());
      T_co2ch.setOrigin(transform.getOrigin());
      Pose6d T_ref2ch = pose_.getInverse() * T_co2ch;
      
      // append the transform to a launch file
      double qx,qy,qz,qw;
      T_ref2ch.getQuaternion(qx, qy, qz, qw);
      outputFile<<"<node pkg=\"tf\" type=\"static_transform_publisher\" name=\"";
      outputFile<<transform_frame_<<"_tf_broadcaster"<<"\" args=\"";
      outputFile<< T_ref2ch.x << ' '<< T_ref2ch.y << ' '<< T_ref2ch.z << ' ';
      outputFile<< qx << ' '<< qy << ' '<<qz << ' ' << qw ;
      outputFile<<" "<<ref_frame_;
      outputFile<<" "<<transform_frame_;
      outputFile<<" 100\" />"<<std::endl;
      outputFile.close();	
      return(true);
    }
    else{
      ROS_ERROR_STREAM("Unable to open file:" <<filePath);
      return false;
    }//end if writing to file
  }

  void  ROSCameraHousingBroadcastTInterface::setReferenceFrame(std::string &ref_frame)
  {
    static ros::NodeHandle nh;
    ref_frame_              = ref_frame;
    ref_frame_defined_ = true;
    timer_                     = nh.createTimer(ros::Rate(1.0),&ROSCameraHousingBroadcastTInterface::timerCallback, this);
  }

  void  ROSCameraHousingBroadcastTInterface::timerCallback(const ros::TimerEvent & timer_event)
  { // broadcast current value of pose.inverse() as a transform each time called

    // Camer optical frame to ref is estimated by bundle adjustment  T_co2ref
    // Camer housing to camera optical frame is specified by urdf   T_ch2co
    // Desired T_ref2ch = T_co2ref^(-1) * T_ch2co^(-1)
    // To get T_ch2co^(-1) we may use the tf listener

    Pose6d T_co2ch;
    tf::StampedTransform transform;
    ros::Time now = ros::Time::now()-ros::Duration(.5);
    while(! tf_listener_.waitForTransform(transform_frame_,housing_frame_, now, ros::Duration(1.0))){
      ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),housing_frame_.c_str());
    }
    tf_listener_.lookupTransform(transform_frame_, housing_frame_, now, transform);
    T_co2ch.setBasis(transform.getBasis());
    T_co2ch.setOrigin(transform.getOrigin());
    Pose6d T_ref2ch = pose_.getInverse() * T_co2ch;
    
    // copy into the stamped transform
    transform_.setBasis(T_ref2ch.getBasis());
    transform_.setOrigin(T_ref2ch.getOrigin());
    transform_.child_frame_id_ = housing_frame_;
    transform_.frame_id_ = ref_frame_;
    //    ROS_INFO("broadcasting %s in %s",housing_frame_.c_str(),ref_frame_.c_str());
    tf_broadcaster_.sendTransform(tf::StampedTransform(transform_, ros::Time::now(), housing_frame_, ref_frame_));
  }

  ROSCameraHousingCalTInterface::ROSCameraHousingCalTInterface(const string &transform_frame, 
							       const string &housing_frame, 
							       const string &mounting_frame) 
  {
    transform_frame_               = transform_frame;
    housing_frame_                  = housing_frame; 
    mounting_frame_               = mounting_frame;
    ref_frame_initialized_         = false;    // still need to initialize ref_frame_
    nh_ = new ros::NodeHandle;

    std::string bn("mutable_joint_state_publisher/");
    get_client_    = nh_->serviceClient<industrial_extrinsic_cal::get_mutable_joint_states>("get_mutable_joint_states");
    set_client_    = nh_->serviceClient<industrial_extrinsic_cal::set_mutable_joint_states>("set_mutable_joint_states");
    store_client_ = nh_->serviceClient<industrial_extrinsic_cal::store_mutable_joint_states>("store_mutable_joint_states");

    get_request_.joint_names.push_back(housing_frame+"_x_joint");
    get_request_.joint_names.push_back(housing_frame+"_y_joint");
    get_request_.joint_names.push_back(housing_frame+"_z_joint");
    get_request_.joint_names.push_back(housing_frame+"_pitch_joint");
    get_request_.joint_names.push_back(housing_frame+"_yaw_joint");
    get_request_.joint_names.push_back(housing_frame+"_roll_joint");

    set_request_.joint_names.push_back(housing_frame+"_x_joint");
    set_request_.joint_names.push_back(housing_frame+"_y_joint");
    set_request_.joint_names.push_back(housing_frame+"_z_joint");
    set_request_.joint_names.push_back(housing_frame+"_pitch_joint");
    set_request_.joint_names.push_back(housing_frame+"_yaw_joint");
    set_request_.joint_names.push_back(housing_frame+"_roll_joint");

    if(get_client_.call(get_request_,get_response_)){
      for(int i=0;i<(int) get_response_.joint_values.size();i++){
	joint_values_.push_back(get_response_.joint_values[i]);
      }
    }
    else{
      ROS_ERROR("get_client_ returned false");
    }

  }				

  Pose6d  ROSCameraHousingCalTInterface::pullTransform()
  {
    // The computed transform from the reference frame to the optical frame is composed of 3 transforms
    // one from reference frame to mounting frame
    // one composed of the 6DOF unknowns we are trying to calibrate
    // from the mounting frame to the housing. It's values are maintained by the mutable joint state publisher
    // the third is from housing to optical frame

    if(!ref_frame_initialized_){ // still need the reference frame in order to return the transform!!
      Pose6d pose(0,0,0,0,0,0);
      ROS_ERROR("Trying to pull transform from interface without setting reference frame");
      return(pose);
    }

    // get all the information from tf and from the mutable joint state publisher
    tf::StampedTransform o2h_transform; // Optical to housing frame
    ros::Time now = ros::Time::now()-ros::Duration(.5);
    while(! tf_listener_.waitForTransform(transform_frame_,housing_frame_, now, ros::Duration(1.0))){
      ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),housing_frame_.c_str());
    }
    tf_listener_.lookupTransform(ref_frame_,transform_frame_, now, o2h_transform);
    Pose6d optical2housing;
    optical2housing.setBasis(o2h_transform.getBasis());
    optical2housing.setOrigin(o2h_transform.getOrigin());

    tf::StampedTransform m2r_transform; // Mounting to reference frame
    while(! tf_listener_.waitForTransform(ref_frame_,mounting_frame_, now, ros::Duration(1.0))){
      ROS_INFO("waiting for tranform: %s to reference: %s",mounting_frame_.c_str(),ref_frame_.c_str());
    }

    tf_listener_.lookupTransform(ref_frame_,transform_frame_, now, m2r_transform);
    Pose6d mount2ref;
    mount2ref.setBasis(m2r_transform.getBasis());
    mount2ref.setOrigin(m2r_transform.getOrigin());

    Pose6d mount2housing;
    get_client_.call(get_request_,get_response_);
    mount2housing.setOrigin(get_response_.joint_values[0],get_response_.joint_values[1],get_response_.joint_values[2]);
    mount2housing.setEulerZYX(get_response_.joint_values[3],get_response_.joint_values[4],get_response_.joint_values[5]);
    //    ROS_ERROR("jv: %lf  %lf  %lf %lf %lf %lf", get_response_.joint_values[0],
    //	      get_response_.joint_values[1],
    //	      get_response_.joint_values[2],
    //	      get_response_.joint_values[3],
    //	      get_response_.joint_values[4],
    //	      get_response_.joint_values[5]);

  //    ROS_ERROR("mount2housing_ = %lf %lf %lf  %lf %lf %lf", mount2housing.ax, mount2housing.ay, mount2housing.az, mount2housing.x, mount2housing.y, mount2housing.z);
    Pose6d housing2mount = mount2housing.getInverse();
    
    pose_ = optical2housing * housing2mount * mount2ref; // construct the transform from the three terms
    
  //    ROS_ERROR("optical2housing_ = %lf %lf %lf  %lf %lf %lf", optical2housing.ax, optical2housing.ay, optical2housing.az, optical2housing.x, optical2housing.y, optical2housing.z);
  //    ROS_ERROR("housing2mount_ = %lf %lf %lf  %lf %lf %lf", housing2mount.ax, housing2mount.ay, housing2mount.az, housing2mount.x, housing2mount.y, housing2mount.z);
  //    ROS_ERROR("mount2ref_ = %lf %lf %lf  %lf %lf %lf", mount2ref.ax, mount2ref.ay, mount2ref.az, mount2ref.x, mount2ref.y, mount2ref.z);
  //    ROS_ERROR("pose_ = %lf %lf %lf  %lf %lf %lf", pose_.ax, pose_.ay, pose_.az, pose_.x, pose_.y, pose_.z);
    return(pose_);
  }

  bool  ROSCameraHousingCalTInterface::pushTransform(Pose6d &pose)
  {
    // get all the information from tf and from the mutable joint state publisher
    tf::StampedTransform o2h_transform; // Optical to housing frame
    ros::Time now = ros::Time::now()-ros::Duration(.5);
    while(! tf_listener_.waitForTransform(transform_frame_,housing_frame_, now, ros::Duration(1.0))){
      ROS_INFO("waiting for tranform: %s to reference: %s",transform_frame_.c_str(),housing_frame_.c_str());
    }
    tf_listener_.lookupTransform(ref_frame_,transform_frame_, now, o2h_transform);
    Pose6d optical2housing;
    optical2housing.setBasis(o2h_transform.getBasis());
    optical2housing.setOrigin(o2h_transform.getOrigin());
    ROS_ERROR("optical2housing %lf %lf %lf %lf %lf %lf", optical2housing.ax,
	      optical2housing.ay,
	      optical2housing.az,
	      optical2housing.x,
	      optical2housing.y,
	      optical2housing.z);

    tf::StampedTransform m2r_transform; // Mounting to reference frame
    while(! tf_listener_.waitForTransform(ref_frame_,mounting_frame_, now, ros::Duration(1.0))){
      ROS_INFO("waiting for tranform: %s to reference: %s",mounting_frame_.c_str(),ref_frame_.c_str());
    }
    tf_listener_.lookupTransform(ref_frame_,mounting_frame_, now, m2r_transform);
    Pose6d mount2ref;
    mount2ref.setBasis(m2r_transform.getBasis());
    mount2ref.setOrigin(m2r_transform.getOrigin());

    // compute the desired transform
    Pose6d mount2housing = mount2ref * pose.getInverse() * optical2housing;

    tf::Vector3 T1     = mount2housing.getOrigin();
    double ez,ey,ex;
    mount2housing.getEulerZYX(ez,ey,ex);
    set_request_.joint_values.clear();
    set_request_.joint_values.push_back(T1[0]);
    set_request_.joint_values.push_back(T1[1]);
    set_request_.joint_values.push_back(T1[2]);
    set_request_.joint_values.push_back(ez);
    set_request_.joint_values.push_back(ey);
    set_request_.joint_values.push_back(ex);
    set_client_.call(set_request_,set_response_);

    return(true);
  }

  bool ROSCameraHousingCalTInterface::store(std::string &filePath)
  {
    // NOTE, file_name is not used, but is kept here for consistency with store functions of other transform interfaces
    store_client_.call(store_request_, store_response_);
    return(true);
  }

  void ROSCameraHousingCalTInterface::setReferenceFrame(std::string &ref_frame)
  {
    ref_frame_ = ref_frame;
    ref_frame_initialized_ = true;
  }

  Pose6d ROSCameraHousingCalTInterface::getIntermediateFrame()
  {
    tf::StampedTransform r2m_transform; // ref to mounting frame
    ros::Time now = ros::Time::now()-ros::Duration(.5);
    while(! tf_listener_.waitForTransform(mounting_frame_,ref_frame_, now, ros::Duration(1.0))){
      ROS_INFO("waiting for tranform: %s to reference: %s",mounting_frame_.c_str(),ref_frame_.c_str());
    }
    tf_listener_.lookupTransform(mounting_frame_, ref_frame_, now, r2m_transform);
    Pose6d mount2ref;
    mount2ref.setBasis(r2m_transform.getBasis());
    mount2ref.setOrigin(r2m_transform.getOrigin());
    return(mount2ref);
  }

  ROSSimpleCalTInterface::ROSSimpleCalTInterface(const string &transform_frame,  const string &parent_frame)
  {
    transform_frame_               = transform_frame;
    parent_frame_                     = parent_frame; 
    ref_frame_initialized_         = false;    // still need to initialize ref_frame_
    nh_ = new ros::NodeHandle;

    std::string bn("mutable_joint_state_publisher/");
    get_client_    = nh_->serviceClient<industrial_extrinsic_cal::get_mutable_joint_states>("get_mutable_joint_states");
    set_client_    = nh_->serviceClient<industrial_extrinsic_cal::set_mutable_joint_states>("set_mutable_joint_states");
    store_client_ = nh_->serviceClient<industrial_extrinsic_cal::store_mutable_joint_states>("store_mutable_joint_states");

    get_request_.joint_names.push_back(transform_frame+"_x_joint");
    get_request_.joint_names.push_back(transform_frame+"_y_joint");
    get_request_.joint_names.push_back(transform_frame+"_z_joint");
    get_request_.joint_names.push_back(transform_frame+"_pitch_joint");
    get_request_.joint_names.push_back(transform_frame+"_yaw_joint");
    get_request_.joint_names.push_back(transform_frame+"_roll_joint");

    set_request_.joint_names.push_back(transform_frame+"_x_joint");
    set_request_.joint_names.push_back(transform_frame+"_y_joint");
    set_request_.joint_names.push_back(transform_frame+"_z_joint");
    set_request_.joint_names.push_back(transform_frame+"_pitch_joint");
    set_request_.joint_names.push_back(transform_frame+"_yaw_joint");
    set_request_.joint_names.push_back(transform_frame+"_roll_joint");

    if(get_client_.call(get_request_,get_response_)){
      for(int i=0;i<(int) get_response_.joint_values.size();i++){
	joint_values_.push_back(get_response_.joint_values[i]);
      }
    }
    else{
      ROS_ERROR("get_client_ returned false");
    }

  }				

  Pose6d  ROSSimpleCalTInterface::pullTransform()
  {
    // The computed transform from the reference frame to the optical frame is composed of 3 transforms
    // one from reference frame to mounting frame
    // one composed of the 6DOF unknowns we are trying to calibrate
    // from the mounting frame to the housing. It's values are maintained by the mutable joint state publisher
    // the third is from housing to optical frame

    if(!ref_frame_initialized_){ // still need the reference frame in order to return the transform!!
      Pose6d pose(0,0,0,0,0,0);
      ROS_ERROR("Trying to pull transform from interface without setting reference frame");
      return(pose);
    }

    get_client_.call(get_request_,get_response_);
    pose_.setOrigin(get_response_.joint_values[0],get_response_.joint_values[1],get_response_.joint_values[2]);
    pose_.setEulerZYX(get_response_.joint_values[3],get_response_.joint_values[4],get_response_.joint_values[5]);
    return(pose_);
  }

  bool  ROSSimpleCalTInterface::pushTransform(Pose6d &pose)
  {
    double ez,ey,ex;
    pose.getEulerZYX(ez,ey,ex);
    
    set_request_.joint_values.clear();
    set_request_.joint_values.push_back(pose.x);
    set_request_.joint_values.push_back(pose.y);
    set_request_.joint_values.push_back(pose.z);
    set_request_.joint_values.push_back(ez);
    set_request_.joint_values.push_back(ey);
    set_request_.joint_values.push_back(ex);
    set_client_.call(set_request_,set_response_);

    return(true);
  }

  bool ROSSimpleCalTInterface::store(std::string &filePath)
  {
    // NOTE, file_name is not used, but is kept here for consistency with store functions of other transform interfaces
    store_client_.call(store_request_, store_response_);
    return(true);
  }

  void ROSSimpleCalTInterface::setReferenceFrame(std::string &ref_frame)
  {
    ref_frame_ = ref_frame;
    ref_frame_initialized_ = true;
  }

} // end namespace industrial_extrinsic_cal
