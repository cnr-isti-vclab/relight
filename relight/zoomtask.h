#ifndef ZOOMTASK_H
#define ZOOMTASK_H

#include "task.h"

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

private:
    ZoomType m_ZoomType;
};

#endif // ZOOMTASK_H
