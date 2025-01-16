#include "metadataframe.h"
#include "relightapp.h"

#include <QVBoxLayout>

MetadataFrame::MetadataFrame(QWidget *parent): QFrame(parent) {
	QVBoxLayout *content = new QVBoxLayout;

}


void MetadataFrame::clear() {

}

void MetadataFrame::init() {
	auto project = qRelightApp->project();
}
