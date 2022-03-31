#ifndef DEEPZOOM_H
#define DEEPZOOM_H

#include <string>
#include <vector>

#if __cplusplus < 201703L // C++ less than 17
#include <experimental/filesystem>
	namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
	namespace fs = std::filesystem;
#endif



class JpegEncoder;

class Tile {
public:
	int width;
	JpegEncoder *encoder = nullptr;
	std::vector<uint8_t> line;
};

class TileRow: public std::vector<Tile> {
public:
	int written = 0;
	fs::path path;
	int tileside;
	int overlap;

	int width; //total width of the level;
	int height; //total height of the level;

	int current_row = -1;
	int current_line = 0; //keep track of which line we are going to write in the tiles
	int end_tile = 0;
	std::vector<uint8_t> lastLine;
	std::vector<std::vector<uint8_t>> overlapping;

	TileRow() {}
	TileRow(int _tileside, int _overlap, fs::path path, int width, int height);
	//create a new row
	void nextRow();
	void finishRow();

	//returns resized line once every 2 lines.
	std::vector<uint8_t> addLine(std::vector<uint8_t> line);
	std::vector<uint8_t> scaleLines( std::vector<uint8_t> &line0, std::vector<uint8_t> &line1);

};

class DeepZoom {
public:
	int tileside = 256;
	int overlap = 0;
	int width, height;
	std::string output;
	bool build(const std::string &filename, const std::string &output, int tile_size = 256, int overlap = 0);

private:
	std::vector<TileRow> rows;
	std::vector<int> heights;
	std::vector<int> widths;


	void initRows();
	TileRow createTileRow(fs::path level_path, int width, int height);
	int nLevels();
};

#endif // DEEPZOOM_H
