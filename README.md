
**Relight** is a library to create and view relightable images (RTI).

## [Demo](https://vcg.isti.cnr.it/relight/)

## Features

Relight supports:

* [Relight](#Relight fitter) a PCA based format (see paper)
* [PTM, HSH](#PTM) 
* [Web viewer](#viewer)
* [Zoomify, deepzoom, google, IIP, IIIF](#lod)

Relight new formats provide better accuracy and smaller size.

# Relight fitter

*relight-cli* is a Qt command-line program to process a stack of photos into an RTI.

	Usage: relight-cli [-frqpsScCey]<input folder> [output folder]

	input folder containing a .lp with number of photos and light directions
	optional output folder (default is ./)

	-b <basis>: rbf(default), ptm, lptm, hsh, yrbf, bilinear
	-p <int>  : number of planes (default: 9)
	-y <int>  : number of Y planes in YCC
	-r <int>  : side of the basis bilinear grid (default 8)
	-q <int>  : jpeg quality (default: 90)
	-s <int>  : sampling rate for pca (default 4)
	-S <int>  : sigma in rgf gaussian interpolation default 0.125 (~100 img)
	-C        : apply chroma subsampling 
	-e        : evaluate reconstruction error (default: false)

*relight-cli* can also be used to convert .ptm files into relight format:

	relight-cli [-q]<file.ptm> [output folder]
	-q <int>  : jpeg quality (default: 90)


# Relight web format

Relight format is a directory containing an *info.json* file and a few images.

Json contains the following fields:

* width:
* height:
* format: image extension: .jpg, .png etc.
* type: one of lptm, hsh, rbf, bilinear ycc
* nplanes: numer of coefficient planes
* resolution: for bilinear types the side of the bilinear grid
* colorspace: one of lrgb, rgb, ycc, mrgb, mycc
* sigma: rbf interpolation parameter
* lights: mandatory for rbf interpolation
* materials: an array of materials, each material specify scale, bias and range
* scale: an array of floats
* bias: an array of floats
* range: an array of floats
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

[libvips](http://jcupitt.github.io/libvips/supported/current) can be used to [generate deepzoom, zoomify and google pyramidal formats](http://libvips.blogspot.com/2013/03/making-deepzoom-zoomify-and-google-maps.html), scripts can be found in the directory *test*.

# Web Viewer

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

## Building relight

### Debian Linux

```
$ uname -a
Linux x220 5.10.0-3-amd64 #1 SMP Debian 5.10.13-1 (2021-02-06) x86_64 GNU/Linux

$ cat /etc/os-release 
PRETTY_NAME="Debian GNU/Linux bullseye/sid"
NAME="Debian GNU/Linux"
ID=debian
HOME_URL="https://www.debian.org/"
SUPPORT_URL="https://www.debian.org/support"
BUG_REPORT_URL="https://bugs.debian.org/"
```

Install dependencies. (replace libjpeg62-turbo0dev with libturbojpeg-dev in Ubuntu 2020)

```shell
$ apt update && apt install \
    build-essential \
    cmake \
    git \
    qtbase5-dev \
    libeigen3-dev \
    libjpeg62-turbo-dev \
    libomp-dev
```

Clone this repository and build.

```shell
$ git clone https://github.com/cnr-isti-vclab/relight.git
$ cd relight
$ git submodule update --init --recursive
$ cmake .
$ make
```
    

### MacOS

Installed tools: 
* Homebrew, CMake, Qt6

Add homebrew binaries on your PATH: 
```shell
echo 'PATH="/opt/homebrew/bin:$PATH"' >> ./zshrc
source ~/.zshrc
```

Add some additional enviroment constants to your shell
```shell
echo "$(brew shellenv)" >> ./zshrc
source ~/.zshrc
```

Install dependencies: 
```shell
brew install jpeg-turbo coreutils llvm cmake ninja eigen 
```

Set the Qt_DIR to point to your Qt6 installation and libomp path: 
```shell
export OPENMP_PATH=$(brew --prefix libomp)
export cmake -D OpenMP_ROOT=$OPENMP_PATH -D Qt6_DIR=/yourpath/Qt/version/macos/lib/cmake/Qt6
make
```


# TODO

* White balance and other conversion from RAW features (dcraw)
* use color tablet to calibrate raw images
* contrast and other image processing
* measure to be added to the images from tag
* remove lens distortion
* find spheres (very optional)
* mask artifact
* crop
* join pieces
* align images using mutual information (or better an edge detector?)
* find highlight

