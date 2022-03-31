#include "deepzoom.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

#include <iostream>
#include <fstream>

#include <assert.h>
using namespace std;

/* start of tiles is 0, side-overlap,, 2*side - overlap etc
 * sizes are         side+overlap, side+2*overla, whatever remains
 */

vector<uint8_t>  TileRow::scaleLines( std::vector<uint8_t> &line0, std::vector<uint8_t> &line1) {
	std::vector<uint8_t> scaled;

	int w = width >> 1;
	scaled.resize(w*3);
	for(int i = 0; i < w; i++) {
		for(int k = 0; k < 3; k++) {
			int j = i*6;
			int c = (int)line0[j+k] + (int)line1[j+k] + (int)line0[j+3+k] + (int)line1[j+3+k];
			c /= 4;
			scaled[i*3+k] = (uint8_t)c;
		}
	}
	return scaled;
}

TileRow::TileRow(int _tileside, int _overlap, fs::path _path, int _width, int _height) {
	tileside = _tileside;
	overlap = _overlap;
	path = _path;
	width = _width;
	height = _height;
	end_tile = 0;
	nextRow();
}

void TileRow::writeLine(std::vector<uint8_t> newline) {
	//write line to jpeg
	int col = 0;
	for(Tile &tile: *this) {
		int x = std::max(0, col*tileside - overlap);
		tile.encoder->writeRows(newline.data() + x*3, 1);
		col++;
	}
}
std::vector<uint8_t> TileRow::addLine(std::vector<uint8_t> newline) {

	vector<uint8_t> scaled;
	if(current_line % 2) {
		scaled = scaleLines(lastLine, newline);
	}

	writeLine(newline);

	current_line++;

	if(current_line > end_tile -2*overlap) //past tile, save overlapped region
		overlapping.push_back(newline);

	if(current_line == end_tile) {
		finishRow();
	}

	if(current_line == end_tile && current_line  != height) {
		nextRow();
	}


	std::swap(lastLine, newline);
	return scaled;
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

	int start_tile = std::max(0, current_row*tileside - overlap);
	end_tile = std::min(height, (current_row+1)*tileside + overlap);
	int h = end_tile - start_tile;

	int col = 0;
	int start = 0;
	do {
		int end = std::min((col+1)*tileside + overlap, width);
		Tile tile;
		tile.width = end - start;
		tile.encoder = new JpegEncoder;
		fs::path filepath = path / (to_string(col) + "_" + to_string(current_row) + ".jpg");
		tile.encoder->init(filepath.c_str(), tile.width, h);
		push_back(tile);

		col++;
		start = std::max(0, col*tileside - overlap);

	} while(start < width);

	for(auto &line: overlapping) {
		writeLine(line);
	}
	overlapping.clear();

}



bool DeepZoom::build(const std::string &filename, const string &_folder, int _tileside, int _overlap) {
	output = _folder;
	tileside = _tileside;
	overlap = _overlap;

	//create folder filename-ext_files and text file

	JpegDecoder decoder;
	bool ok = decoder.init(filename.c_str(), width, height);
	if(!ok) return false;

	fs::create_directory(output + "_files");

	initRows();

	//for each level
	//line by line
	for(int y = 0; y < height; y++) {
		std::vector<uint8_t> line(width*3);
		decoder.readRows(1, line.data());

		for(size_t i = 0; i < rows.size(); i++) {
			line = rows[i].addLine(line);
			if(!line.size())
				break;
		}
	}

	//define current row. and all its parents.
	FILE *fp = fopen((output + ".dzi").c_str(), "wb");

	std::ofstream out;
	out.open(output + ".dzi");
	out << R"(<?xml version="1.0" encoding="UTF-8"?>
<Image xmlns="http://schemas.microsoft.com/deepzoom/2008"
  Format="jpg"
	)";
	out << "  Overlap=\"" << overlap << "\"\n";
	out << "  TileSize=\"" << tileside << "\">\n";
	out << "  <Size Height=\"" << height << "\" Width=\"" << width << "\"/>\n";
	out << "</Image>\n";
	out.close();

	return true;
}

int DeepZoom::nLevels() {
	int level = 1;
	int w = width;
	int h = height;
	while(w > tileside ||  h > tileside) {
		w >>= 1;
		h >>= 1;
		level++;
	}
	return level;
}

void DeepZoom::initRows() {
	fs::path path(output + "_files");
	int w = width;
	int h = height;
	int level = nLevels()-1;
	while(level >= 0) {
		//create folder.
		fs::path level_path = path / to_string(level);
		fs::create_directory(level_path);

		TileRow row(tileside, overlap, level_path, w, h);
		rows.push_back(row);

		widths.push_back(w);
		heights.push_back(h);

		w >>= 1;
		h >>= 1;
		level--;
	}
}

