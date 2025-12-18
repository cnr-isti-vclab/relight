
**Relight** is a library and set of tools to create and view relightable images (RTI).

## [Demo](https://vcg.isti.cnr.it/relight/)

## Features

Relight supports:

* [RelightLab](#relightlab) - GUI application for creating RTIs and normal maps
* [Relight CLI](#relight-cli) - Command-line RTI processing tool with PCA based format
* [PTM, HSH](#ptm-hsh) - Standard RTI formats
* [OpenLIME](#openlime-web-viewer) - Modern web viewer for RTIs (recommended)
* [Zoomify, deepzoom, google, IIP, IIIF](#relight-tiled-web-format) - Tiled format support
* [Relight.js](#relightjs-legacy-viewer) - Legacy JavaScript viewer (obsolete)

Relight new formats provide better accuracy and smaller size.

# RelightLab

*RelightLab* is a Qt GUI application for creating relightable images (RTI) and performing photometric stereo processing from a set of photographs.

## Key Features

* **Project Management**: Create, save, and load projects with datasets
* **Light Direction Detection**: 
  - Import light positions from `.lp` or `.dome` files
  - Auto-detect light directions using reflective spheres
* **Multiple Output Formats**: Export RTI in various formats (PTM, HSH, RBF, etc.)
* **Normal Map Extraction**: Generate normal maps from the image stack
* **Scale Calibration**: Establish real-world measurements for accurate analysis
* **Crop Region Selection**: Process only specific areas of interest
* **Web Publishing**: Export in web-friendly formats (DeepZoom, TarZoom, IIIF)
* **Queue Management**: Monitor and manage processing tasks


# Relight CLI

*relight-cli* is a Qt command-line program to process a stack of photos into an RTI.

## Usage

```shell
relight-cli [-bpqy3PnmMwkrsSRQcCeEv] <input folder> [output folder]
relight-cli [-q] <input.ptm|.rti> [output folder]
relight-cli [-q] <input.json> [output.ptm]
```

## Basic Options

* **Input**: Folder containing a `.lp` or `.dome` file with photo count and light directions
* **Output**: Optional output folder (default: `./`)

### Core Parameters

* `-b <basis>`: Basis type - `rbf` (default), `ptm`, `lptm`, `hsh`, `sh`, `h`, `yrbf`, `bilinear`, `yptm`, `yhsh`, `dmd`, `skip`
* `-p <int>`: Number of coefficient planes (default: 9)
* `-q <int>`: JPEG quality (default: 95)
* `-y <int>`: Number of Y planes in YCC colorspace

### Processing Options

* `-3 <radius[:offset]>`: 3D light positions processing
  - `radius`: Ratio of dome diameter to image width
  - `offset`: Optional vertical offset of sphere center to surface
* `-P <pixel_size>`: Pixel size in millimeters (saved in JSON and image metadata)
* `-n`: Extract normal maps
* `-m`: Extract mean image
* `-M`: Extract median image (7/8th quantile)
* `-w <int>`: Number of worker threads (default: 8)
* `-k <W>x<H>+<X>+<Y>`: Crop region (width×height+offsetX+offsetY)

### Advanced Options

* `-H`: Fix overexposure in PTM and HSH due to bad sampling
* `-r <int>`: Side of the basis function grid (default: 8, 0 means RBF interpolation)
* `-s <int>`: RAM sampling for PCA in MB (default: 500MB)
* `-S <float>`: Sigma for RGF Gaussian interpolation (default: 0.125, ~100 images)
* `-R <float>`: Regularization coefficient for bilinear (default: 0.1)
* `-Q <float>`: Quantile for histogram-based range compression (default: 0.995)
  - Clamps outliers to improve quantization resolution
* `-c <float>`: Coefficient quantization (default: 1.5)
* `-C`: Apply chroma subsampling
* `-I <mode>`: ICC color profile handling
  - `preserve` (default): Keep original color profile
  - `srgb`: Convert to sRGB
  - `displayp3`: Convert to Display P3
* `-e`: Evaluate reconstruction error
* `-E <int>`: Evaluate error on specific image (excludes it from fitting)

### Testing Options

* `-D <path>`: Directory to store rebuilt images
* `-L <x:y:z>`: Reconstruct single image from light parameters
* `-v`: Verbose mode - print progress information

## Format Conversion

Convert PTM/RTI files to relight format:

```shell
relight-cli [-q] <file.ptm> [output folder]
```

Convert relight format to PTM:

```shell
relight-cli [-q] <input.json> [output.ptm]
```

## Examples

```shell
# Basic RTI creation with default RBF basis
relight-cli ./photos ./output

# High-quality PTM with 18 planes
relight-cli -b ptm -p 18 -q 98 ./photos ./output

# Extract normals and mean image
relight-cli -n -m ./photos ./output

# Process with cropping and custom quality
relight-cli -k 1024x768+100+50 -q 90 ./photos ./output

# 3D light processing with dome setup
relight-cli -3 2.5:0.1 -b bilinear ./photos ./output
```


# Relight web format

Relight format is a directory containing an *info.json* file and a few images.

Json contains the following fields:

* width:
* height:
* format: image extension: .jpg, .png etc.
* type: one of lptm, hsh, rbf, bilinear ycc
* nplanes: number of coefficient planes
* resolution: for bilinear types the side of the bilinear grid
* colorspace: one of lrgb, rgb, ycc, mrgb, mycc
* sigma: rbf interpolation parameter
* lights: mandatory for rbf interpolation
* materials: an array of materials, each material specify scale, bias and range
* scale: an array of floating point values (float)
* bias: an array of floating point values
* range: an array of floating point values
* basis: an array of unsigned chars containing the basis for rbf, bilinear and ycc basis.

Each image contains 3 coefficient planes
* PTM: r, g, b, 1, u, v, u^2, uv, v^2
* HSH: see source code :)
* RBF: PCA basis coefficients
* BILINEAR: PCA basis coefficients
* YCC: PCA basis coefficients as Y<sub>0</sub>C<sub>0</sub>C<sub>0</sub>, Y<sub>1</sub>C<sub>1</sub>C<sub>1</sub>... Y<sub>k</sub>C<sub>k</sub>C<sub>k</sub>, Y<sub>k+1</sub>Y<sub>k+2</sub>Y<sub>k+3</sub>

Scale and bias will be applied to the texture coefficients (from [0, 1]) as:

	(c - bias)*scale

# Relight tiled web format

*relight.js* support a variety of tiled formats

* [Deepzoom](https://www.microsoft.com/silverlight/deep-zoom/)
* [Zoomify](http://www.zoomify.com/)
* [Google maps](https://developers.google.com/maps/)
* [IIP](https://iipimage.sourceforge.io/)
* [IIIF](http://iiif.io)

[libvips](https://github.com/libvips/libvips/) can be used to [generate deepzoom, zoomify and google pyramidal formats](http://libvips.blogspot.com/2013/03/making-deepzoom-zoomify-and-google-maps.html), scripts can be found in the directory *test*.

The native `DeepZoom` generator is now able to emit multiple layouts without relying on the Python/libvips helper scripts. Run `deepzoom/main.cpp` (built as `relight-deepzoom`) with:

```
relight-deepzoom <input.jpg> <basename> [format=deepzoom] [tileSize=254] [overlap=1]
```

Supported `format` values are `deepzoom`, `google`, `zoomify`, and `tiff`. The `tiff` option writes a tiled multi-resolution TIFF pyramid, while the others create JPEG tiles in the expected folder layouts (including Zoomify `ImageProperties.xml`).

# OpenLIME Web Viewer

[**OpenLIME**](https://github.com/cnr-isti-vclab/openlime) is the recommended modern web viewer for displaying RTIs and other relightable images in the browser. It provides a powerful, feature-rich interface with support for multiple image formats and interactive visualization.

## Features

* **RTI Support**: Display all relight formats (RBF, PTM, HSH, etc.)
* **Multi-resolution**: Seamless support for tiled formats (DeepZoom, IIIF, etc.)
* **Interactive Controls**: Light direction manipulation, zoom, pan
* **Annotations**: Add measurements, markers, and annotations
* **Modern Architecture**: Built with ES6 modules and WebGL
* **Extensible**: Plugin system for custom functionality

## Usage

```javascript
import { Viewer, UIBasic } from 'openlime';

const viewer = new Viewer('#viewer-container');
const layer = viewer.addLayer('rti', { 
    url: 'path/to/rti/info.json',
    layout: 'image' // or 'deepzoom', 'iiif', etc.
});

const ui = new UIBasic(viewer);
```

For detailed documentation and examples, visit the [OpenLIME repository](https://github.com/cnr-isti-vclab/openlime).

# Relight.js (Legacy Viewer)

> **Note**: This viewer is now obsolete. Please use [OpenLIME](#openlime-web-viewer) for new projects.

*relight.min.js* is a small Javascript library to render the RTI on a WebGL canvas.

	var relight = new Relight(canvas, options);

Options:
* url: path to a directory containing the web format
* layout: deepzoom, zoomify, google, iip, iiif or image for the web format
* server: server URL for use with iip or iiif
* stack: true or false - whether image is an image stack (IIP only)
* light: initial light, array x, y, z.
* pos: initial view object { x: y: z:}
* border: for tiled formats amount of prefetching tiles arount the view (default 1)
* fit: scale and center the view on load


Members:
* pos: the position of the view as {x: y: z: t}
 where 
  x and y are the coords of the center of the screen in full scale image.
  z is the zoom level with 0 being the original image.
  t is for interpolation in ms
* light: array of x, y, z light direction
* onload: function to be called when rendering is ready

Methods:
* setUrl(url): change url
* resize(width, height): resize canvas
* zoom(amount, dt): 
* pan(dx, dy, dt): change the center of the view
* setPosition(x, y, z, dt):
* center(dt): center view (but does not change zoom
* centerAndScale(dt): fit the view to the canvas
* setLight(x, y, z): change light direction
* draw(time): draw the canvas, use time for interpolation
* redraw(): schedule an animaterequest

## Building Relight

### Prerequisites

Clone the repository:

```shell
git clone https://github.com/cnr-isti-vclab/relight.git
cd relight
git submodule update --init --recursive
```

### Linux (Ubuntu 22.04+)

Install dependencies:

```shell
sudo apt-get update
sudo apt-get install -y mesa-common-dev libglu1-mesa-dev 
sudo apt-get install -y cmake ninja-build patchelf fuse libjpeg-dev libeigen3-dev
sudo apt-get install -y libxcb-cursor0 liblcms2-dev
sudo apt-get install -y qt6-base-dev

```

Build:

```shell
mkdir -p build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
make -j 8
```

### macOS

Install dependencies via Homebrew:

```shell
brew install coreutils libomp eigen libjpeg cmake ninja qt@6
```

Build:

```shell
mkdir -p build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DOpenMP_ROOT=$(brew --prefix libomp)
make -j 8
```

**Note**: If Qt is not found automatically, you may need to specify `Qt6_DIR`:
```shell
cmake ../ -DCMAKE_BUILD_TYPE=Release \
  -DOpenMP_ROOT=$(brew --prefix libomp) \
  -DQt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6
```

### Windows

Install dependencies:

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with C++ support
2. Install [CMake](https://cmake.org/download/)
3. Install [Ninja](https://ninja-build.org/)
4. Install Qt 6.6 or later from [qt.io](https://www.qt.io/download)

Install vcpkg dependencies:

```shell
C:\vcpkg\vcpkg.exe install lcms:x64-windows
```

Build (from Developer Command Prompt):

```shell
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

**Note**: You may need to specify `Qt6_DIR` if Qt is not found automatically:
```shell
cmake ../ -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DQt6_DIR=C:\Qt\6.6.0\msvc2019_64\lib\cmake\Qt6
```

