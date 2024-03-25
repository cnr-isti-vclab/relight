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


VerifyDialog::VerifyDialog(std::vector<QImage> &_thumbs, std::vector<QPointF> &_positions, QWidget *parent):
	QDialog(parent), thumbs(_thumbs), positions(_positions) {
	setModal(true);

	showMaximized();
	QVBoxLayout *layout = new QVBoxLayout(this);
	QScrollArea *area = new QScrollArea(this);
	layout->addWidget(area);

	area->setWidgetResizable(true);
	area->setWidget(new QWidget);
	flowlayout = new FlowLayout();
	area->widget()->setLayout(flowlayout);

	area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	for(int i = 0; i < thumbs.size(); i++) {
		VerifyView *thumb = new VerifyView(thumbs[i], positions[i], 192);
		flowlayout->addWidget(thumb);
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	layout->addWidget(buttonBox);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}
