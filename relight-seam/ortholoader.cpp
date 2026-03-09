#include "ortholoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTextStream>
#include <QStringList>
#include <QXmlStreamReader>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
QStringList imageExtensions() {
	return {QStringLiteral("tif"), QStringLiteral("tiff"),
	        QStringLiteral("TIF"), QStringLiteral("TIFF")};
}

constexpr double kRotationTolerance = 0;
constexpr double kResolutionTolerance = 0;
constexpr double kIntegerTolerance = 0;

bool nearlyZero(double value, double tolerance) {
	return std::abs(value) <= tolerance;
}
}

bool OrthoLoader::loadFromDirectory(const QString &directoryPath, QString *errorMessage) {
	tiles_.clear();
	canvasSize_ = QSize();
	pixelWidth_ = 0.0;
	pixelHeight_ = 0.0;
	hasReference_ = false;

	if (directoryPath.isEmpty()) {
		if (errorMessage)
			*errorMessage = QStringLiteral("No directory selected.");
		return false;
	}

	QDir dir(directoryPath);
	if (!dir.exists()) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Directory does not exist: %1").arg(directoryPath);
		return false;
	}

	// Check for optional Orthophotomosaic.tfw and MTDOrtho.xml reference files
	const QString referenceTfwPath = dir.absoluteFilePath(QStringLiteral("Orthophotomosaic.tfw"));
	const QString mtdOrthoPath = dir.absoluteFilePath(QStringLiteral("MTDOrtho.xml"));
	
	if (QFileInfo::exists(referenceTfwPath)) {
		if (!parseTfw(referenceTfwPath, &referenceTfw_, errorMessage))
			return false;
		if (!ensureRotationIsZero(referenceTfw_, QStringLiteral("Orthophotomosaic.tfw"), errorMessage))
			return false;
		hasReference_ = true;
		pixelWidth_ = std::abs(referenceTfw_.scaleX);
		pixelHeight_ = std::abs(referenceTfw_.scaleY);
		
		// Optionally parse MTDOrtho.xml for canvas size
		if (QFileInfo::exists(mtdOrthoPath)) {
			parseMTDOrtho(mtdOrthoPath, &referenceCanvasSize_, nullptr); // Ignore errors, it's optional
		}
	}

	const QStringList tfwFiles = dir.entryList({QStringLiteral("*.tfw"), QStringLiteral("*.TFW")}, QDir::Files | QDir::Readable);
	if (tfwFiles.isEmpty()) {
		if (errorMessage)
			*errorMessage = QStringLiteral("No TFW files found in %1").arg(directoryPath);
		return false;
	}

	for (const QString &tfwFileName : tfwFiles) {
		// Skip the reference file if it exists
		if (tfwFileName == QStringLiteral("Orthophotomosaic.tfw"))
			continue;
		
		const QString tfwPath = dir.absoluteFilePath(tfwFileName);
		TfwRecord record;
		if (!parseTfw(tfwPath, &record, errorMessage))
			return false;

		if (!ensureRotationIsZero(record, tfwFileName, errorMessage))
			return false;

		if (!ensureResolutionConsistency(record, tfwFileName, errorMessage))
			return false;

		const QString imagePath = resolveImagePath(dir, tfwFileName);
		if (imagePath.isEmpty()) {
			if (errorMessage)
				*errorMessage = QStringLiteral("No matching TIFF image for %1").arg(tfwFileName);
			return false;
		}

		// Get image dimensions without loading full image
		QImageReader reader(imagePath);
		if (!reader.canRead()) {
			if (errorMessage)
				*errorMessage = QStringLiteral("Cannot read image %1").arg(imagePath);
			return false;
		}
		QSize imageSize = reader.size();
		if (!imageSize.isValid() || imageSize.isEmpty()) {
			if (errorMessage)
				*errorMessage = QStringLiteral("Invalid image dimensions in %1").arg(imagePath);
			return false;
		}

		Tile tile;
		tile.name = QFileInfo(imagePath).fileName();
		tile.imagePath = imagePath;
		tile.maskPath = resolveMaskPath(imagePath);
		tile.width = imageSize.width();
		tile.height = imageSize.height();
		if (!computeTileOffset(record, &tile, tfwFileName, errorMessage))
			return false;

		tiles_.push_back(tile);
	}

	if (!finalizeTiles(errorMessage))
		return false;

	return true;
}

