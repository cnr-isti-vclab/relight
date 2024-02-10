#include "spherepicking.h"
#include "canvas.h"

#include <QToolBar>
#include <QStatusBar>

SpherePicking::SpherePicking(QWidget *parent): ImageFrame(parent) {
	left_toolbar->hide();
	right_toolbar->hide();

	status->showMessage("Double click on the boundary of the sphere.");
}

void SpherePicking::showImage(int id) {
	 ImageFrame::showImage(id);

	 status->showMessage("Double click on the boundary of the sphere.");
}
