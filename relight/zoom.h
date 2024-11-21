#ifndef ZOOM_H
#define ZOOM_H

#include <cmath>
#include <functional>
#include <QDir>
#include <QMessageBox>
	
#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QRegularExpression>
#include "../src/deepzoom.h"
#include <deque>

typedef struct _ZoomData
{
    int overlap;
    int tilesize;
    int width;
    int height;
    int nlevels;
} ZoomData;

inline int getNFiles(const QString& folder, const QString& format)
{
    QDir destination(folder);
    return destination.entryList(QStringList(QString("plane_*.%1").arg(format)), QDir::Files).size();
}

inline QString getTarzoomPlaneData(const QString& path, ZoomData& data)
{
    QDomDocument dziXml;
    QFile dziFile(path);

    if (!dziFile.open(QIODevice::ReadOnly))
        return QString("Couldn't open file %1").arg(path);
    if (!dziXml.setContent(&dziFile)) {
        dziFile.close();
        return QString("Couldn't read file %1 as an XML document").arg(path);
    }
    dziFile.close();

    QDomNode imageNode = dziXml.namedItem("Image");
    QDomNode sizeNode = imageNode.firstChild();

    data.overlap = imageNode.attributes().namedItem("Overlap").nodeValue().toInt();
    data.tilesize = imageNode.attributes().namedItem("TileSize").nodeValue().toInt();
    data.width = sizeNode.attributes().namedItem("Width").nodeValue().toInt();
    data.height = sizeNode.attributes().namedItem("Height").nodeValue().toInt();

    return "OK";
}

