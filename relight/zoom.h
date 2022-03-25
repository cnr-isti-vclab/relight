#ifndef ZOOM_H
#define ZOOM_H

#include <vips/vips.h>
#include <functional>
#include <QDir>
#include <QMessageBox>
#include <QXmlSimpleReader>
#include <QString>
#include <QStringList>
#include <QDomDocument>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

typedef struct _TarzoomData
{
    int overlap;
    int tilesize;
    int width;
    int height;
} TarzoomData;

inline int getNFiles(const QString& folder, const QString& format)
{
    QDir destination(folder);
    return destination.entryList(QStringList(QString("plane_*.%1").arg(format)), QDir::Files).size();
}

inline QString getTarzoomPlaneData(const QString& path, TarzoomData& data)
{
    QDomDocument dziXml;
    QFile dziFile(path);

    if (!dziFile.open(QIODevice::ReadOnly))
        return QString("Couldn't open file %1").arg(path);
    if (!dziXml.setContent(&dziFile)) {
        dziFile.close();
        return QString("Couldn't read file %1 as an XML document").arg(path);
    }

    QDomNode imageNode = dziXml.namedItem("Image");
    QDomNode sizeNode = imageNode.firstChild();

    data.overlap = imageNode.attributes().namedItem("Overlap").nodeValue().toInt();
    data.tilesize = imageNode.attributes().namedItem("TileSize").nodeValue().toInt();
    data.width = sizeNode.attributes().namedItem("Width").nodeValue().toInt();
    data.height = sizeNode.attributes().namedItem("Height").nodeValue().toInt();

    return "OK";
}

inline QString deepZoom(QString inputFolder, QString output, uint32_t quality, uint32_t overlap,
              uint32_t tileSize, std::function<bool(std::string s, int n)> progressed)
{
    int nplanes = getNFiles(output, "jpg");

    // Deep zoom every plane
    for(int plane = 0; plane < nplanes; plane++)
    {
        // Load image, setup output folder for this plane
        QString fileName = (QStringList() << QString("%1/plane_%2").arg(output).arg(plane) << QString(".jpg")).join("");
        VipsImage* image = vips_image_new_from_file(fileName.toStdString().c_str(), NULL);
        if (image == NULL)
            return vips_error_buffer();

        QString folderName = QString("%1\\plane_%2").arg(output).arg(plane).toStdString().c_str();

        // Call dzsave and create the deepzoom tiles
        if (image == NULL || vips_dzsave(image, folderName.toStdString().c_str(),
            "overlap", overlap,
            "tile_size", tileSize,
            "layout", VIPS_FOREIGN_DZ_LAYOUT_DZ,
            "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE,
            "suffix", QString(".jpg[Q=%1]").arg(quality).toStdString().c_str(), NULL) != 0)
        {
            return vips_error_buffer();
        }

        // Update progress bar
        if(!progressed("Deepzoom:", 100*(plane+1)/nplanes))
            break;
    }

    return "OK";
}

/** TODO:
 *  - simplify levelName / levelPath thing
 * @brief tarZoom
 * @param inputFolder
 * @param output
 * @return
 */
inline QString tarZoom(QString inputFolder, QString output, std::function<bool(std::string s, int n)> progressed)
{
    // Find number of planes
    int nPlanes = getNFiles(inputFolder, "dzi");
    if (nPlanes == 0)
        return QString("No dzi files in input folder %1").arg(inputFolder);

    // Tzi json data
    QJsonObject index;
    // Data contained in the dzi
    TarzoomData data;
    // Output files and paths
    QString outPath = QString("%1/%2.tzb").arg(output).arg(inputFolder.mid(inputFolder.lastIndexOf("/")));
    QString outIndexPath = QString("%1/%2.tzi").arg(output).arg(inputFolder.mid(inputFolder.lastIndexOf("/")));
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
    QString dziPath = QString("%1/plane_0.dzi").arg(inputFolder);
    QString err = getTarzoomPlaneData(dziPath, data);
    index.insert("tilesize", data.tilesize);
    index.insert("overlap", data.overlap);
    index.insert("format", "jpg");
    index.insert("nlevels", QDir(QString("%1/plane_0_files").arg(inputFolder)).entryList(QDir::AllDirs | QDir::NoDotAndDotDot).size());

    // Convert each plane
    for (int i=0; i<nPlanes; i++)
    {
        QDir planeFolder(QString("%1/plane_%2_files").arg(inputFolder).arg(i));
        qDebug() << planeFolder.path();
        QString dziPath = QString("%1/plane_%2.dzi").arg(inputFolder).arg(i);
        QString err = getTarzoomPlaneData(dziPath, data);

        // Error while reading current dzi file
        if (err.compare("OK") != 0)
            return err;

        for (QString levelName : planeFolder.entryList(QDir::AllDirs | QDir::NoDotAndDotDot))
        {
            QString levelPath = QString("%1/%2").arg(planeFolder.path()).arg(levelName);
            QDir level(levelPath);
            QStringList files = level.entryList(QDir::Files);

            for (QString fileName : files)
            {
                // Open the file
                QString filePath = QString("%1/%2").arg(levelPath).arg(fileName);
                QFile currFile(filePath);
                if (!currFile.open(QIODevice::ReadOnly))
                    return QString("Couldn't open image file %1 for processing").arg(filePath);

                // Add the file, keep track of the offsets
                offset += currFile.size();
                offsets.push_back(offset);
                outFile.write(currFile.readAll());

                currFile.close();
            }
        }

        // Update progress bar
        if(!progressed("Deepzoom:", 100*(i+1)/nPlanes))
            break;
    }

    outFile.close();

    // Add offsets to index
    QJsonArray offsetsJson;
    for (int i=0; i<offsets.size(); i++)
        offsetsJson.push_back(offsets[i]);
    index.insert("offsets", offsetsJson);

    // Write the index data
    outIndexFile.write(QJsonDocument(index).toJson());
    outIndexFile.close();

    return "OK";
}

#endif // ZOOM_H
