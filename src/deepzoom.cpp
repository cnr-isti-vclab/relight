#include "deepzoom.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>

#include <tiffio.h>

#include <assert.h>

using namespace std;

namespace {
constexpr int kGaussianKernel[5] = {1, 4, 6, 4, 1};
constexpr int kGaussianNorm = 16;

std::vector<uint8_t> downsampleGaussian(const std::vector<uint8_t> &src, int width, int height) {
	if(width < 2 || height < 2)
		return {};
	int halfW = width >> 1;
	int halfH = height >> 1;
	std::vector<uint8_t> horiz(halfW * height * 3);
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < halfW; ++x) {
			int center = 2*x + 1;
			for(int c = 0; c < 3; ++c) {
				int acc = 0;
				for(int k = -2; k <= 2; ++k) {
					int ix = std::min(std::max(center + k, 0), width - 1);
					acc += kGaussianKernel[k + 2] * src[(y*width + ix)*3 + c];
				}
				horiz[(y*halfW + x)*3 + c] = static_cast<uint8_t>((acc + (kGaussianNorm/2)) / kGaussianNorm);
			}
		}
	}
	std::vector<uint8_t> out(halfW * halfH * 3);
	for(int y = 0; y < halfH; ++y) {
		int centerY = 2*y + 1;
		for(int x = 0; x < halfW; ++x) {
			for(int c = 0; c < 3; ++c) {
				int acc = 0;
				for(int k = -2; k <= 2; ++k) {
					int iy = std::min(std::max(centerY + k, 0), height - 1);
					acc += kGaussianKernel[k + 2] * horiz[(iy*halfW + x)*3 + c];
				}
				out[(y*halfW + x)*3 + c] = static_cast<uint8_t>((acc + (kGaussianNorm/2)) / kGaussianNorm);
			}
		}
	}
	return out;
}

bool writeTiffLevel(TIFF *tif, const std::vector<uint8_t> &data, int width, int height, int tileside, int levelIndex, int levelCount, int quality) {
	if(!tif)
		return false;
	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(width));
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(height));
	TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileside);
	TIFFSetField(tif, TIFFTAG_TILELENGTH, tileside);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
	TIFFSetField(tif, TIFFTAG_JPEGQUALITY, quality);
	TIFFSetField(tif, TIFFTAG_SUBFILETYPE, levelIndex == 0 ? 0 : FILETYPE_REDUCEDIMAGE);
	TIFFSetField(tif, TIFFTAG_PAGENUMBER, levelIndex, levelCount);
	TIFFSetField(tif, TIFFTAG_SOFTWARE, "relight");

	int tilesX = (width + tileside - 1) / tileside;
	int tilesY = (height + tileside - 1) / tileside;
	size_t tileSize = static_cast<size_t>(tileside) * static_cast<size_t>(tileside) * 3;
	std::vector<uint8_t> tileBuf(tileSize, 0);
	for(int ty = 0; ty < tilesY; ++ty) {
		for(int tx = 0; tx < tilesX; ++tx) {
			std::fill(tileBuf.begin(), tileBuf.end(), 0);
			int copyH = std::min(tileside, height - ty*tileside);
			int copyW = std::min(tileside, width - tx*tileside);
			for(int row = 0; row < copyH; ++row) {
				const uint8_t *src = &data[((ty*tileside + row) * width + tx*tileside)*3];
				uint8_t *dst = &tileBuf[row * tileside * 3];
				std::memcpy(dst, src, static_cast<size_t>(copyW) * 3);
			}
			tsize_t tIndex = TIFFComputeTile(tif, tx*tileside, ty*tileside, 0, 0);
			if(TIFFWriteEncodedTile(tif, tIndex, tileBuf.data(), tileBuf.size()) == -1)
				return false;
		}
	}
	TIFFWriteDirectory(tif);
	return true;
}
}

TileRow::TileRow(int _tileside, int _overlap, const TileRowConfig &config, int _width, int _height, int _quality) {
	tileside = _tileside;
	overlap = _overlap;
	layout = config;
	path = config.levelPath;
	width = _width;
	height = _height;
	quality = _quality;
	end_tile = 0;
	downsampleWidth = width > 1 ? width >> 1 : 0;
	scaledLinesTarget = height > 1 ? height >> 1 : 0;
	hasNextLevel = downsampleWidth > 0 && scaledLinesTarget > 0;
	nextRow();
}

