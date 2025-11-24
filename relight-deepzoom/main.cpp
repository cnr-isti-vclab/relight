#include "../src/deepzoom.h"
#include <iostream>
#include <string>

using namespace std;

PyramidFormat parseFormat(const std::string &value) {
	if(value == "google")
		return PyramidFormat::Google;
	if(value == "zoomify")
		return PyramidFormat::Zoomify;
	if(value == "tiff")
		return PyramidFormat::TiledTiff;
	return PyramidFormat::DeepZoom;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		cerr << "Converts an image to a tiled pyramid format for web viewing." << endl;
		cerr << endl;
		cerr << "Usage: " << argv[0] << " <input.jpg> <output> [format] [tileSize] [overlap] [quality]" << endl;
		cerr << endl;
		cerr << "  format    deepzoom (default), google, zoomify, or tiff" << endl;
		cerr << "  tileSize  tile dimension in pixels (default: 256, must be multiple of 16 for tiff)" << endl;
		cerr << "  overlap   pixel overlap between tiles (default: 1, not used for tiff)" << endl;
		cerr << "  quality   JPEG quality 1-100 (default: 95)" << endl;
		return 0;
	}

	DeepZoom deep;
	PyramidFormat format = PyramidFormat::DeepZoom;
	int tileSize = 256;
	int overlap = 1;
	int quality = 95;
	if(argc >= 4)
		format = parseFormat(argv[3]);
	if(argc >= 5)
		tileSize = atoi(argv[4]);
	if(argc >= 6)
		overlap = atoi(argv[5]);
	if(argc >= 7)
		quality = atoi(argv[6]);
	deep.quality = quality;
	if(!deep.build(argv[1], argv[2], tileSize, overlap, format)) {
		cerr << "Something failed!" << endl;
	}
	return 0;
}
