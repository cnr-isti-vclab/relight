#ifndef DEEPZOOM_H
#define DEEPZOOM_H

#include <string>
#include <vector>

#include <QString>

class JpegEncoder;

class Tile {
public:
	int width;
	JpegEncoder *encoder = nullptr;
	std::vector<uint8_t> line;
};

class TileRow: public std::vector<Tile> {
public:
	QString path;
	int tileside;
	int overlap;

	int width;  //total width of the scaled image;
	int height; //total height of the scaled image;

	int current_row = -1;
	int current_line = 0;                          //keeps track of which image line we are processing
	int end_tile = 0;                              //wich line ends the current tile
	std::vector<uint8_t> lastLine;                 //keep previous line for scaling
	std::vector<std::vector<uint8_t>> overlapping; //overlapped regions is kept, to be inserted in the new row


	TileRow() {}
	TileRow(int _tileside, int _overlap, QString path, int width, int height);
	void nextRow();
	void finishRow();

	//returns resized line once every 2 lines or empty array
	std::vector<uint8_t> addLine(std::vector<uint8_t> line);
	//scale 2 lines into a single line half the length for smaller level
	std::vector<uint8_t> scaleLines( std::vector<uint8_t> &line0, std::vector<uint8_t> &line1);
private:
	//actually write the line to the jpegs
	void writeLine(std::vector<uint8_t> newline);

};

class DeepZoom {
public:
	int tileside = 254;
	int overlap = 1;
	int width, height;
	QString output;
	bool build(QString filename, QString basename, int tile_size = 254, int overlap = 1);

private:
	std::vector<TileRow> rows;      //one row per level
	std::vector<int> heights;
	std::vector<int> widths;

	int nLevels();
	void initRows();
	TileRow createTileRow(QString level_path, int width, int height);
};

#endif // DEEPZOOM_H
