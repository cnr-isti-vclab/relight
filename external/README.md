# External Dependencies

This directory contains third-party libraries used by Relight.

## Current Dependencies

- **eigen-3.3.9**: Matrix/linear algebra library (Header-only)
- **libjpeg-turbo-2.0.6**: High-performance JPEG library 
- **tiff-4.7.0**: TIFF image format library

## Notes

These libraries are included as source code for easier building across platforms.
Consider using system packages when available for better maintenance.

## Recommendations

- For Linux distributions: Use system packages via package manager
- For Windows/macOS: Keep bundled versions for easier building
- Consider migrating to modern package managers (vcpkg, Conan) in the future