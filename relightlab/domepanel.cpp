#include "lightgeometry.h"
#include "relightapp.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include "domepanel.h"

#include <QComboBox>
#include <QVBoxLayout>

DomePanel::DomePanel(QWidget *parent): QFrame(parent) {

	QVBoxLayout *content = new QVBoxLayout(this);

	QLabel *title = new QLabel("<h2>Dome</h2>");
	content->addWidget(title);
	content->addSpacing(30);

	QComboBox *dome_combo = new QComboBox;
	content->addWidget(dome_combo);

	QHBoxLayout *columns = new QHBoxLayout();
	content->addLayout(columns);
}

void DomePanel::init() {
}

