#include "zoom.h"
#include "zoomtask.h"

#include <QDirIterator>

void ZoomTask::run()
{
    status = RUNNING;
    // Parameter retrieval
    QString outFolder = hasParameter("output") ? (*this)["output"].value.toString() : "";
    QString inFolder = hasParameter("input") ? (*this)["input"].value.toString() : "";

    this->output = outFolder;
    this->input_folder = inFolder;

    int quality = hasParameter("quality") ? (*this)["quality"].value.toInt() : -1;
    int overlap = hasParameter("overlap") ? (*this)["overlap"].value.toInt() : -1;
    int tilesize = hasParameter("tilesize") ? (*this)["tilesize"].value.toInt() : -1;
    bool deleteFiles = hasParameter("deletefiles") ? (*this)["deletefiles"].value.toBool() : false;

    std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };
    QString zoomError;

    // Try copying info.json to the output folder to allow for casting
    if (outFolder.compare(inFolder) != 0)
    {
        // Overwrite previous file if it exists
        QFile outInfo(QString("%1/info.json").arg(outFolder));
        if (outInfo.exists())
        {
            outInfo.setPermissions(QFileDevice::WriteOther | QFileDevice::WriteOwner);
            outInfo.remove();
        }

        // Copy file
        QFile inInfo(QString("%1/info.json").arg(inFolder));
        if (inInfo.exists())
            inInfo.copy(QString("%1/info.json").arg(outFolder));
    }

    // General error checking
    if (outFolder.compare("") == 0) {
        error = "Unspecified output folder";
        status = FAILED;
        return;
    }else if (inFolder.compare("") == 0) {
        error = "Unspecified input folder";
        status = FAILED;
        return;
    }

    switch (m_ZoomType)
    {
    case ZoomType::DeepZoom:
        // Deepzoom error checking
        if (quality == -1) {
            error = "Unspecified jpeg quality for DeepZoom";
            status = FAILED;
            return;
        }else if (overlap == -1) {
            error = "Unspecified overlap for DeepZoom";
            status = FAILED;
            return;
        } else if (tilesize == -1) {
            error = "Unspecified tile size for DeepZoom";
            status = FAILED;
            return;
        } else {
            // Launching deep zoom
            zoomError = deepZoom(inFolder, outFolder, quality, overlap, tilesize, callback);
        }
        break;
    case ZoomType::Tarzoom:
        // Launching tar zoom
        zoomError = tarZoom(inFolder, outFolder, callback);
        break;
    case ZoomType::ITarzoom:
        // Launching itar zoom
        zoomError = itarZoom(inFolder, outFolder, callback);
        break;
    case ZoomType::None:
        break;
    }

    if (zoomError.compare("OK") != 0) {
        error = zoomError;
        status = FAILED;
        return;
    }

    if (deleteFiles)
        deletePrevFiles(inFolder);
    status = DONE;
}


void ZoomTask::deletePrevFiles(QDir folder)
{
    QRegularExpression dzRegex("plane_\\d+_files");
    QRegularExpression tzRegex("plane_\\d+.tz(i|b)");
    QStringList names;

    if (m_ZoomType == ZoomType::Tarzoom)
    {
        // Delete deepzoom folders
        names = folder.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);

        for (QString& fileName : names)
        {
            if (dzRegex.match(fileName).hasMatch())
            {
                QDir dir(QString("%1/%2").arg(folder.absolutePath(), fileName));
                QDirIterator it(dir.absolutePath(), QStringList() << "*.jpg", QDir::Files,  QDirIterator::Subdirectories);

                for (QString s : it.next())
                {
                    QFile file(s);
                    file.setPermissions(QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ReadOwner | QFileDevice::WriteOwner);
                }
                 dir.removeRecursively();
            }
        }

        // Delete .dzi files
        names = folder.entryList(QDir::Files);
        for (QString& fileName : names)
        {
            if (fileName.endsWith(".dzi"))
            {
                QFile file(QString("%1/%2").arg(folder.absolutePath(), fileName));
                file.setPermissions(QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ReadOwner | QFileDevice::WriteOwner);
                file.remove();
            }
        }
    }
    else if (m_ZoomType == ZoomType::ITarzoom)
    {
        // Delete tarzoom files
        names = folder.entryList(QDir::Files);

        for (QString& fileName : names)
        {
            if (tzRegex.match(fileName).hasMatch())
            {
                QFile file(QString("%1/%2").arg(folder.absolutePath(), fileName));
                file.setPermissions(QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ReadOwner | QFileDevice::WriteOwner);
                file.remove();
            }
        }
    }
}

bool ZoomTask::progressed(std::string s, int percent)
{
    if(status == PAUSED) {
        mutex.lock();
        mutex.unlock();
    }
    if(status == STOPPED)
        return false;

    QString str(s.c_str());
    emit progress(str, percent);
    if(status == STOPPED)
        return false;
    return true;
}
