#ifndef ZOOM_H
#define ZOOM_H

#include <ui_zoomdialog.h>
#include <QDialog>


class Zoom : public QDialog
{
    enum class ZoomType { DeepZoom = 0, Tarzoom = 1, ITarzoom = 2 };

    Q_OBJECT
public:
    explicit Zoom(QWidget* parent = nullptr);
    ~Zoom(){};

    void doZoom();

    void setTabIndex(int index);
    void setZoomType(ZoomType type);

    void setOutputFolder(QString output) {m_OutputFolder = output;}
    void setInputFolder(QString input) {m_InputFolder = input;}
    void setJpegQuality(int quality) {m_JpegQuality = quality;}
    void setOverlap(int overlap) {m_Overlap = overlap;}
    void setTileSize(int tilesize) {m_TileSize = tilesize;}

private slots:
    void buttonBrowseOutputClicked();
    void buttonBrowseRtiClicked();
    void buttonConfirmClicked();

    void inputQualityChanged(QString& newValue);
    void inputTileSizeChanged(QString& newValue);
    void inputOverlapChanged(QString& newValue);

private:
    Ui::ZoomDialog* m_Ui;

    QString m_OutputFolder;
    QString m_InputFolder;

    int m_JpegQuality;
    int m_Overlap;
    int m_TileSize;
};

#endif // ZOOM_H
