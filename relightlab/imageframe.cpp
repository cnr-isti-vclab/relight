#include <QVBoxLayout>
#include <QToolBar>
#include <QGraphicsView>

#include "relightapp.h"
#include "imageframe.h"

ImageFrame::ImageFrame() {
	QVBoxLayout *container = new QVBoxLayout(this);

	QHBoxLayout *toolbars = new QHBoxLayout();

	QToolBar *left_toolbar = new QToolBar;
	left_toolbar->addAction(qRelightApp->action("zoom_fit"));
	left_toolbar->addAction(qRelightApp->action("zoom_one"));
	left_toolbar->addAction(qRelightApp->action("zoom_in"));
	left_toolbar->addAction(qRelightApp->action("zoom_out"));
	left_toolbar->addAction(qRelightApp->action("rotate_right"));
	left_toolbar->addAction(qRelightApp->action("rotate_left"));

	toolbars->addWidget(left_toolbar);
	toolbars->addSpacing(1);

	QToolBar *right_toolbar = new QToolBar;
	right_toolbar->addAction(qRelightApp->action("show_image"));
	right_toolbar->addAction(qRelightApp->action("show_list"));
	right_toolbar->addAction(qRelightApp->action("show_grid"));
	toolbars->addWidget(right_toolbar);

	container->addLayout(toolbars);
	container->addWidget(new QGraphicsView);

}
