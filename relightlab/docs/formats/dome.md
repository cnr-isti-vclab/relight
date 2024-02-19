### Dome configuration file format

Dome configuration is saved in a JSON file (.dome extension) and contains all the informations
related to geometry and calibration of the lights, dimensions, offsets etc.

| key | value |
| --- | --- |
| diameter | Dome diameter in cm, and yes, metric and only cm. |
| imageWidth | Default width in cm of the area covered by the image, this is useful for domes having a fixed optic. |
| verticalOffset | Default vertical offset from the origin of the coordinated 
(usually the center of the sphere which is the most common geometry for domes). Positive if the center is above the surface. |
| positions | Array of light positions, in cm, where z is up, x is right and y is positive going toward the top of the image, optional |
| directions | Array of light directions, a reasonable approximation if the imageWidth is small relative to the diameter. Either positions or directions needs to be defined. |
| ledCalibration | To be defined: led can have different colors, intensity and even non uniform emissions, hence the need for calibration |


