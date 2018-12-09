# RPi-BaslerOpencv


Using a RPi, you can control a Basler camera by using its SDK in C / C ++ and with a SIM808 GPS development card, generate image metadata, such as the exact date and time, in addition to the georeferencing of the image.
By using OpenCV, it allows versatility in the manipulation and editing of the image.
Base code is added to obtain images with Basler cameras through SDK and basic OpenCV operations for editing and generating images.
The code manages to save tagged images with their respective date and time, in addition to a json file that adds the binary sums of each photo, model of the camera and georeferenced location of the same.


## Install Basler SDK Suite & OpenCV

Prerequisites

```
Install_bash.sh (by adding)
```

## for the installation of the repo

```
git clone https://github.com/LeerySpice/RPi-BaslerOpencv /home/pi/BASLER/
```

Change string "ModelCamera" according to camera to use, line 63, BASLER-EXEC.cpp

```
cd /home/pi/BASLER/
sudo make -j4
```

Later, create the service BASLER.service according to the notes of the wiki
