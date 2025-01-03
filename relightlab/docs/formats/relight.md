### Relight project file format

RelightLab saves a project in a JSON file (.relight extension), containing the list of images,
lighting directions or posittions and all other attributed required for processing.

## Metadata
| key  | value |
| ---- | ---- |
| application | "ReglightLab", so we can check it is actually from this software |
| version | "RELIGHT_VERSION" |
| created | date of creation |
| authors | |

## images
| key | value |
| --- | --- |
| folder | Images folder relative to he position of the project file |
| width | Width of the images in pixels |
| height | Height of the images in pixels |
| images | Array of image properties |

## image

| key | value |
| --- | --- |
| filename | Filename of the image |
| direction | x, y, z object |
| position | x, y, z object, one of the two needs to be present for processing |
| lightCalibration | see Dome definition. |

## Geometry
| key  | value |
| ---- | ---- |
| diameter | Dome diameter in mm, and yes, metric and only mm. |
| imageWidth | Default width in mm of the area covered by the image, this is useful for domes having a fixed optic. |
| verticalOffset | Default vertical offset from the origin of the coordinated 
(usually the center of the sphere which is the most common geometry for domes). Positive if the center is above the surface. |


## Reflective pheres

| key  | value |
| ---- | ---- |
| border| Array of the points used to fit a circle or an ellipse (in pixels 0,0 is top left corner) |
| lights | Array of coordinates of the reflections, one for each image, [0, 0] if not found |

### Deprecated?
This values can all be simply computed from the border values.

| key | value |
| --- | --- |
| center | [x, y] coordinates of the center of the sphere/ellipse |
| inner | Bounding box of the part of the sphere where light reflections are supposed to be |
| radius | For the sphere fitting |
| majorAxisLenght | For ellipse fitting (in pixels) |
| minorAxisLenght | For ellipse fitting (in pixels) |
| axisAngle | Rotation of the major axis in radians |



## If a dome is specified, it is saved here

| key  | value |
| ---- | ---- |
| dome | Dome json config |

Dome configuration (geometry, LED calibrations etc.) are copied inside here, not to depend on external files.
See [dome format](dome_format.md) specifications.
