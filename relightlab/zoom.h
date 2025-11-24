#ifndef ZOOM_H
#define ZOOM_H

#include <cmath>
#include <functional>
#include <QDir>
#include <QMessageBox>

#include <QString>
#include <QFile>
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

// Function declarations - implementations in zoom.cpp
void deepZoom(QString inputFolder, QString output, uint32_t quality, uint32_t overlap,
              uint32_t tileSize, std::function<bool(QString s, int n)> progressed);

void tiffZoom(QString inputFolder, QString output, uint32_t quality, uint32_t tileSize,
              std::function<bool(QString s, int n)> progressed);

void tarZoom(QString inputFolder, QString output, std::function<bool(QString s, int n)> progressed);

void itarZoom(const QString& inputFolder, const QString& output, std::function<bool(QString s, int n)> progressed);

#endif // ZOOM_H
