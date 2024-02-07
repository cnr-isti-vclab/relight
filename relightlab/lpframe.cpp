#include "lpframe.h"
#include "relightapp.h"
#include "lightgeometry.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QCheckBox>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QVector>
#include <QString>
#include <QPair>
#include <QDoubleSpinBox>

class PropertyBox: public QFrame {
public:
	QStringList keys;
	QGridLayout *grid;

	PropertyBox(QStringList _keys, QWidget *parent = nullptr): QFrame(parent) {
		keys = _keys;
		grid = new QGridLayout(this);
		for(int i = 0; i < keys.size(); i++) {
			QLabel *key = new QLabel(keys[i]);
			grid->addWidget(key, i, 0);
			grid->addWidget(new QLabel(""), i, 1);
		}
		grid->setRowStretch(grid->rowCount(), 1);
	}
	void setValue(QString key, QVariant value) {
		int pos = keys.indexOf(key);
		QLabel *label = (QLabel *)grid->itemAtPosition(pos, 1)->widget();
		label->setText(value.toString());
	}
};


LpFrame::LpFrame(QWidget *parent): QFrame(parent) {

	QVBoxLayout *content = new QVBoxLayout(this);
	content->setContentsMargins(31, 31, 31, 31);
	this->setLayout(content);


	QLabel *title = new QLabel("<h2>LP light directions</h2>");
	content->addWidget(title);
	content->addSpacing(30);

	QFrame *file_frame = new QFrame;
	content->addWidget(file_frame);
	QHBoxLayout *file_layout = new QHBoxLayout(file_frame);

	QPushButton *load = new QPushButton("Load LP file...");
	load->setProperty("class", "large");
	load->setMaximumWidth(300);
	file_layout->addWidget(load);

	filename = new QLabel();
	file_layout->addWidget(filename);


	QHBoxLayout *columns = new QHBoxLayout();
	content->addLayout(columns);


	lights = new QGraphicsView(&scene);
	lights->setBackgroundBrush(Qt::black);
	lights->setMinimumSize(300, 300);
	lights->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	columns->addWidget(lights);
	columns->setAlignment(lights, Qt::AlignTop);



	geometry = new LightsGeometry;
	columns->addWidget(geometry);
	columns->addStretch();

	content->addStretch();

	QPushButton *cancel = new QPushButton(QIcon::fromTheme("cancel"), "Cancel");
	cancel->setProperty("class", "large");
	cancel->setMinimumWidth(200);
	content->addWidget(cancel, 0, Qt::AlignRight);

	connect(cancel, SIGNAL(clicked()), this->parent(), SLOT(showChoice()));
	connect(load, SIGNAL(clicked()), this, SLOT(loadLP()));
}

void LpFrame::init() {
	geometry->init();

}

void LpFrame::initLights() {
	qreal scale = 200;
	//scene goes from [-1, +1]x[-1, +1], view will just zoom on it
	Project &project = qRelightApp->project();
	qreal radius = scale/50;
	for(Image &img: project.images) {
		Vector3f &dir = img.direction;
		QGraphicsEllipseItem *e = scene.addEllipse(dir[0]*scale, dir[1]*scale, radius, radius);
		e->setBrush(Qt::white);
	}

	qreal margin = scale/10;
	qreal side = scale + margin;
	lights->fitInView(QRectF(-side, -side, 2*side, 2*side), Qt::KeepAspectRatio);
}

void LpFrame::loadLP() {
	QString lp = QFileDialog::getOpenFileName(this, "Load an LP file", QString(), "Light directions (*.lp)");
	if(lp.isNull())
		return;
	try {
		qRelightApp->project().loadLP(lp);
	} catch(QString error) {
		QMessageBox::critical(this, "Loading .lp file failed", error);
		return;
	}
	filename->setText("Filename: " + lp);
	geometry->images_number->setText(QString::number(qRelightApp->project().size()));
	geometry->image_width->setValue(0);
	geometry->diameter->setValue(0);
	geometry->vertical_offset->setValue(0);
	initLights();
}
