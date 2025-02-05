#include "verifydialog.h"
#include "../src/sphere.h"
#include "flowlayout.h"
#include "relightapp.h"
#include "verifyview.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QPushButton>
#include <QImage>
#include <QLabel>
#include <QDialogButtonBox>

#include <opencv2/opencv.hpp>

#include "assert.h"
#include <iostream>
using namespace std;

VerifyDialog::VerifyDialog(std::vector<QImage> &_thumbs, std::vector<QPointF> &_positions, Markers marker, QWidget *parent):
	QDialog(parent), thumbs(_thumbs), positions(_positions) {
	setModal(true);

	showMaximized();
	QVBoxLayout *layout = new QVBoxLayout(this);
	if(marker == ALIGN) {
		QHBoxLayout *operations_layout = new QHBoxLayout;
		layout->addLayout(operations_layout);
		QPushButton *reset = new QPushButton("Reset");
		operations_layout->addWidget(reset);
		QPushButton *ecc = new QPushButton("Align");
		operations_layout->addWidget(ecc);
		connect(reset, SIGNAL(clicked()), this, SLOT(resetAligns()));
		connect(ecc, SIGNAL(clicked()), this, SLOT(alignSamples()));
	}
	QScrollArea *area = new QScrollArea(this);
	layout->addWidget(area);

	area->setWidgetResizable(true);
	area->setWidget(new QWidget);
	flowlayout = new FlowLayout();
	area->widget()->setLayout(flowlayout);

	area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	for(size_t i = 0; i < thumbs.size(); i++) {
		assert(!thumbs[i].isNull());
		VerifyView *thumb = new VerifyView(thumbs[i], 192, positions[i], marker == REFLECTION? VerifyMarker::REFLECTION : VerifyMarker::ALIGN);
		views.push_back(thumb);
		flowlayout->addWidget(thumb);
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
	layout->addWidget(buttonBox);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

void VerifyDialog::resetAligns() {
	for(QPointF &p: positions)
		p = QPointF(0, 0);
	update();
}

cv::Mat qimg2mat(QImage img) {
	QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
	return cv::Mat(gray.height(), gray.width(), CV_8UC1,
				  const_cast<uchar*>(gray.bits()), gray.bytesPerLine()).clone();
}

void VerifyDialog::alignSamples() {
	if (positions.empty()) return;

	cv::Mat ref = qimg2mat(thumbs[0]);

	for (size_t i = 1; i < thumbs.size(); i++) {
		cv::Mat warpMat = cv::Mat::eye(2, 3, CV_32F);

		try {
			cv::findTransformECC(ref, qimg2mat(thumbs[i]), warpMat, cv::MOTION_TRANSLATION);

			positions[i] = QPointF(warpMat.at<float>(0, 2), warpMat.at<float>(1, 2));
		} catch(cv::Exception &e) {
			cerr << e.msg << endl;
			positions[i] = QPointF(0.0f, 0.0f);
		}
	}
	update();
}

void VerifyDialog::update() {
	for(VerifyView *view: views)
		view->set();
}
