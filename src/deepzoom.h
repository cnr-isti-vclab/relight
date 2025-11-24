#ifndef DEEPZOOM_H
#define DEEPZOOM_H

#include <string>
#include <vector>
#include <deque>
#include <array>

#include <QString>

class JpegEncoder;

enum class PyramidFormat {
	DeepZoom,
	Google,
	Zoomify,
	TiledTiff
};

struct TileRowConfig {
	QString basePath;      // root folder for the layout (zoomify uses TileGroup folders under this path)
	QString levelPath;     // folder specific to this level (deepzoom/google layouts)
	QString suffix = ".jpg";
	PyramidFormat format = PyramidFormat::DeepZoom;
	int level = 0;         // layout level identifier used in filenames
	int tilesX = 0;        // number of tiles along X for this level (used by zoomify indexing)
	int tileStartIndex = 0;// cumulative tile offset for zoomify groups
};

class Tile {
public:
	int width;
	JpegEncoder *encoder = nullptr;
	std::vector<uint8_t> line;
};

class TileRow: public std::vector<Tile> {
public:
	QString path;
	TileRowConfig layout;
	int tileside;
	int overlap;

	int width;  //total width of the scaled image;
	int height; //total height of the scaled image;
	int quality; //0 100 jpeg quality.

	int current_row = -1;
	int current_line = 0;                          //keeps track of which image line we are processing
	int end_tile = 0;                              //wich line ends the current tile
	std::vector<uint8_t> lastLine;                 //keep previous line for scaling
	std::vector<std::vector<uint8_t>> overlapping; //overlapped regions is kept, to be inserted in the new row


	TileRow() {}
	TileRow(int _tileside, int _overlap, const TileRowConfig &config, int width, int height, int quality = 95);
	void nextRow();
	void finishRow();

	//returns resized line once the gaussian kernel can be applied, or empty array
	std::vector<uint8_t> addLine(const std::vector<uint8_t> &line);
	void finalizeInput();
	std::vector<uint8_t> drainScaledLine();
private:
	//actually write the line to the jpegs
	void writeLine(const std::vector<uint8_t> &newline);
	std::vector<uint8_t> emitReadyScaledLine();
	std::vector<uint8_t> applyHorizontalFilter(const std::vector<uint8_t> &line) const;
	std::vector<uint8_t> applyVerticalFilter(const std::array<const std::vector<uint8_t>*,5> &lines) const;
	const std::vector<uint8_t> &lineForIndex(int index) const;
	void expireObsoleteLines(int centerIndex);
	QString tileFilePath(int col) const;

	struct FilteredLine {
		int index = 0;
		std::vector<uint8_t> data;
	};
	std::deque<FilteredLine> filtered;
	int lastInputLine = -1;
	int scaledLinesProduced = 0;
	int scaledLinesTarget = 0;
	int downsampleWidth = 0;
	bool hasNextLevel = false;
	bool inputCompleted = false;

};

class DeepZoom {
public:
	int tileside = 256;
	int overlap = 1;
	int width, height;
	int quality; //0 100 jpeg quality
	QString output;
	bool build(QString filename, QString basename, int tile_size = 254, int overlap = 1, PyramidFormat format = PyramidFormat::DeepZoom);

private:
	std::vector<TileRow> rows;      //one row per level
	std::vector<int> heights;
	std::vector<int> widths;
	PyramidFormat layoutFormat = PyramidFormat::DeepZoom;
	QString layoutRoot;
	QString tileSuffix = ".jpg";
	std::vector<int> tilesX;
	std::vector<int> tilesY;
	std::vector<int> zoomifyOffsets;

	int nLevels();
	void initRows();
	bool buildTiledImages(QString input);
	bool buildTiledTiff(QString input);
	void finalizeMetadata();
	void flushLevels();
};

#endif // DEEPZOOM_H