bool OrthoLoader::parseTfw(const QString &filePath, TfwRecord *record, QString *errorMessage) const {
	if (!record)
		return false;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Unable to open %1").arg(filePath);
		return false;
	}

	QTextStream stream(&file);
	double values[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	int valueCount = 0;

	while (!stream.atEnd() && valueCount < 6) {
		const QString line = stream.readLine().trimmed();
		if (line.isEmpty())
			continue;

		bool ok = false;
		const double number = line.toDouble(&ok);
		if (!ok) {
			if (errorMessage)
				*errorMessage = QStringLiteral("Invalid numeric value in %1: %2").arg(filePath, line);
			return false;
		}

		values[valueCount++] = number;
	}

	if (valueCount != 6) {
		if (errorMessage)
			*errorMessage = QStringLiteral("TFW file %1 does not contain 6 values.").arg(filePath);
		return false;
	}

	record->scaleX = values[0];
	record->rotationY = values[1];
	record->rotationX = values[2];
	record->scaleY = values[3];
	record->translateX = values[4];
	record->translateY = values[5];

	return true;
}

bool OrthoLoader::ensureRotationIsZero(const TfwRecord &record, const QString &tfwFile, QString *errorMessage) const {
	if (nearlyZero(record.rotationX, kRotationTolerance) &&
	    nearlyZero(record.rotationY, kRotationTolerance))
		return true;

	if (errorMessage)
		*errorMessage = QStringLiteral("Expected zero rotation in %1").arg(tfwFile);
	return false;
}

bool OrthoLoader::ensureResolutionConsistency(const TfwRecord &record, const QString &tfwFile, QString *errorMessage) {
	const double width = std::abs(record.scaleX);
	const double height = std::abs(record.scaleY);
	if (width <= 0.0 || height <= 0.0) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Invalid pixel size in %1").arg(tfwFile);
		return false;
	}

	if (pixelWidth_ == 0.0 && pixelHeight_ == 0.0) {
		pixelWidth_ = width;
		pixelHeight_ = height;
		return true;
	}

	if (!nearlyZero(pixelWidth_ - width, kResolutionTolerance) ||
	    !nearlyZero(pixelHeight_ - height, kResolutionTolerance)) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Tile %1 uses a different resolution").arg(tfwFile);
		return false;
	}

	return true;
}

bool OrthoLoader::computeTileOffset(const TfwRecord &record, Tile *tile, const QString &tfwFile, QString *errorMessage) const {
	if (!tile)
		return false;

	if (pixelWidth_ <= 0.0 || pixelHeight_ <= 0.0) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Missing resolution metadata before processing %1").arg(tfwFile);
		return false;
	}

	const double rawX = record.translateX / pixelWidth_;
	const double rawY = -record.translateY / pixelHeight_;
	const int offsetX = static_cast<int>(std::lround(rawX));
	const int offsetY = static_cast<int>(std::lround(rawY));

	tile->x = offsetX;
	tile->y = offsetY;
	return true;
}

QString OrthoLoader::resolveImagePath(const QDir &directory, const QString &tfwFile) const {
	const QFileInfo tfwInfo(tfwFile);
	const QString baseName = tfwInfo.completeBaseName();

	for (const QString &extension : imageExtensions()) {
		const QString candidate = directory.absoluteFilePath(baseName + QLatin1Char('.') + extension);
		if (QFileInfo::exists(candidate))
			return candidate;
	}

	return QString();
}

QString OrthoLoader::resolveMaskPath(const QString &imagePath) const {
	const QFileInfo imageInfo(imagePath);
	const QString fileName = imageInfo.fileName();
	const QString dirPath = imageInfo.absolutePath();

	// Try to find mask by replacing Ort_ with PC_
	if (fileName.startsWith(QStringLiteral("Ort_"), Qt::CaseInsensitive)) {
		QString maskFileName = fileName;
		maskFileName.replace(0, 4, QStringLiteral("PC_"));
		const QString maskPath = QDir(dirPath).absoluteFilePath(maskFileName);
		if (QFileInfo::exists(maskPath))
			return maskPath;
	}

	// No mask found - will use black pixel detection fallback
	return QString();
}

