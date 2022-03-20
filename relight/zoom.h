#ifndef ZOOM_H
#define ZOOM_H

#include <vips/vips.h>
#include <functional>
#include <QDir>
#include <QMessageBox>
#include <QString>
#include <QStringList>

int getNPlanes(QString& output) {
    QDir destination(output);
    return destination.entryList(QStringList("plane_*.jpg"), QDir::Files).size();
}


const char* deepZoom(QString inputFolder, QString output, uint32_t quality, uint32_t overlap,
              uint32_t tileSize, std::function<bool(std::string s, int n)> progressed)
{
    int nplanes = getNPlanes(output);

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

    return NULL;
}

#endif // ZOOM_H
