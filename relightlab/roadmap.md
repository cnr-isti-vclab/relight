# Roadmap for RelightLab.



## Reinstate existing functions:

* lp saving
* normals saving and filtering/flattening
* image scale measuring

## New Features

* Don't freeze on project loading.
* consider eventual modifications to the file format to mirror interface parameters.
* manual alignment
* ellipse reflections geometrically accurate

## Long term new features

* preprocessing files and caching
* TIF/CR2/JPEGXL support
* floating point processing
* shadow and burned pixels filtering
* Assisted/automated alignment
* PBR texture creation

## Bugs

* integrated normal seems to be reverrsed. (might be a y top/bottom problem).
* Find highlights crashes if quitting early.
* Accessing the crop tab before loading the project causes it to stop working.

## Small Bugs

* Check version is properly written and read from the .relight file