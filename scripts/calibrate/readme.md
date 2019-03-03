\mainpage Astrobee Calibration

This folder contains various scripts for calibration.

# Setup
- Build and install the Astrobee code on the robot.
- Install Kalibr on your computer.

## Installation instructions for Ubuntu 16.04

sudo apt install python-rosinstall ipython python-software-properties python-git \
        ipython python-catkin-tools
sudo apt install libopencv-dev ros-kinetic-vision-opencv

# Install pip and use it to install python packages
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
sudo python get-pip.py
sudo -H pip install testresources
sudo -H pip install python-igraph --upgrade
sudo -H pip install opencv-python

If necessary, the pip packages can also be installed in user space.

# Build using catkin
export KALIBR_WS=$HOME/source/kalibr_workspace
mkdir -p $KALIBR_WS/src
cd $KALIBR_WS
source /opt/ros/kinetic/setup.bash
catkin init
catkin config --extend /opt/ros/kinetic
catkin config --cmake-args -DCMAKE_BUILD_TYPE=Release
cd $KALIBR_WS/src
git clone git@github.com:oleg-alexandrov/Kalibr.git

# This line takes care of catkin not finding numpy.
ln -s /usr/local/lib/python2.7/dist-packages/numpy/core/include/numpy $KALIBR_WS/src/Kalibr/Schweizer-Messer/numpy_eigen/include

catkin build -DCMAKE_BUILD_TYPE=Release -j4

# Prepare the environment. This is expected for all the steps below.

Run, for example:

    export SOURCE_PATH=$HOME/freeflyer
    export SCRIPT_DIR=$SOURCE_PATH/scripts/calibrate
    export ASTROBEE_CONFIG_DIR=$SOURCE_PATH/astrobee/config

# Setup to calibrate with the dock AR target. Only run when configuration is changed.

This script only needs to be executed whenever the configuration of
the target is changed (number of tags, tag's position or id
changed). Normally this step should be skipped.

Run markers_to_Kalibr.py: Usage:

    export KALIBR_WS=$HOME/source/kalibr_workspace
    source $KALIBR_WS/devel/setup.bash
    cd $SCRIPT_DIR
    ./markers_to_Kalibr.py --config config_filename
  
config_filename: config file where the specifications of AR tags of
the target are defined. Default value is
dock_markers_specs.config. This will write the file:

   $SOURCE_PATH/scripts/calibrate/config/granite_april_tag.yaml

Note that $KALIBR_WS should be defined since the script will also
automatically add a header file to Kalibr's directory describing the
family of AR tags (Kalibr uses AprilTag library and not the alvar
library used in the freeflyer software).

# Intrinsics camera calibration

## Recording calibration data

Prepare the environment:

    export BUILD_PATH=$HOME/freeflyer_build/native
    source $BUILD_PATH/devel/setup.bash

To launch the nodes needed for calibration, do:

   cd $SCRIPT_DIR
   roslaunch calibration.launch

On the flight unit, do:

   roslaunch astrobee astrobee.launch llp:=??? mlp:=??? nodes:=calibration_nav_cam,calibration_dock_cam,calibration_imu,pico_driver,framestore

This will start the IMU driver, camera drivers, and an image
viewer. 

Astrobee has four cameras. On one side there is the nav camera, which
acquires high-resolution images, and the haz camera, which is a type of
depth camera, and acquires a lower-resolution "amplitude" image. On the
opposite face of Astrobee it has the high-resolution dock camera and
low-resolution perch camera, which is again of the depth type. We will
refer to the high-resolution nav and dock cameras as the HD camera.

To be able to record the amplitude, which is necessary for haz_cam
calibration, the file cameras.config (on astrobee, not on the local
machine) needs to be edited before calibration.launch is started. The
api_key variable in the picoflexx section should be set to the correct
value. This will cause the depth camera to run in L2 mode, and produce
data on the "extended" topic.

After calibration.launch is run, one should also invoke

  roslaunch $SOURCE_PATH/hardware/pico_driver/launch/pico_proxy.launch 
  
which will process the extended topic and produce the needed amplitude
data.

When it is desired to record the amplitude for the perch camera,
one should pass to roslaunch the argument:

  topic:=/hw/depth_perch/extended

To calibrate nav_cam attached to a standalone computer, rather than
astrobee's built in one, one can do instead:

    roslaunch astrobee granite.launch mlp:=local llp:=disabled nodes:=nav_cam,framestore

(See freeflyer/astrobee/readme.md if the device cannot be found.)

Face the robot towards where you can hold the AR tag in front of the
camera. It should be under bright lighting conditions where the AR tag
is clearly visible.

If no image viewer was started by now, run:

    rosrun rviz rviz

and then add the 'image' topic for cam_nav and cam_dock, and the
amplitude_int topic for the haz and perch cameras. 

If the image in amplitude_int does not look quite right, one may want
to restart pico_proxy with a different value for the amplitude_factor
variable, which is a scaling factor. That one can be passed to
roslaunch, for example, as:

    amplitude_factor:=200

Begin recording the bag. Example to record nav_cam and haz_cam:

    rosbag record /hw/cam_nav /hw/depth_haz/extended/amplitude_int

For the dock and perch cams, one should do:

      rosbag record /hw/cam_dock /hw/depth_perch/extended/amplitude_int

Move the AR tag in front of the camera. If the AR tag is only
partially visible in some frames that is fine. Try to cover the entire
field of view of both cameras (be aware that it varies from depth to
HD camera), and to have the tag in various orientations.

In good networking conditions, it is acceptable to record on your own
computer, not Astrobee. A high frame rate is not required.

