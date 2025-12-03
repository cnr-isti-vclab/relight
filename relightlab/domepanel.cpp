#include "lightgeometry.h"
#include "relightapp.h"
#include "directionsview.h"
#include "domepanel.h"
#include "../src/lp.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>

#include <QDebug>

#include <vector>
#include <iostream>
using namespace std;

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QWidget>

class ElidedComboBox : public QComboBox {
public:
	using QComboBox::QComboBox;

protected:
	void paintEvent(QPaintEvent *event) override {
		QStyleOptionComboBox opt;
		initStyleOption(&opt);
		QPainter painter(this);

		// Draw the combo box background and frame
		style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);

		// Get the selected text and elide if necessary
		QString text = currentText();
		QFontMetrics fm(font());
		opt.currentText = fm.elidedText(text, Qt::ElideRight, width() - 20); // Leave space for arrow

		// Draw the elided text inside the box
		style()->drawControl(QStyle::CE_ComboBoxLabel, &opt, &painter, this);
	}

	void showPopup() override {
		view()->setMinimumWidth(view()->sizeHintForColumn(0) + 20); // Ensure full text is visible
		QComboBox::showPopup();
	}
};


DomePanel::DomePanel(QWidget *parent): QFrame(parent) {

	//	setContentsMargins(10, 10, 10, 10);
	QHBoxLayout *content = new QHBoxLayout(this);
	//content->setHorizontalSpacing(20);

	{
		sphere_frame = new QFrame;
		sphere_frame->setFrameShape(QFrame::Panel);
		sphere_frame->setAutoFillBackground(true);

		QHBoxLayout *sphere_layout = new QHBoxLayout(sphere_frame);
		{
			sphere_button = new QPushButton(QIcon::fromTheme("highlight"), "Use reflective spheres");
			sphere_button->setProperty("class", "large");
			sphere_button->setMinimumWidth(200);
			sphere_button->setMaximumWidth(300);
			connect(sphere_button, SIGNAL(clicked()), this, SLOT(setSpheres()));

			sphere_layout->addWidget(sphere_button);
		}
		content->addWidget(sphere_frame, 0, Qt::AlignTop);
	}

	{
		dome_frame = new QFrame;
		dome_frame->setFrameShape(QFrame::Panel);
		dome_frame->setAutoFillBackground(true);

		QHBoxLayout *dome_layout = new QHBoxLayout(dome_frame);
		{
			QPushButton *load = new QPushButton(QIcon::fromTheme("folder"), "Load dome file...");
			load->setProperty("class", "large");
			load->setMinimumWidth(200);
			load->setMaximumWidth(300);
			connect(load, SIGNAL(clicked()), this, SLOT(loadDomeFile()));

			dome_layout->addWidget(load);

			dome_list = new ElidedComboBox;
			dome_list->setMinimumWidth(200);
			dome_list->setMaximumWidth(300);
			dome_list->setProperty("class", "large");
			connect(dome_list, SIGNAL(currentIndexChanged(int)), this, SLOT(setDome(int)));

			dome_layout->addWidget(dome_list);
		}
		content->addWidget(dome_frame, 0, Qt::AlignTop);
	}



	QPushButton *save = new QPushButton(QIcon::fromTheme("save"), "Export dome...");
	save->setProperty("class", "large");
	save->setMinimumWidth(200);
	save->setMaximumWidth(300);
	connect(save, SIGNAL(clicked()), this, SLOT(exportDome()));
	content->addWidget(save, 0, Qt::AlignTop);


	init();
}

void DomePanel::setSphereSelected() {
	bool use_sphere = qRelightApp->project().dome.lightSource == Dome::FROM_SPHERES;
	QPalette pal = palette();
	QColor highlightColor = pal.color(QPalette::Highlight);  // Theme-defined highlight color
	QColor normalColor = pal.color(QPalette::Window);
	sphere_frame->setPalette(QPalette(use_sphere? highlightColor : normalColor));
	dome_frame->setPalette(QPalette(use_sphere? normalColor : highlightColor));
	
	// Force visual update
	sphere_frame->update();
	dome_frame->update();
}

void DomePanel::init() {
	setSphereSelected();
	updateDomeList();
}

