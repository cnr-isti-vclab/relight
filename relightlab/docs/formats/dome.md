# Dome configuration file format

Dome configuration is saved in a JSON file (.dome extension) and contains all the informations
related to geometry and calibration of the lights, dimensions, offsets etc.

| key | value |
| --- | --- |
| path | path and filename of the dome config |
| diameter | Dome diameter in cm, and yes, metric and only cm. |
| imageWidth | Default width in cm of the area covered by the image, this is useful for domes having a fixed optic. |
| verticalOffset | Default vertical offset from the origin of the coordinated 
(usually the center of the sphere which is the most common geometry for domes). Positive if the center is above the surface. |
| positions | Array of light positions, in cm, where z is up, x is right and y is positive going toward the top of the image, optional |
| directions | Array of light directions, a reasonable approximation if the imageWidth is small relative to the diameter. Either positions or directions needs to be defined. |
| lightsCalibration | Array. Leds can have different colors, intensity and even non uniform emissions, hence the need for calibration |

## Lights calibration

Each element in this array is defined as such:
Spatial calibration is a grid sampling of the light cone intensity of each led, 
centered on the coordinate origin (the center of the sphere dome, or [0,0,0] 
when each led is assigned a position. (not usable id directions only are given).
Width and height define the size of the grid in cm, rows and cols the number of 
divisions in the grid, number of coefficients is cols*rows and the values.
The distance between the samples on the x axis is width/(cols-1).

Spatial calibration includes the distance factor; if no spatial calibration 
is given the 1/d^2 formula is to be used, with d distance from the light to the point.

The formula is:

R = R * whiteBalance.r * bilinearInterpolation(spatialCalibration.intensity)

or 

R = R * whiteBalance.r * intensity*(1/d^2)

| key | value |
| --- | --- |
| whiteBalance | RGB correction coefficients { r: 1.0, g:1.0, b:1.0 } |
| intensity | Intensity correction coefficient. Each component is multiplied by this factor. |
| spatialCalibration | { width: 8.5, height: 6.1, rows: 8, cols: 7, intensity: [ 1.0, 0.9, 0.8... ] } intensity correction values on a grid wxh, the 0, 0 is a the top left corner of image |


