/* Copyright (c) 2017, United States Government, as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 *
 * All rights reserved.
 *
 * The Astrobee platform is licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with the
 * License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef CHOREOGRAPHER_VALIDATOR_H_
#define CHOREOGRAPHER_VALIDATOR_H_

// ROS libraries
#include <ros/ros.h>

// ROS messages
#include <visualization_msgs/MarkerArray.h>

// FSW utils
#include <ff_util/ff_flight.h>
#include <ff_util/ff_serialization.h>
#include <ff_util/config_server.h>

// FSW messages
#include <ff_msgs/Zone.h>
#include <ff_msgs/SetZones.h>
#include <ff_msgs/GetZones.h>

// STL includes
#include <string>

namespace choreographer {

class Validator {
 public:
  // Possible responses
  enum Response : uint8_t {
    SUCCESS,
    VIOLATES_RESAMPLING,
    VIOLATES_KEEP_OUT,
    VIOLATES_KEEP_IN,
    VIOLATES_MINIMUM_FREQUENCY,
    VIOLATES_STATIONARY_ENDPOINT,
    VIOLATES_FIRST_IN_PAST,
    VIOLATES_MINIMUM_SETPOINTS,
    VIOLATES_HARD_LIMIT_VEL,
    VIOLATES_HARD_LIMIT_ACCEL,
    VIOLATES_HARD_LIMIT_OMEGA,
    VIOLATES_HARD_LIMIT_ALPHA
  };

  // Load the keep in the keeo out zones and return if successful
  bool Init(ros::NodeHandle *nh, ff_util::ConfigServer & cfg);

  // If the check fails, then the info block is populated
  Response CheckSegment(ff_util::Segment const& msg,
    ff_msgs::FlightMode const& flight_mode, bool face_forward);

 protected:
  // Markers for keep in / keep out zones
  void PublishMarkers();

  // Check if a point is inside a cuboid
  bool PointInsideCuboid(geometry_msgs::Point const& x,
                         geometry_msgs::Vector3 const& cubemin,
                         geometry_msgs::Vector3 const& cubemax);

  // Callback to get the keep in/out zones
  bool GetZonesCallback(ff_msgs::GetZones::Request& req,
                       ff_msgs::GetZones::Response& res);

  // Callback to set the keep in/out zones
  bool SetZonesCallback(ff_msgs::SetZones::Request& req,
                       ff_msgs::SetZones::Response& res);

 private:
  std::string zone_file_;                             // Zone file path
  bool overwrite_;                                    // New zones overwrite
  ff_msgs::SetZones::Request zones_;                  // Zones
  ros::Publisher pub_zones_;                          // Zone publisher
  ros::ServiceServer get_zones_srv_;                  // Set zone service
  ros::ServiceServer set_zones_srv_;                  // Set zone service
};

}  // namespace choreographer

#endif  // CHOREOGRAPHER_VALIDATOR_H_