inline QString getItarzoomPlaneData(const QString& path, ZoomData& data, QJsonArray& offsets)
{
    QFile inputFile(path);
    QJsonDocument inputJson;
    QJsonObject inputObject;
    QJsonParseError jsonError;

    // Put the contents of the tzi file into the QJsonDocument
    if (!inputFile.open(QIODevice::ReadOnly))
        return QString("Couldn't open file %1").arg(path);

    inputJson = QJsonDocument::fromJson(inputFile.readAll(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
        return QString("Error reading tzi file: %1").arg(jsonError.errorString());
    inputFile.close();

    // Read the relevant data
    inputObject = inputJson.object();
    data.overlap = inputObject.value("overlap").toInt();
    data.tilesize = inputObject.value("tilesize").toInt();
    data.height = inputObject.value("height").toInt();
    data.width = inputObject.value("width").toInt();
    data.nlevels = inputObject.value("nlevels").toInt();
    offsets = inputObject.value("offsets").toArray();

    return "OK";
}

inline QString deepZoom(QString inputFolder, QString output, uint32_t quality, uint32_t overlap,
			  uint32_t tileSize, std::function<bool(QString s, int n)> progressed)
{
    int nplanes = getNFiles(inputFolder, "jpg");


    // Deep zoom every plane
    for(int plane = 0; plane < nplanes; plane++)
    {
        // Load image, setup output folder for this plane
        QString fileName = (QStringList() << QString("%1/plane_%2").arg(inputFolder).arg(plane) << QString(".jpg")).join("");
		DeepZoom dz;
		dz.quality = quality;
        dz.build(fileName, output + "/" + QString("plane_%1").arg(plane), tileSize, overlap);

        // Update progress bar
        if(!progressed("Deepzoom:", 100*(plane+1)/nplanes))
            break;
    }

    return "OK";
}

inline QString tarZoom(QString inputFolder, QString output, std::function<bool(QString s, int n)> progressed)
{
    // Find number of planes
    int nPlanes = getNFiles(inputFolder, "dzi");
    if (nPlanes == 0)
        return QString("No dzi files in input folder %1").arg(inputFolder);

    // Regex to find the indexes of the images
    QRegularExpression regex("(\\d+)_(\\d+).jp.*g");

    // Convert each plane
    for (int i=0; i<nPlanes; i++)
    {
        // Tzi json data
        QJsonObject index;
        // Data contained in the dzi
        ZoomData data;
        // Output files and paths
        QString outPath = QString("%1/plane_%2.tzb").arg(output).arg(i);
        QString outIndexPath = QString("%1/plane_%2.tzi").arg(output).arg(i);
        QFile outFile(outPath);
        QFile outIndexFile(outIndexPath);

        std::vector<float> offsets;
        int offset = 0;

        offsets.push_back(offset);
        if (!outFile.open(QIODevice::WriteOnly))
            return QString("Couldn't open output file %1").arg(outPath);
        if (!outIndexFile.open(QIODevice::WriteOnly))
            return QString("Couldn't open output index file %1").arg(outIndexPath);

        // Setup index file
        QString dziPath = QString("%1/plane_%2.dzi").arg(inputFolder).arg(i);
        QString err = getTarzoomPlaneData(dziPath, data);
        if (err.compare("OK") != 0)
            return err;

        index.insert("tilesize", data.tilesize);
        index.insert("overlap", data.overlap);
        index.insert("format", "jpg");
        index.insert("nlevels", QDir(QString("%1/plane_0_files").arg(inputFolder)).entryList(QDir::AllDirs | QDir::NoDotAndDotDot).size());
        index.insert("width", data.width);
        index.insert("height", data.height);

        QDir planeFolder(QString("%1/plane_%2_files").arg(inputFolder).arg(i));

        // Error while reading current dzi file
        if (err.compare("OK") != 0)
            return err;

        for (QString& levelName : planeFolder.entryList(QDir::AllDirs | QDir::NoDotAndDotDot))
        {
            QString levelPath = QString("%1/%2").arg(planeFolder.path(), levelName);
            QDir level(levelPath);
            level.setSorting(QDir::Name);
            QStringList files = level.entryList(QDir::Files);
            std::vector<QFile*> orderedFiles;

            int maxX = std::floor((float)data.width / data.tilesize + 1);
            int maxY = std::floor((float)data.height / data.tilesize + 1);
            orderedFiles.resize(maxX * maxY);

            for (QString& fileName : files)
            {
                QRegularExpressionMatch match = regex.match(fileName);
                int x = match.captured(1).toUInt();
                int y = match.captured(2).toUInt();

                // Save the file for later
                QString filePath = QString("%1/%2").arg(levelPath, fileName);
                orderedFiles[x + maxX * y] = new QFile(filePath);
            }

			for (size_t i = 0; i < orderedFiles.size(); i++)
            {
                QFile* file = orderedFiles[i];
                if (file != nullptr)
                {
                    if (!file->open(QIODevice::ReadOnly))
                        return QString("Couldn't open image file %1 for processing").arg(file->fileName());

                    // Add the file, keep track of the offsets
                    offset += file->size();
                    offsets.push_back(offset);
                    outFile.write(file->readAll());

                    file->close();
                    delete file;
                }
            }
        }

        outFile.close();

        // Add offsets to index
        QJsonArray offsetsJson;
		for (size_t i = 0; i < offsets.size(); i++)
            offsetsJson.push_back(offsets[i]);
        index.insert("offsets", offsetsJson);

        // Write the index data
        outIndexFile.write(QJsonDocument(index).toJson());
        outIndexFile.close();

        // Update progress bar
        if(!progressed("Tarzoom:", 100*(i+1)/nPlanes))
            break;
    }

    return "OK";
}

inline QString itarZoom(const QString& inputFolder, const QString& output, std::function<bool(QString s, int n)> progressed)
{
    // Find number of planes
    int nPlanes = getNFiles(inputFolder, "tzb");
    if (nPlanes == 0)
        return QString("No tzb files in input folder %1").arg(inputFolder);

    int nIndexes = getNFiles(inputFolder, "tzi");
    if (nIndexes == 0)
        return QString("No tzi files in input folder %1").arg(inputFolder);
    if (nIndexes != nPlanes)
        return QString("The number of .tzi and .tzb files doesn't match");

    // Output files and paths
    QString outPath = QString("%1/planes.tzb").arg(output);
    QString outIndexPath = QString("%1/planes.tzi").arg(output);
    QFile outFile(outPath);
    QFile outIndexFile(outIndexPath);

    if (!outFile.open(QIODevice::WriteOnly))
        return QString("Couldn't open output file %1").arg(outPath);
    if (!outIndexFile.open(QIODevice::WriteOnly))
        return QString("Couldn't open output index file %1").arg(outIndexPath);

    // Output Json objects
    QJsonObject tzi;
    QJsonObject tzb;
    bool tziSetup = false;

    // Vector of files in the right order
    std::vector<QFile*> files;
    // Final tzi sizes
    std::vector<std::deque<uint32_t>> tzbSizes;
    int nSizes = 0;
    // Final tzi offsets
    QJsonArray tzbOffsets;

    tzbOffsets.append(0);
    tzbSizes.resize(nPlanes);

    // Convert each plane
    for (int i=0; i<nPlanes; i++)
    {
        // Data contained in the tzi
        ZoomData data;
        // Tarzoom vector of offset
        QJsonArray offsets;
        // Offset to size convertion
        std::deque<uint32_t> sizes;

        // Get tzi data
        QString err = getItarzoomPlaneData(QString("%1/plane_%2.tzi").arg(inputFolder).arg(i), data, offsets);
        if (err.compare("OK") != 0)
            return err;

        // Convert offsets to sizes and add them to the tzi
        for (int j=0; j<offsets.size() - 1; j++)
            sizes.push_back(offsets[j + 1].toInt() - offsets[j].toInt());
        tzbSizes[i].insert(tzbSizes[i].end(), sizes.begin(), sizes.end());
        nSizes += sizes.size();

        if (!tziSetup) {
            tzi.insert("tilesize", data.tilesize);
            tzi.insert("overlap", data.overlap);
            tzi.insert("format", "jpg");
            tzi.insert("nlevels", data.nlevels);
            tzi.insert("mode", "interleaved");
            tzi.insert("stride", nPlanes);
            tzi.insert("width", data.width);
            tzi.insert("height", data.height);
            tziSetup = true;
        }

        // Append file to final tzb
        QString tzbPath = QString("%1/plane_%2.tzb").arg(inputFolder).arg(i);
        files.push_back(new QFile(tzbPath));
        if (!files[i]->open(QIODevice::ReadOnly))
            return QString("Error while opening .tzb file %1").arg(files[i]->fileName());

        // Update progress bar
        if(!progressed("Itarzoom:", 50*(i+1)/nPlanes))
            break;
    }

    int offset = 0;
    for (int i=0; i<nSizes; i++)
    {
        // Get tzbSizes[i] bytes from the i%nPlanes file and write them on the final file
        int fileIdx = i % nPlanes;
        // Take a size from vector 0 first, then from vector 1 etc, then get the next one from vector0,
        // then from vector1 etc
        outFile.write(files[fileIdx]->read(tzbSizes[fileIdx][0]));
        offset += tzbSizes[fileIdx][0];

        tzbSizes[fileIdx].pop_front();
        tzbOffsets.append(offset);

        if(!progressed("Itarzoom:", 50 + 50*(i+1)/tzbSizes.size()))
            break;
    }

    tzi.insert("offsets", tzbOffsets);
    outIndexFile.write(QJsonDocument(tzi).toJson());

    outFile.close();
    outIndexFile.close();

    // Clean file pointers
	for(QFile *file: files)
		delete file;

    return "OK";
}

#endif // ZOOM_H
