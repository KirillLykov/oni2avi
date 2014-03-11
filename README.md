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

* --help
* --input-file
* --output-file
* --codec
* --depth-png

Input-file must be a valid *.oni file, output-file is always avi file with extension,
codec might be MPEG-1, MPEG-4, MPEG-4.2, MPEG-4.3, FLV1. The default
codec is MPEG-4.2. Option depth-png allows to save depth frames as png images instead of avi file.
The result of the program execution is two avi files for image and depth data 
or, in case of --depth-png=yes, avi file for images and a folder with *.png for depth.

Requirements & installation
--------------------------

The following libraries must be installed in order to build oni2avi:
* OpenNI 1.x
* OpenCV
* Boost

Paths to these libraries as well as related headers paths must be specified in the Makefile. 
Alternatively, it's possible to use CMake to build oni2avi. In case all the necessary libraries
are installed, CMake will find them automatically.

Note that I checked the code only with OpenNI 1.5.2.23, OpenCV 2.4.5/2.3, boost 1.53, gcc 4.7,
under MacOS > 10.6 and Ubuntu > 12.

Building with make (Linux, Mac OS X)
--------------------------

```bash
git clone git://github.com/KirillLykov/oni2avi.git
cd oni2avi
make
```

Building with CMake (Windows, Linux, Mac OS X)
--------------------------

```bash
git clone git://github.com/KirillLykov/oni2avi.git
cd oni2avi
mkdir build && cd build
cmake ../
make
```