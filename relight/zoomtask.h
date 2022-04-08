#ifndef ZOOMTASK_H
#define ZOOMTASK_H

#include <QDir>
#include "task.h"

/** TODO
 *  -   Copy info.json to output folder if it's different from the input folder
 *  -   Properly show task info in the process queue: trying to delete a task shows the info for some reason
 *  -   Make resume and stop actually work
 */


class ZoomTask : public Task
{
public:
    enum class ZoomType { None = -1, DeepZoom = 0, Tarzoom = 1, ITarzoom = 2 };

    ZoomTask(QObject* parent, ZoomType zoomType) : Task(parent), m_ZoomType(zoomType) {}
    ~ZoomTask(){};

    virtual void run() override;
    virtual bool progressed(std::string s, int percent) override;

private:
    void deletePrevFiles(QDir folder);
private:
    ZoomType m_ZoomType;
};

#endif // ZOOMTASK_H
