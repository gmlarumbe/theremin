# Overview #

Larumbe's Theremin, based on [OpenTheremin](http://www.gaudi.ch/OpenTheremin/) by Urs Gaudenz.

<img src="http://larumbe.hopto.org/img/theremin_side.jpg" width="400">

# Operation #

Theremin control consists of two metal antennas that sense the relative position of the performer hands. Right hand controls frequency and left hand controls volume.

Functionality and electronics are similar to that of the OpenThereminUNO 1.2 with some added features: additional LC Colpitts oscillators, LEDs to check appropriate output pitch range, input debouncing and filtering, knob for pitch range selection or midi song autoplaying in calibration mode among others.

Schematics and layouts can be found at my personal website: <https://larumbe.hopto.org>.


# Misc #
  * There are some scripts and functions at the MATLAB folder for filtering test purposes.

  * Makefile makes use of [Arduino-Makefile](https://github.com/sudar/Arduino-Makefile) to compile and upload binaries to Arduino by using the command line and Emacs.


