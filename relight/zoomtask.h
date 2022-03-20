#ifndef ZOOMTASK_H
#define ZOOMTASK_H

#include "task.h"


#define LIB_VIPS_ERR {const char* cError = vips_error_buffer(); \
    error = cError; \
    status = FAILED;}


class ZoomTask : public Task
{
public:
    enum class ZoomType { None = -1, DeepZoom = 0, Tarzoom = 1, ITarzoom = 2 };

    ZoomTask(QObject* parent, ZoomType zoomType) : Task(parent), m_ZoomType(zoomType) {}
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
    ZoomType m_ZoomType;
};

#endif // ZOOMTASK_H
