#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>

#include "imagestab.h"
#include "relightapp.h"

ImagesTab::ImagesTab() {

	QVBoxLayout *container = new QVBoxLayout(this);

	QToolBar *toolbar = new QToolBar(this);
	toolbar->addAction(qRelightApp->action("fullscreen"));
}