bool OrthoLoader::parseMTDOrtho(const QString &filePath, QSize *canvasSize, QString *errorMessage) const {
	if (!canvasSize)
		return false;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Unable to open %1").arg(filePath);
		return false;
	}

	QXmlStreamReader xml(&file);
	while (!xml.atEnd()) {
		xml.readNext();
		if (xml.isStartElement() && xml.name() == QStringLiteral("NombrePixels")) {
			const QString text = xml.readElementText().trimmed();
			const QStringList parts = text.split(QRegularExpression(QStringLiteral("\\s+")));
			if (parts.size() != 2) {
				if (errorMessage)
					*errorMessage = QStringLiteral("Invalid NombrePixels format in %1").arg(filePath);
				return false;
			}
			bool okWidth = false, okHeight = false;
			const int width = parts[0].toInt(&okWidth);
			const int height = parts[1].toInt(&okHeight);
			if (!okWidth || !okHeight || width <= 0 || height <= 0) {
				if (errorMessage)
					*errorMessage = QStringLiteral("Invalid pixel dimensions in %1").arg(filePath);
				return false;
			}
			*canvasSize = QSize(width, height);
			return true;
		}
	}

	if (xml.hasError()) {
		if (errorMessage)
			*errorMessage = QStringLiteral("XML parsing error in %1: %2").arg(filePath, xml.errorString());
		return false;
	}

	if (errorMessage)
			*errorMessage = QStringLiteral("NombrePixels not found in %1").arg(filePath);
	return false;
}

bool OrthoLoader::finalizeTiles(QString *errorMessage) {
	if (tiles_.isEmpty()) {
		if (errorMessage)
			*errorMessage = QStringLiteral("No TIFF images were loaded.");
		return false;
	}

	if (pixelWidth_ <= 0.0 || pixelHeight_ <= 0.0) {
		if (errorMessage)
			*errorMessage = QStringLiteral("Invalid pixel size metadata.");
		return false;
	}

	if (hasReference_) {
		// Use Orthophotomosaic.tfw as reference for georeferencing
		// Calculate reference origin in pixel coordinates
		const double refOriginX = referenceTfw_.translateX / pixelWidth_;
		const double refOriginY = -referenceTfw_.translateY / pixelHeight_;
		const int refX = static_cast<int>(std::lround(refOriginX));
		const int refY = static_cast<int>(std::lround(refOriginY));

		// Adjust all tiles relative to reference origin
		for (Tile &tile : tiles_) {
			tile.x -= refX;
			tile.y -= refY;
		}

		// Use full canvas size from MTDOrtho.xml if available
		if (referenceCanvasSize_.isValid() && !referenceCanvasSize_.isEmpty()) {
			canvasSize_ = referenceCanvasSize_;
		} else {
			// Fallback: calculate tight bounding box of tiles
			int minX = std::numeric_limits<int>::max();
			int minY = std::numeric_limits<int>::max();
			int maxX = std::numeric_limits<int>::min();
			int maxY = std::numeric_limits<int>::min();
			
			for (const Tile &tile : tiles_) {
				minX = std::min(minX, tile.x);
				minY = std::min(minY, tile.y);
				maxX = std::max(maxX, tile.x + tile.width);
				maxY = std::max(maxY, tile.y + tile.height);
			}

			// Shift tiles to start at (0,0) while preserving relative positions
			for (Tile &tile : tiles_) {
				tile.x -= minX;
				tile.y -= minY;
			}

			canvasSize_ = QSize(maxX - minX, maxY - minY);
		}
	} else {
		// No reference: use tight bounding box from tiles
		int minX = std::numeric_limits<int>::max();
		int minY = std::numeric_limits<int>::max();
		for (const Tile &tile : tiles_) {
			minX = std::min(minX, tile.x);
			minY = std::min(minY, tile.y);
		}

		int canvasWidth = 0;
		int canvasHeight = 0;
		for (Tile &tile : tiles_) {
			tile.x -= minX;
			tile.y -= minY;
			canvasWidth = std::max(canvasWidth, tile.x + tile.width);
			canvasHeight = std::max(canvasHeight, tile.y + tile.height);
		}

		canvasSize_ = QSize(canvasWidth, canvasHeight);
	}

	std::sort(tiles_.begin(), tiles_.end(), [](const Tile &a, const Tile &b) {
		return a.x < b.x;
	});

	return true;
}