void TileRow::writeLine(const std::vector<uint8_t> &newline) {
	int col = 0;
	for(Tile &tile: *this) {
		int x = std::max(0, col*tileside - overlap);
		tile.encoder->writeRows(const_cast<uint8_t*>(newline.data()) + x*3, 1);
		col++;
	}
}

std::vector<uint8_t> TileRow::addLine(const std::vector<uint8_t> &newline) {
	if(newline.empty())
		return {};

	writeLine(newline);
	lastInputLine = current_line;

	if(hasNextLevel) {
		std::vector<uint8_t> filteredLine = applyHorizontalFilter(newline);
		filtered.push_back({lastInputLine, std::move(filteredLine)});
	}

	current_line++;

	if(current_line > end_tile - 2*overlap)
		overlapping.push_back(newline);

	if(current_line == end_tile) {
		finishRow();
	}

	if(current_line == end_tile && current_line != height) {
		nextRow();
	}

	return emitReadyScaledLine();
}

void TileRow::finalizeInput() {
	inputCompleted = true;
}

std::vector<uint8_t> TileRow::drainScaledLine() {
	return emitReadyScaledLine();
}

std::vector<uint8_t> TileRow::emitReadyScaledLine() {
	if(!hasNextLevel)
		return {};
	if(scaledLinesProduced >= scaledLinesTarget)
		return {};
	int center = 1 + scaledLinesProduced * 2;
	if(!inputCompleted && lastInputLine < center + 2)
		return {};
	if(filtered.empty())
		return {};

	std::array<const std::vector<uint8_t>*, 5> lines;
	for(int k = -2; k <= 2; ++k) {
		int idx = std::min(std::max(center + k, 0), height - 1);
		lines[k + 2] = &lineForIndex(idx);
	}
	std::vector<uint8_t> scaled = applyVerticalFilter(lines);
	scaledLinesProduced++;
	expireObsoleteLines(center);
	return scaled;
}

std::vector<uint8_t> TileRow::applyHorizontalFilter(const std::vector<uint8_t> &line) const {
	if(!hasNextLevel)
		return {};
	std::vector<uint8_t> filtered(downsampleWidth * 3);
	for(int i = 0; i < downsampleWidth; ++i) {
		int center = 2*i + 1;
		for(int c = 0; c < 3; ++c) {
			int acc = 0;
			for(int k = -2; k <= 2; ++k) {
				int src = std::min(std::max(center + k, 0), width - 1);
				acc += kGaussianKernel[k + 2] * line[src*3 + c];
			}
			filtered[i*3 + c] = static_cast<uint8_t>((acc + (kGaussianNorm/2)) / kGaussianNorm);
		}
	}
	return filtered;
}

std::vector<uint8_t> TileRow::applyVerticalFilter(const std::array<const std::vector<uint8_t>*,5> &lines) const {
	std::vector<uint8_t> scaled(downsampleWidth * 3);
	for(int x = 0; x < downsampleWidth; ++x) {
		for(int c = 0; c < 3; ++c) {
			int acc = 0;
			for(int k = 0; k < 5; ++k) {
				acc += kGaussianKernel[k] * (*(lines[k]))[x*3 + c];
			}
			scaled[x*3 + c] = static_cast<uint8_t>((acc + (kGaussianNorm/2)) / kGaussianNorm);
		}
	}
	return scaled;
}

const std::vector<uint8_t> &TileRow::lineForIndex(int index) const {
	assert(!filtered.empty());
	if(index <= filtered.front().index)
		return filtered.front().data;
	if(index >= filtered.back().index)
		return filtered.back().data;
	for(const auto &entry : filtered) {
		if(entry.index == index)
			return entry.data;
	}
	return filtered.back().data;
}

void TileRow::expireObsoleteLines(int centerIndex) {
	int minIndex = centerIndex - 2;
	while(!filtered.empty() && filtered.front().index < minIndex) {
		filtered.pop_front();
	}
}

