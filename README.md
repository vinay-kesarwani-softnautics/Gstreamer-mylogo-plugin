# GStreamer Custom Plugin: logo plugin

## Objective:

Develop a GStreamer plugin that imposes a logo onto a video stream in NV12 format while supporting a maximum resolution of 1080p and 60 fps. The plugin will also offer animation options, allowing the user to rotate the logo clockwise or counterclockwise and scroll the logo vertically or horizontally at variable speeds.

Phase-1:

Develop a GStreamer plugin that imposes a logo onto a video stream in NV12 format while supporting a maximum resolution of 1080p and 60 fps. Provide an option for the user to specify the location of the PNG logo file on the local file system. Allow the user to specify the coordinates for placing the logo on the video frame.

The plugin should also offer below animation option:Provide options for clockwise and counterclockwise rotation.  Allow the user to set the rotation speed as 'slow,' 'medium,' 'fast,' or with any other relevant arguments.

Phase-2:

Add alpha blending support:plugin imposes the logo with transparency. Transparency level should be user configurable.

Add below additional option in animation:Support scrolling of the logo in both horizontal and vertical directions. Allow the user to specify the direction (up, down, left, right).

*****Steps to run the custom plugin*****

## Compilation Steps:
$ meson build
$ ninja -C build

## Loading the Plugin:
$ sudo ninja -C build install
$ export GST_PLUGIN_PATH=/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0

## Verify Plugin: 
-After the plugin is loaded, you can inspect it using the following command:
$ gst-inspect-1.0 mylogo

## Using the Plugin:

## usage
Once the gstreamer plugin is loaded and installed you can use it in gstreamer pipeline with the element name as 'mylogo'.
-To use the mylogo plugin in your GStreamer pipeline, you have the following options:

1.Default Configuration:
    -The default logo_x and logo_y coordinates are set to 100 and 100 respectively with respect to video frame with default rotation_direction to clockwise(1).
    $ gst-launch-1.0 videotestsrc ! video/x-raw, format=NV12, width=1920, height=1080, framerate=60/1 ! mylogo ! autovideosink

2.Custom Configuration:
    -You can customize the configuration of mylogo plugin by modifying its properties such as logo_x, logo_y, rotation_angle, rotation_speed, alpha, etc.
    -First, set logo_x and logo_y coordinates and accordingly other properties with specific values as you needed.
    example-
    1. $ gst-launch-1.0 videotestsrc ! video/x-raw, format=NV12, width=1920, height=1080, framerate=24/1 ! mylogo logo_x=800 logo_y=500 rotation_direction=-1 rotation_angle=40 alpha=0.7 ! autovideosink
    
    2. $ gst-launch-1.0 videotestsrc ! video/x-raw, format=NV12, width=1920, height=1080, framerate=24/1 ! mylogo logo_x=800 logo_y=500 scroll_direction=down scroll_speed=20 ! autovideosink
