#ifndef ZOOM_H
#define ZOOM_H

#include <ui_zoomdialog.h>
#include <QDialog>

#include "task.h"

#define LIB_VIPS_ERR {const char* cError = vips_error_buffer(); \
    error = cError; \
    status = FAILED;}

class Zoom : public QDialog
{
    Q_OBJECT
public:
    enum class ZoomType { None = -1, DeepZoom = 0, Tarzoom = 1, ITarzoom = 2 };

    explicit Zoom(QWidget* parent = nullptr);
    ~Zoom(){delete m_Ui;};

    void doZoom();

    void setTabIndex(int index);
    void setZoomType(ZoomType type) {m_ZoomType = type;};

    void setOutputFolder(const QString& output) {m_OutputFolder = output;}
    void setInputFolder(const QString& input) {m_InputFolder = input;}
    void setJpegQuality(int quality) {m_JpegQuality = quality;}
    void setOverlap(int overlap) {m_Overlap = overlap;}
    void setTileSize(int tilesize) {m_TileSize = tilesize;}

private slots:
    void buttonBrowseOutputClicked();
    void buttonBrowseInputClicked();
    void buttonConfirmClicked();

    void inputQualityChanged(const QString& newValue);
    void inputTileSizeChanged(const QString& newValue);
    void inputOverlapChanged(const QString& newValue);

private:
    void updateZoomString();

private:
    Ui::ZoomDialog* m_Ui;

    QString m_OutputFolder;
    QString m_InputFolder;

    ZoomType m_ZoomType;
    QString m_ZoomString;

    int m_JpegQuality;
    int m_Overlap;
    int m_TileSize;
};

class ZoomTask : public Task
{
public:
    ZoomTask(QObject* parent, Zoom::ZoomType zoomType) : Task(parent), m_ZoomType(zoomType) {}
    ~ZoomTask(){};

    virtual void run() override;
    virtual void pause() override {}
    virtual void resume() override {}
    virtual void stop() override {}
    virtual bool progressed(std::string s, int percent) override;

    void deepZoom(QString inputFolder, QString outputFolder, uint32_t quality, uint32_t overlap, uint32_t tileSize);
    void tarZoom(QString inputFolder, QString outputFolder);
    void itarZoom(QString inputFolder, QString outputFolder);

private:
    int getNPlanes(QString& output);

private:
    Zoom::ZoomType m_ZoomType;
};

#endif // ZOOM_H
