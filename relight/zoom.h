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

inline QString tarZoom(QString inputFolder, QString output)
{
    // Find number of planes
    int nPlanes = getNFiles(inputFolder, "dzi");
    if (nPlanes == 0)
        return "No dzi files in input folder";

    std::vector<float> offsets;
    QRegExp positionRegExp("(\\d+)_(\\d+).jp.*g");

    // Convert each plane
    for (int i=0; i<nPlanes; i++)
    {
        TarzoomData data;
        QDir planeFolder(QString("%1/plane_%2_files").arg(inputFolder).arg(i));
        QString dziPath = QString("%1/plane_%2.dzi").arg(inputFolder).arg(i);
        QString err = getTarzoomPlaneData(dziPath, data);

        // Error while reading current dzi file
        if (err.compare("OK") != 0)
            return err;

        for (QDir level : planeFolder.entryList())
        {
            QStringList files = level.entryList(QStringList() << "*.jpg" << "*.JPG", QDir::Files);
            int maxX = 0, maxY = 0;

            for (QString filePath : files)
            {
                // Get x and y for current file
                int x, y;
                // Open the file
                QFile currFile(filePath);
                if (!currFile.open(QIODevice::ReadOnly))
                    return QString("Couldn't open image file %1 for processing").arg(filePath);

                // Read all the contents and use the regex to find x and y
                QTextStream textStream(&currFile);
                QString fileContents = textStream.readAll();

                positionRegExp.indexIn(fileContents);
                x = positionRegExp.cap(0).toUInt();
                y = positionRegExp.cap(1).toUInt();

                maxX = MAX(x, maxX);
                maxY = MAX(y, maxY);
            }
        }

    }
    return NULL;
}

#endif // ZOOM_H
