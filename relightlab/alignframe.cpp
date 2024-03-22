#include "alignframe.h"
#include "imageview.h"
#include "flowlayout.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

AlignFrame::AlignFrame(QWidget *parent): QFrame(parent) {
    //TODO: make a function for this to use in all frames. (inheritance?)
    setFrameStyle(QFrame::NoFrame);
    
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);

    QVBoxLayout *content = new QVBoxLayout(this);

	content->addWidget(image_view = new ImageView());
	//content->addWidget(new FlowLayout());
}
