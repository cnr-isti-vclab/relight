#include "deepzoom.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

#include <iostream>
#include <fstream>

#include <assert.h>
using namespace std;

vector<uint8_t>  TileRow::scaleLines( std::vector<uint8_t> &line0, std::vector<uint8_t> &line1) {
	assert(line0.size() == width*3);
	assert(line0.size() == line1.size());
	std::vector<uint8_t> scaled;

	int w = width >> 1;
	scaled.resize(w*3);
	for(int k = 0; k < 3; k++) {
		for(size_t i = 0; i < w; i++) {
			int j = i*2*3;
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


std::vector<uint8_t> TileRow::addLine(std::vector<uint8_t> newline) {

	vector<uint8_t> scaled;
	if(current_line % 2) {
		scaled = scaleLines(lastLine, newline);
	}

	//write line to jpeg
	written++;
	int x = 0;
	for(Tile &tile: *this) {
		tile.encoder->writeRows(newline.data() + x*3, 1);
		x += tileside;
	}

	current_line++;

	if(current_line - end_tile > 0) //past tile, save overlapped region
		overlapping.push_back(newline);

	if(current_line == end_tile + overlap || current_line == height) {
		finishRow();
	}

	if(current_line == end_tile + overlap && current_line  != height) {
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
	written = 0;
	clear();
}

void TileRow::nextRow() {
	current_row++;

	int h = std::min(tileside + overlap, height - current_row*tileside);

	int x = 0;
	int col = 0;
	do {

		Tile tile;
		tile.width = std::min(tileside+overlap, width - x);
		tile.encoder = new JpegEncoder;
		fs::path filepath = path / (to_string(col) + "_" + to_string(current_row) + ".jpg");
		tile.encoder->init(filepath.c_str(), tile.width, h);
		push_back(tile);

		x += tileside;
		col++;

	} while(x < width);

	for(auto &line: overlapping) {
		written++;
		int x = 0;
		for(Tile &tile: *this) {
			tile.encoder->writeRows(line.data() + x*3, 1);
			x += tileside;
		}
	}

	//current_line -= overlapping.size();
	end_tile += tileside;

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
  Format="jpeg"
	)";
	out << "  Overlap=\"" << overlap << "\"\n";
	out << "  TileSize=\"" << tileside << "\">\n";
	out << "  <Size Height=\"" << width << "\" Width=\"3184\"/>\n";
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