QString TileRow::tileFilePath(int col) const {
	switch(layout.format) {
	case PyramidFormat::DeepZoom:
		return QString("%1/%2_%3%4").arg(layout.levelPath).arg(col).arg(current_row).arg(layout.suffix);
	case PyramidFormat::Google: {
		QString rowFolder = QString("%1/%2").arg(layout.levelPath).arg(current_row);
		QDir().mkpath(rowFolder);
		return QString("%1/%2%3").arg(rowFolder).arg(col).arg(layout.suffix);
	}
	case PyramidFormat::Zoomify: {
		int tileIndex = layout.tileStartIndex + current_row * layout.tilesX + col;
		int group = tileIndex >> 8;
		QString groupFolder = QString("%1/TileGroup%2").arg(layout.basePath).arg(group);
		QDir().mkpath(groupFolder);
		return QString("%1/%2-%3-%4%5").arg(groupFolder).arg(layout.level).arg(col).arg(current_row).arg(layout.suffix);
	}
	default:
		return {};
	}
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
		tile.encoder = new JpegEncoder();
		tile.encoder->setQuality(quality);
		tile.encoder->setColorSpace(JCS_RGB, 3);
		QString filepath = tileFilePath(col);
		QFileInfo info(filepath);
		QDir().mkpath(info.path());
		tile.encoder->init(filepath.toStdString().c_str(), tile.width, h);
		push_back(tile);

		col++;
		start = std::max(0, col*tileside - overlap);

	} while(start < width);

	for(auto &line: overlapping) {
		writeLine(line);
	}
	overlapping.clear();
}

bool DeepZoom::build(QString input, QString _output, int tile_size, int _overlap, PyramidFormat format) {
	output = _output;
	tileside = tile_size;
	overlap = _overlap;
	layoutFormat = format;
	tileSuffix = ".jpg";

	// Ensure tile size is multiple of 16 for JPEG compression
	if(format == PyramidFormat::TiledTiff && (tileside % 16) != 0) {
		tileside = ((tileside + 15) / 16) * 16;
	}

	if(format == PyramidFormat::TiledTiff) {
		return buildTiledTiff(input);
	}
	return buildTiledImages(input);
}

bool DeepZoom::buildTiledImages(QString input) {
	JpegDecoder decoder;
	if(!decoder.init(input.toStdString().c_str(), width, height))
		return false;

	switch(layoutFormat) {
	case PyramidFormat::DeepZoom:
		layoutRoot = output + "_files";
		break;
	case PyramidFormat::Google:
		layoutRoot = output + "_google";
		break;
	case PyramidFormat::Zoomify:
		layoutRoot = output + "_zoomify";
		break;
	default:
		layoutRoot = output;
		break;
	}
	QDir().mkpath(layoutRoot);

	initRows();

	std::vector<uint8_t> line(width * 3);
	std::vector<uint8_t> carry;

	for(int y = 0; y < height; ++y) {
		decoder.readRows(1, line.data());
		const std::vector<uint8_t> *current = &line;
		carry.clear();
		for(size_t level = 0; level < rows.size(); ++level) {
			std::vector<uint8_t> next = rows[level].addLine(*current);
			if(next.empty())
				break;
			carry = std::move(next);
			current = &carry;
		}
	}

	flushLevels();
	finalizeMetadata();
	return true;
}

int DeepZoom::nLevels() {
	int level = 1;
	int w = width;
	int h = height;
	while(w > tileside || h > tileside) {
		w = std::max(1, w >> 1);
		h = std::max(1, h >> 1);
		level++;
	}
	return level;
}

void DeepZoom::initRows() {
	rows.clear();
	widths.clear();
	heights.clear();
	tilesX.clear();
	tilesY.clear();
	zoomifyOffsets.clear();

	int levels = nLevels();
	tilesX.resize(levels);
	tilesY.resize(levels);
	zoomifyOffsets.resize(levels);

	int w = width;
	int h = height;
	for(int level = levels - 1; level >= 0; --level) {
		tilesX[level] = std::max(1, (w + tileside - 1) / tileside);
		tilesY[level] = std::max(1, (h + tileside - 1) / tileside);
		widths.push_back(w);
		heights.push_back(h);
		w = std::max(1, w >> 1);
		h = std::max(1, h >> 1);
	}

	int cumulative = 0;
	for(int level = levels - 1; level >= 0; --level) {
		zoomifyOffsets[level] = cumulative;
		cumulative += tilesX[level] * tilesY[level];
	}

	w = width;
	h = height;
	for(int level = levels - 1; level >= 0; --level) {
		TileRowConfig config;
		config.format = layoutFormat;
		config.level = level;
		config.tilesX = tilesX[level];
		config.tileStartIndex = zoomifyOffsets[level];
		config.suffix = tileSuffix;
		if(layoutFormat == PyramidFormat::Zoomify) {
			config.basePath = layoutRoot;
		} else {
			config.levelPath = layoutRoot + "/" + QString::number(level);
			QDir().mkpath(config.levelPath);
		}
		rows.emplace_back(tileside, overlap, config, w, h, quality);
		w = std::max(1, w >> 1);
		h = std::max(1, h >> 1);
	}
}

