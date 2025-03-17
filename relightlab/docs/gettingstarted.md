# Getting Started with RelightLab

## Index
- [Introduction](#introduction)
- [Capturing and Preparing the Dataset](#capturing-and-preparing-the-dataset)
- [Creating a New Project](#creating-a-new-project)
- [Saving a Project](#saving-a-project)
- [Loading an Existing Project](#loading-an-existing-project)
- [Checking Image Alignment](#checking-image-alignment)
- [Establishing a Scale](#establishing-a-scale)
- [Finding Light Directions](#finding-light-directions)
- [Selecting a Crop Region](#selecting-a-crop-region)
- [Exporting RTI](#exporting-rti)
- [Managing Processing in the Queue](#managing-processing-in-the-queue)

## Introduction
RelightLab is an application for creating relightable images and performing basic photostereo processing from a set of photos. This guide will help you get started with creating, saving, loading projects, and processing images.

## Capturing and Preparing the Dataset
For guidance on how to properly capture and prepare your dataset, refer to the [**Cultural Heritage Imaging Foundation**](https://culturalheritageimaging.org/) resources. They provide best practices for image acquisition, lighting setups, and data preparation.

## Creating a New Project
To create a new project, click the "New project..." button and simply select a folder containing the set of photos. The application will load the images and prepare them for processing.

## Saving a Project
To save your project:
- Go to **Menu → File → Save...**
- A `.relight` JSON file will be saved.
- It is recommended to save the file in the same folder as the dataset or in the folder above, so the dataset can be moved easily with the project.

## Loading an Existing Project
To load a previously saved project, select the `.relight` file, and the application will restore the project state.

## Checking Image Alignment
1. Create a new alignment.
2. Select a small region of the image.
3. Wait for the patches to load.
4. Click the **Verify** button to visually inspect the alignment. 
if the images are not properly aligned the RTI will be blurry.
5. If necessary, adjust alignment by dragging the marker.

## Establishing a Scale
1. Click the "Set new scale" button.
2. Click on the edges of a segment. 
3. Specify the real-world dimension in millimeters.

This enables accurate measurements in the viewers and can be used to establish scene geometry.

## Finding Light Directions
Each image corresponds to a different lighting direction represented as a vector. There are two ways to determine these directions:

- **Using a Light Position File**: If a dome was used, a `.lp` text file with the ordered light directions is usually available. (See `.lp` documentation.)
- **Using Reflective Spheres**:
  1. Create a new sphere.
  2. Click on the sphere's border in the image (at least 3 points for a sphere, 5 for an ellipse, which may be needed for wide-angle lenses).
  3. Confirm selection.
  4. The application will auto-detect reflections.
  5. Once completed, verify and adjust if necessary. See [reflective sphere documentation](interface/spheres.md)

## Selecting a Crop Region
You can define a crop region in the **Crop** tab to process only a specific part of the image instead of the entire dataset.

## Exporting RTI
In the **Export RTI** tab, you can:
1. Choose the RTI type (e.g., [PTM, HSH, etc.](rti/basis.md)).
2. Set quality and format parameters.
3. Export in a [web-friendly image layout](rti.web.md) 
4. Specify the folder name where the RTI will be saved. 

## Managing Processing in the Queue
- The **Queue** tab shows processing progress.
- Once finished, click the **Cast** icon to view the RTI in a browser.
- You can open the folder to inspect images, useful for further processing.
- If publishing the dataset on a web server, consider using pyramidal formats such as:
  - **DeepZoom**
  - **TarZoom**
  - **ITarZoom**
  These formats optimize large images using a Google Maps-like approach for responsive interfaces and reduced data transfer. (See web format documentation.)


