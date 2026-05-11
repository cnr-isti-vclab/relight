Honest Gem
Gem personalizzato
This comprehensive summary defines the mathematical and technical architecture for your multiplatform RTI (Reflectance Transformation Imaging) software. It is organized into actionable development phases for a student to implement.

Technical Overview
The software aims to produce high-precision PTM/HSH files by decoupling geometric (lens), radiometric (LED/sensor), and photometric (light direction) calibrations.

Phase 1: Geometric Calibration (The Lens)
Goal: Map distorted sensor coordinates (u,v) to "perfect" pinhole coordinates (x,y).

Distortion Model: Use the Brown-Conrady model, accounting for radial (k 
1
​
 ,k 
2
​
 ,k 
3
​
 ) and tangential (p 
1
​
 ,p 
2
​
 ) distortion.

Fitting Process:

Input: 5–10 RAW/TIFF images of a checkerboard at various orientations.

Logic: Use cv::findChessboardCornersSB for sub-pixel accuracy. Run cv::calibrateCamera to find the Intrinsic Matrix (K) and Distortion Coefficients (D).

Fallback: If no dataset is provided, attempt to load coefficients from a lens.json (user-provided or EXIF-matched).

Application: During pre-processing, use cv::initUndistortRectifyMap once and cv::remap for every frame.

Phase 2: Photometric Calibration (The Lights)
Goal: Determine the vector  
L

  
i
​
  for every light source in the dome.

Detection Method:

Reflective Spheres: Detect the specular highlight (blob) center on one or two glossy spheres.

Ray Casting: Use the sphere’s center and radius in image space (plus camera focal length) to calculate the 3D reflection vector.

Output: A lights.json containing a list of (x,y,z) unit vectors for all LEDs.

Phase 3: Radiometric Calibration (The Gain)
Goal: Correct the Per-Light Gain Map (G 
i
​
 ), which is the product of lens vignetting and the LED’s unique illumination cone.

Data Ingestion:

Input: One RAW image per LED of a uniform neutral grey card.

Linearization: Extract raw Bayer data using LibRaw (no gamma, no WB).

Fitting the Cone:

Smoothing: Apply a heavy Gaussian Blur to the grey card images to eliminate sensor noise and local dust, leaving only the "light shape."

Normalization: Divide the blurred image by its own maximum value (or the global peak) to create a multiplier map where 1.0 is the peak intensity.

Approximation: To save memory, fit the resulting map to a 4th-order 2D Polynomial or a low-resolution grid (e.g., 32×32 nodes). This ignores 1/r 
2
  (handled mathematically later) and focuses on the cone shape and vignetting.

Output: cones.json containing polynomial coefficients for each LED.

Phase 4: The Execution Pipeline (Actionable Steps)
This is the main loop that converts source RAWs into fit-ready data.

Step 4.1: Linearization & Peak Discovery
Histogram Analysis: Perform a fast pass over all RAW images (using LibRaw's half-size mode).

User Feedback: Display a histogram. Ask the user to define the "White Point" or use the 99th percentile to avoid clipping.

Global Peak: Identify the single highest intensity value across the entire dataset to use as a normalization constant (Max 
global
​
 ).

Step 4.2: Radiometric Correction
For every image i in the set:

Develop RAW: Load via LibRaw with linear settings (Gamma 1.0).

Apply Gain: Multiply the image by the LED-specific gain map G 
i
​
  from cones.json.

Normalize: Divide by Max 
global
​
  found in Step 4.1. This ensures all images are balanced relative to each other.

Step 4.3: Geometric Correction
Remap: Use the cv::remap function with the lens.json parameters to move pixels to their undistorted locations.

Interpolation: Use INTER_LANCZOS4 or INTER_CUBIC to maintain sharpness during the move.

Step 4.4: Image Enhancement
Unsharp Mask: (Optional) Apply Sharpened = Original + Amount * (Original - Blurred) to recover micro-contrast lost during demosaicing and remapping.

Format Conversion: Save the resulting RGB data as a Tiled 16-bit Float EXR.


Interface:

Dome calibration comes in few steps: distortion, light positions, flatfielding.

Distortion depend on lens and zoom, flatfielding depends on vignetting+leds positions and led characteristics.
This means you cannot really reuse them if you change zoom.
But the user might want to reuse the 3d positions of the lights (will need to input image width)
We will allow the user to save:

1) the entire dome
2) the distorsion parameters
3) the flatgielding
4) the 3d positions of the light.

For the calibration window let's use the same tabbed interface as in mainwindow.cpp

1) images
2) histogram (allows user to change the max and see which pixels will be considered burned)
3) distortion (image or from lens and lensfun database)
4) flatfield (if dataset is available)
5) light directions lp + dome radius + offset( if the user want to use reflective spheres we might want to reuse the existing sphere tab)