#include "deepzoom.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

#include <assert.h>
using namespace std;

vector<uint8_t>  scaleLines( std::vector<uint8_t> &line0, std::vector<uint8_t> &line1) {
	assert(line0.size() == line1.size());
	std::vector<uint8_t> scaled;

	int w = line0.size() >> 1;
	scaled.resize(w);
	for(int i = 0; i < line0.size(); i += 2) {
		int c = (int)line0[i] + (int)line1[i];
		if(i == line0.size()-1) { //odd
			c /= 2;
		} else {
			c += (int)line0[i+1] + (int)line1[i+1];
			c /= 4;
		}
		scaled[i/2] = (uint8_t)c;
	}
	return scaled;
}
TileRow::TileRow(int _tileside, fs::path _path, int _width, int _height) {
	tileside = _tileside;
	path = _path;
	width = _width;
	height = _height;
}


std::vector<uint8_t> TileRow::addLine(std::vector<uint8_t> &newline) {
	vector<uint8_t> scaled;
	if(current_line % 2) {
		scaled = scaleLines(lastLine, newline);
		//merge lines.
	}
	//write line to jpeg
	int x = 0;
	for(Tile &tile: *this) {
		tile.encoder->writeRows(newline.data() + x, 1);
		x += tile.width;
	}
	current_line++;
	if(current_line == height) {
		//save tiles and init new tiles.
	}


	std::swap(lastLine, newline);
	return scaled;
}


bool DeepZoom::build(const std::string &filename, const string &_folder, int _tileside, int _overlap) {
	folder = _folder;
	tileside = _tileside;
	overlap = _overlap;

	//create folder filename-ext_files and text file

	JpegDecoder decoder;
	bool ok = decoder.init(filename.c_str(), width, height);
	if(!ok) return false;

	initRows();

	//for each level
	//line by line
	std::vector<uint8_t> line(width);
	for(int y = 0; y < height; y++) {
		decoder.readRows(1, line.data());
		for(int i = 0; i < rows.size(); i++) {
			std::vector<uint8_t> scaled = rows[i].addLine(line);
			if(!scaled.size())
				break;
		}


	}

	//define current row. and all its parents.


	return true;
}

int DeepZoom::nLevels() {
	int level = 0;
	int w = width;
	int h = height;
	while(w > tileside && h > tileside) {
		w >>= 1;
		h >>= 1;
		level++;
	}
	return level;
}

void DeepZoom::initRows() {
	fs::path path(folder);
	int w = width;
	int h = height;
	int level = nLevels()-1;
	while(w > tileside && h > tileside) {
		//create folder.
		fs::path level_path = path / to_string(level);
		fs::create_directory(level_path);

		TileRow row(tileside, level_path, w, h);
		rows.push_back(row);

		widths.push_back(w);
		heights.push_back(h);

		w >>= 1;
		h >>= 1;
		level--;
	}
	assert(level == -1);
}

void TileRow::finishRow() {
	for(Tile &tile: *this) {
		tile.encoder->finish();
		delete tile.encoder;
	}
	clear();
}

void TileRow::nextRow() {
	current_row++;
	int x = 0;
	int col = 0;
	do {

		Tile tile;
		tile.width = std::min(tileside, width - x);
		tile.encoder = new JpegEncoder;
		fs::path filepath = path / (to_string(col) + "_" + to_string(current_row) + ".jpg");
		tile.encoder->init(filepath.c_str(), tile.width, height);
		push_back(tile);

		x += tileside;
		col++;

	} while(x < width);
}
