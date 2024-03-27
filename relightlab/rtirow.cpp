#include "rtirow.h"
#include "helpbutton.h"

#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>

RtiRow::RtiRow(QWidget *parent): QFrame(parent) {
	row = new QHBoxLayout(this);
    
}

PtmRow::PtmRow(QWidget *parent): RtiRow(parent) {
	row->addWidget(new HelpLabel("PTM: Polynomial Texture Maps", "rti/ptm"), 1);
	row->addStretch(1);
    QCheckBox *lrgb = new QCheckBox("LRGB");
	row->addWidget(lrgb, 1);


	row->addWidget(new QPushButton("Create"), 1);
	row->addWidget(new QPushButton("Create .ptm"), 1);
}


HshRow::HshRow(QWidget *parent): RtiRow(parent) {
	row->addWidget(new HelpLabel("HSH: emiSpherical Harmonics", "rti/hsh"), 1);
    QComboBox *nharmonics = new QComboBox();
    nharmonics->addItem("4 harmonics");
    nharmonics->addItem("9 harmonics");

	row->addWidget(nharmonics, 1);

    QCheckBox *lrgb = new QCheckBox("LRGB");
	row->addWidget(lrgb, 1);

	row->addWidget(new QPushButton("Create"), 1);
	row->addWidget(new QPushButton("Create .rti"), 1);
}

RbfRow::RbfRow(QWidget *parent): RtiRow(parent) {
	row->addWidget(new HelpLabel("RBF: Radial Basis Functions", "rti/rbf"), 1);
    QComboBox *nplanes = new QComboBox();
	nplanes->addItem("9 planes");
	nplanes->addItem("12 planes ");
	nplanes->addItem("15 planes ");
	nplanes->addItem("18 planes ");
	nplanes->addItem("21 planes ");
	nplanes->addItem("24 planes ");
	nplanes->addItem("27 planes ");

	row->addWidget(nplanes, 1);

    QComboBox *chroma = new QComboBox();
    chroma->addItem("0 chroma reserved");
    chroma->addItem("1 chroma reserved");
    chroma->addItem("2 chroma reserved");
    chroma->addItem("3 chroma reserved");
    
	row->addWidget(chroma, 1);

	row->addWidget(new QPushButton("Create"), 2);
}
