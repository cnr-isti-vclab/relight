# An Adaptive Screen-Space Meshing Approach for Normal Integration
### [Project Page](https://moritzheep.github.io/adaptive-screen-meshing/) | [Paper](https://arxiv.org/abs/2409.16907) | [Poster](https://moritzheep.github.io/publication/heep-isotropicmeshing-2024/poster.pdf) | [Video](https://www.youtube.com/watch?v=6m2SKqb1M5M)


[Moritz Heep](https://moritzheep.github.io/),
[Eduard Zell](http://www.eduardzell.com/)

University of Bonn, PhenoRob

# Getting Started
To clone the repository with all its submodules run
```Shell
$ git clone --recursive https://github.com/moritzheep/adaptive-screen-meshing.git
```

## Prerequisites
Our method uses [nvdiffrast](https://github.com/NVlabs/nvdiffrast) to translate between the triangle mesh and the pixel grid. Please make sure that all dependencies of nvdiffrast are met, especially torch. Furthermore, we require [OpenCV](https://opencv.org/) to be installed.
### Docker
We prepared a Docker image to take care of these dependencies and facilitate testing. We still need the nvidia-container-runtime. It can be installed via
```Shell
$ cd docker
$ .\nvidia-container-runtime.sh
```
To build the image, run
```Shell
$ cd docker
$ ./build.sh
```
Finally, run
```Shell
$ docker run \
    --runtime=nvidia \
    --gpus all \
    adaptive-screen-meshing
```
to get a list of options. You can then mount a volume and point towards your input files. All arguments can be appended to the above command and are passed through.
## Building
To build the project, run
```Shell
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Running
After the build has been completed successfully, you can run
```sh
$ src/main \
    -n <path-to-the-normal-map> \
    -m <path-to-the-foreground-mask> \
    -t <path-to-save-the-mesh>
```
for a quick test. The normal map should be in the `.exr` format, the mask can be any 8Bit grayscale format supported by OpenCV. The mesh can be saved to any format supported by the [pmp-library](https://github.com/pmp-library/pmp-library). We recommend `.obj`.

You can run `src/main` to get of full list of all options.

# Troubleshooting
If you get meshes that curve in the wrong direction, try flipping the x or y coordinate of your normal map.

# Citation
```
@inproceedings{heep2024screen-space-meshing,
    title={An Adaptive Screen-Space Meshing Approach for Normal Integration},
    author={Moritz Heep and Eduard Zell},
    booktitle = {European Conference on Computer Vision (ECCV)},
    year={2024}
}
```