void DeepZoom::flushLevels() {
	for(size_t level = 0; level < rows.size(); ++level) {
		rows[level].finalizeInput();
		while(true) {
			std::vector<uint8_t> pending = rows[level].drainScaledLine();
			if(pending.empty())
				break;
			const std::vector<uint8_t> *current = &pending;
			for(size_t next = level + 1; next < rows.size(); ++next) {
				std::vector<uint8_t> forwarded = rows[next].addLine(*current);
				if(forwarded.empty()) {
					current = nullptr;
					break;
				}
				pending = std::move(forwarded);
				current = &pending;
			}
		}
	}
}

void DeepZoom::finalizeMetadata() {
	if(layoutFormat == PyramidFormat::DeepZoom) {
		std::ofstream out(output.toStdString() + ".dzi");
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		out << "<Image xmlns=\"http://schemas.microsoft.com/deepzoom/2008\"\n";
		out << "  Format=\"" << tileSuffix.toStdString().substr(1) << "\"\n";
		out << "  Overlap=\"" << overlap << "\"\n";
		out << "  TileSize=\"" << tileside << "\">\n";
		out << "  <Size Height=\"" << height << "\" Width=\"" << width << "\"/>\n";
		out << "</Image>\n";
		out.close();
	} else if(layoutFormat == PyramidFormat::Zoomify) {
		int totalTiles = 0;
		for(size_t i = 0; i < tilesX.size(); ++i)
			totalTiles += tilesX[i] * tilesY[i];
		QString metadataPath = layoutRoot + "/ImageProperties.xml";
		std::ofstream out(metadataPath.toStdString());
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		out << "<IMAGE_PROPERTIES WIDTH=\"" << width << "\" HEIGHT=\"" << height << "\" ";
		out << "NUMTILES=\"" << totalTiles << "\" NUMIMAGES=\"1\" ";
		out << "VERSION=\"1.8\" TILESIZE=\"" << tileside << "\"/>\n";
		out.close();
	}
}

bool DeepZoom::buildTiledTiff(QString input) {
	JpegDecoder decoder;
	if(!decoder.init(input.toStdString().c_str(), width, height))
		return false;

	std::vector<uint8_t> levelData(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
	for(int y = 0; y < height; ++y) {
		decoder.readRows(1, levelData.data() + static_cast<size_t>(y) * width * 3);
	}

	std::vector<std::vector<uint8_t>> pyramid;
	std::vector<int> widthsCascade;
	std::vector<int> heightsCascade;
	pyramid.push_back(std::move(levelData));
	widthsCascade.push_back(width);
	heightsCascade.push_back(height);

	int currentW = width;
	int currentH = height;
	while(currentW > tileside || currentH > tileside) {
		if(currentW < 2 || currentH < 2)
			break;
		const std::vector<uint8_t> &src = pyramid.back();
		std::vector<uint8_t> down = downsampleGaussian(src, currentW, currentH);
		currentW = std::max(1, currentW >> 1);
		currentH = std::max(1, currentH >> 1);
		if(down.empty())
			break;
		pyramid.push_back(std::move(down));
		widthsCascade.push_back(currentW);
		heightsCascade.push_back(currentH);
	}

	QString path = output + ".tif";
	TIFF *tif = TIFFOpen(path.toStdString().c_str(), "w8");
	if(!tif)
		return false;
	bool ok = true;
	for(size_t level = 0; level < pyramid.size() && ok; ++level) {
		ok = writeTiffLevel(tif,
			pyramid[level],
			widthsCascade[level],
			heightsCascade[level],
			tileside,
			static_cast<int>(level),
			static_cast<int>(pyramid.size()),
			quality);
	}
	TIFFClose(tif);
	return ok;
}
