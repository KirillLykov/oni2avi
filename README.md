Oni2Avi
============================

https://github.com/KirillLykov/oni2avi

Overview
--------

Oni2Avi is a command line application converting *.oni files into *.avi.
Oni data file contains images and depth obtained from Kinect. Oni files 
are supported by OpenNI library.

Command line syntax
--------------------------

The following options are available:
--help
--input-file
--output-file
--codec
--depth-png
Input-file must be a valid *.oni file, output-file is always avi file with extension,
codec specifies available codecs (MPEG-1, MPEG-4, MPEG-4.2, MPEG-4.3, FLV1). The default
codec is MPEG-4.2. Option depth-png allows to save depth frames as png images instead of avi file.
The result of the program execution is two avi files for image and depth data 
or, in case of --depth-png=yes, avi file for images and a folder with *.png for depth.

Linux and Mac OS X requirements
--------------------------

In order to build oni2avi using provided makefile, user need to have gcc version 4.6 or newer.
In addition to that, the following libraries must be installed:
* OpenNI
* OpenCV
* boost

Paths to these libraries as well as related headers paths must be specified in the Makefile. 
Note that I checked the code only with OpenNI 1.5.2.23, OpenCV 2.4.5/2.3, boost 1.53, gcc 4.7.