Stop recording.

## Processing the data

Make sure the target configuration yaml file is in
$SCRIPT_DIR/config/. If there is no yaml file or the configuration is
not the current one, follow #Setup to calibrate with the dock AR
target.

Run

     export KALIBR_WS=$HOME/source/kalibr_workspace
     source $KALIBR_WS/devel/setup.bash
     cd $SCRIPT_DIR
    ./intrinsics_calibrate.py robotname bagfile 

Arguments: 
	robotname is the robot's config file to edit (e.g., p4d, cert_unit, honey)
	bagfile is the bag with the recorded data.
Additional flags:

	--dock_cam: To calibrate the dock cam and perch cam pair. If
          not set, the script calibrates nav cam and haz cam.
	--depth_cam: To calibrate both HD camera (nav or dock) and 
          respective depth camera (haz or perch). If not set, the script
          only calibrates the HD camera.
	--only_depth_cam: To calibrate only the depth camera (haz or perch).
        --from <value> --to <value>: Use bag data between these times,
          in seconds.
        --approx_sync <value> Time tolerance for approximate image 
          synchronization [s] (default: 0.02).
        --calibration_target <value> Use this yaml file to desribe the
          calibration target, overriding the default april tag
          mentioned above.
 	--verbose: To output additional information on the Kalibr's calibration. 

The script will overwrite the intrinsics calibration in the specified
config file. It will also generate in the bagfile directory the
following files:

	.txt file with the calibration results
	.pdf report
	.yaml file with the estimated intrinsics parameters that will be used 
           as an input to the extrinsics calibration 
           (see # Extrinsic camera calibration -> ## Processing data)

## Advice on intrinsics calibration

It is strongly suggested that the intrinsics of the HD and depth
camera be calibrated separately. This makes the process much better
behaved. To calibrate the HD camera only, do not specify the flag
--depth_cam, and to calibrate the depth camera only, use
--only_depth_cam.

It is suggested to examine the calibration results*txt file. If the
errors are too large or the camera parameters are implausible, perhaps
calibration need to be rerun with a new bag file. In either case it is
suggested to run calibration several times with different bags, and
pick the result with the smallest error.

Sometimes, if calibration fails, it can be attempted again several
times with the same bag and it may succeed at some point. The
indeterministic nature is likely due to the random shuffling being
done by the algorithm. One can also attempt to use just a portion of
the bag, using the --from and --to options.

The --verbose flag is useful to see if the calibration target corners
are detected properly.

The depth cameras can be tricky to calibrate, since they have lower
resolution and it is hard to do corner detection. One should try to
have the calibration target fill the camera field of view as much as
possible while still having all of it visible at most times.

# Extrinsic camera calibration

## Recording calibration data

- Detach the robot from its stand so it can be lifted freely.
- Attach the april target to a wall under bright light.
- Launch, as for intrinsics calibration, calibration.launch and pico_proxy.launch.

- Lift the robot and face the target.
- Begin recording the bag on the robot. The recording cannot have shocks from picking
  up and placing the robot down.
     Example: rosbag record /hw/cam_nav /hw/depth_haz/extended/amplitude_int /hw/imu
- Accelerate the robot rapidly along all axes of motion. Try to excite
  all axis of the IMU. Be careful not to drop the robot.
- Stop recording.
- Put the robot down.
- Copy the bag off of the robot. If the robot is docked, the wired
  network is used, which can be much faster then WiFi.

## Processing the data

Make sure the target configuration yaml file and the IMU yaml file are in $SCRIPT_DIR/config/.

Run
     export KALIBR_WS=$HOME/source/kalibr_workspace
     source $KALIBR_WS/devel/setup.bash
     cd $SCRIPT_DIR
     ./extrinsics_calibrate.py robotname intrinsicsYaml bagfile 

Arguments: 
	robotname is the robot's config file to edit (e.g., p4d, cert_unit, honey)
	intrinsicsYaml is a YAML file generated by the previous intrinsics calibration
	bagfile is the bag with the recorded data.

Additional flags:
 	--dock_cam: To calibrate dock cam and perch cam pair. If not
	  set, the script calibrates nav cam and haz cam.
        --from <value> --to <value>: Use bag data between these times, in seconds.
        --timeoffset-padding <value>: Maximum range in which the
          timeoffset may change during estimation, in seconds. See below
          for more info. (default: 0.01)
        --calibration_target <value> Use this yaml file to desribe the
          calibration target, overriding the default april tag
          mentioned above.
 	--verbose: To output additional information on Kalibr's
          calibration.

If the previous intrinsics calibration was run for the HD and depth
camera pair, then the output will generate extrinsics for both
cameras. If not, it will only generate for the nav or dock cam. The
script will overwrite the extrinsics calibration in the specified
robot's config file.  It will also generate some results reports in
the bagfile directory.

## Advice on extrinsics calibration

If kalibr crashes with the error "Spline Coefficient Buffer
Exceeded. Set larger buffer margins!" one should increase the value of
--timeoffset-padding, from the default of 0.01, perhaps to 0.02 or
even 0.1. Making this too large can result in the tool being very slow
and perhaps running out of memory.

As for intrinsics, it is suggested that each of the four cameras be
individually calibrated with the IMU to make the process better
behaved. Hence, one first calibrates the intrinsics of one camera (say
nav or haz), then runs extrinsics calibration for it, before switching
to a new camera. (A single bag can be used for both cameras facing the
same direction, or separate bags can be acquired.) The tool will infer
from its input .yaml file if to work with the HD or depth camera.

