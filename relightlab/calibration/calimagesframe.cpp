#include "calimagesframe.h"
#include "../../src/calibration/calibrationsession.h"
#include "../relightapp.h"
#include "../imagelist.h"
#include "../imagegrid.h"
#include "../imageview.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QStatusBar>
#include <QToolBar>

CalImagesFrame::CalImagesFrame(CalibrationSession *session, QWidget *parent)
	: ImageFrame(parent), session(session)
{

	// Add calibration-specific toolbar buttons.
	left_toolbar->addSeparator();
	left_toolbar->addAction(QIcon::fromTheme("folder"), "Add images...",
		this, &CalImagesFrame::addImages);
	left_toolbar->addAction(QIcon::fromTheme("trash"), "Remove selected",
		this, &CalImagesFrame::removeSelected);
}


void CalImagesFrame::showImage(int id) {
	if(id < 0 || id >= session->imageCount())
		return;
	qRelightApp->action("previous_image")->setEnabled(id != 0);
	qRelightApp->action("next_image")->setEnabled(id != session->imageCount() - 1);
	image_view->showImage(id);
	Image &img = session->imageAt(id);
	status->showMessage(QFileInfo(img.filename).canonicalFilePath());
}

void CalImagesFrame::nextImage() {
	int current = image_list->currentRow();
	if(current + 1 >= session->imageCount())
		return;
	image_list->setCurrentRow(current + 1);
	showImage(current + 1);
}

void CalImagesFrame::previousImage() {
	int current = image_list->currentRow();
	if(current <= 0)
		return;
	image_list->setCurrentRow(current - 1);
	showImage(current - 1);
}

void CalImagesFrame::updateSkipped(int /*n*/) {
	// No sphere-based direction computation for calibration images.
	emit skipChanged();
}

void CalImagesFrame::addImages() {
	QStringList files = QFileDialog::getOpenFileNames(
		this, "Select calibration images", QString(),
		"Images (*.tif *.tiff *.jpg *.jpeg *.png *.cr2 *.arw *.nef *.dng);;All files (*)");
	if(files.isEmpty())
		return;
	session->addImages(files);
	init();
}

void CalImagesFrame::removeSelected() {
	QList<QListWidgetItem*> selected = image_list->selectedItems();
	// Collect indices in reverse order to safely remove without shifting.
	QVector<int> indices;
	for(QListWidgetItem *item : selected)
		indices.prepend(item->data(Qt::UserRole).toInt());
	std::sort(indices.begin(), indices.end(), std::greater<int>());
	for(int i : indices)
		session->removeImage(i);
	init();
}