void DomePanel::updateDomeList(QString path) {
	//if path is present and not current
	//dome_labels.clear();
	dome_paths.clear();
	dome_list->clear();
	dome_list->addItem("Select a recent dome...");
	//get list of existing domes
	QStringList paths = qRelightApp->domes();
	for(QString path: paths) {
		Dome dome; //yep, same name for a class member
		try {
			dome.load(path);
		} catch (QString error) {
			qDebug() << "Problems loading dome at" << path << ": " << error;
			qRelightApp->removeDome(path);
			continue;
		}

		QFileInfo info(path);
		dome.label = info.filePath();

		dome_labels.append(dome.label);
		dome_paths.append(path);
		dome_list->addItem(dome.label);
	}
	if(!path.isNull()) {
		int index = dome_paths.indexOf(path);
		assert(index != -1);

		if(dome_list->currentIndex() != index+1) {
			dome_list->blockSignals(true);
			dome_list->setCurrentIndex(index+1);
			dome_list->blockSignals(false);
		}
	}
}

void DomePanel::setSpheres() {
	dome_list->blockSignals(true);
	dome_list->setCurrentIndex(0);
	dome_list->blockSignals(false);

	emit useSpheres();
	setSphereSelected();
}

void DomePanel::setDome(int index) {
	if(index <= 0)
		return;
	try {
		loadDomeFile(dome_paths[index-1]); //First index is "Seelect a recent dome..."
	} catch(QString error) {
		QMessageBox::critical(this, "Could not load this dome:", error);
		Dome &dome = qRelightApp->project().dome;
		int index = dome_paths.indexOf(dome.label) +1; //if not found returns -1 and goes to 0. Lucky!
		dome_list->blockSignals(true);
		dome_list->setCurrentIndex(index);
		dome_list->blockSignals(false);
	}
	setSphereSelected();
}
void DomePanel::loadDomeFile() {
	QString path = QFileDialog::getOpenFileName(this, "Load a .lp or .dome file", QDir::currentPath(), "Light directions and domes (*.lp *.dome )");
	if(path.isNull())
		return;
	loadDomeFile(path);
	setSphereSelected();
}

void DomePanel::loadDomeFile(QString path) {
	if(path.endsWith(".lp"))
		loadLP(path);
	if(path.endsWith(".dome"))
		loadDome(path);
	//	dome_list->clearSelection();
}

void DomePanel::exportDome() {
	QString filename = QFileDialog::getSaveFileName(this, "Select a dome file", qRelightApp->lastProjectDir(), "*.dome *.lp");
	if(filename.isNull())
		return;

	Dome &dome = qRelightApp->project().dome;

	if(filename.endsWith(".lp")) {
		try {
			//TODO this should be a function in Dome.
			qRelightApp->project().saveLP(filename, dome.directions);
		} catch(QString error) {
			QMessageBox::critical(this, "Failed to save the .lp file", error);
			return;
		}

	} else {

		if(!filename.endsWith(".dome"))
			filename += ".dome";

		try {
			dome.save(filename);
		} catch(QString error) {
			QMessageBox::critical(this, "Failed to save the .dome file", error);
			return;
		}
	}
	qRelightApp->addDome(filename);
}


void DomePanel::loadLP(QString path) {
	std::vector<QString> filenames;
	std::vector<Eigen::Vector3f> directions;

	parseLP(path, directions, filenames);
	if(qRelightApp->project().size() != directions.size())
		QMessageBox::warning(this, "Wrong number of lights (or images)",
			"The number of lights must be the same as the number of checked images.");

	Dome &dome = qRelightApp->project().dome;
	dome.lightConfiguration = Dome::DIRECTIONAL;
	dome.directions = directions;
	QFileInfo info(path);
	dome.label = info.filePath();
	dome.lightSource = Dome::FROM_LP;
	dome.recomputePositions();

	qRelightApp->addDome(path);

	updateDomeList(path);
	setSphereSelected();  // Update highlighting after loading LP
	emit updated();
}

void DomePanel::loadDome(QString path) {
	Dome &dome = qRelightApp->project().dome;

	float imageWidth = dome.imageWidth;
	Dome new_dome;
	new_dome.load(path);

	if(qRelightApp->project().size() != new_dome.directions.size())
		QMessageBox::warning(this, "Wrong number of lights (or images)",
			"The number of lights must be the same as the number of checked images.");

	dome = new_dome;

	QFileInfo info(path);
	dome.label = info.filePath();
	qRelightApp->addDome(path);
	//preserve image width if we actually have a measurement.
	if(imageWidth != 0 && qRelightApp->project().measures.size() != 0)
		dome.imageWidth = imageWidth;
	dome.recomputePositions();

	updateDomeList(path);
	setSphereSelected();  // Update highlighting after loading dome
	emit updated();
}
