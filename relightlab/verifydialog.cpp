#include "verifydialog.h"
#include "../src/sphere.h"
#include "flowlayout.h"
#include "relightapp.h"
#include "verifyview.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QImage>
#include <QLabel>
#include <QDialogButtonBox>
#include "assert.h"


VerifyDialog::VerifyDialog(Sphere *_sphere, QWidget *parent): QDialog(parent) {
	setModal(true);

	showMaximized();
	sphere = _sphere;
	QVBoxLayout *layout = new QVBoxLayout(this);
	QScrollArea *area = new QScrollArea(this);
	layout->addWidget(area);

	area->setWidgetResizable(true);
	area->setWidget(new QWidget);
	flowlayout = new FlowLayout();
	area->widget()->setLayout(flowlayout);
	area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	std::vector<QImage> &thumbs = sphere->thumbs;

	for(size_t i = 0; i < thumbs.size(); i++) {
		VerifyView *thumb = new VerifyView(i, sphere, 192);
		flowlayout->addWidget(thumb);
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	layout->addWidget(buttonBox);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}
