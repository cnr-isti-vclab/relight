#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QGraphicsPixmapItem>
#include <QRect>
#include <QResizeEvent>
#include <QFuture>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <time.h>

#include "rtiexport.h"
#include "ui_rtiexport.h"
#include "imagecropper.h"
#include "httpserver.h"
#include "scripts.h"
#include "processqueue.h"
#include "../src/rti.h"
#include "../relight-cli/rtibuilder.h"

#include "normalstask.h"
#include "rtitask.h"

#include <functional>
#include <iostream>
#include "../src/jpeg_encoder.h"
using namespace std;

RtiExport::RtiExport(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RtiExport) {
    ui->setupUi(this);

    this->setWindowTitle("Relight- Export");

    ui->crop_frame->hide();
    connect(ui->basis, SIGNAL(currentIndexChanged(int)), this, SLOT(changeBasis(int)));
    connect(ui->planes, SIGNAL(valueChanged(int)), this, SLOT(changePlanes(int)));
    connect(ui->build_rti, SIGNAL(clicked()), this, SLOT(createRTI()));
    connect(ui->build_normals, SIGNAL(clicked()), this, SLOT(createNormals()));
//	connect(ui->close, SIGNAL(clicked()), this, SLOT(close()));
    connect(this, SIGNAL(rejected()), this, SLOT(close()));

    connect(ui->cropbuttonbox, SIGNAL(accepted()), this, SLOT(acceptCrop()));
    connect(ui->crop,          SIGNAL(clicked()),  this, SLOT(showCrop()));
    connect(ui->crop1,         SIGNAL(clicked()),  this, SLOT(showCrop()));

    connect(ui->cropreset, SIGNAL(clicked()), this, SLOT(resetCrop()));
    connect(ui->cropbuttonbox, SIGNAL(accepted()), this, SLOT(acceptCrop()));
    connect(ui->cropbuttonbox, SIGNAL(rejected()), this, SLOT(rejectCrop()));
    connect(ui->cropview, SIGNAL(areaChanged(QRect)), this, SLOT(cropChanged(QRect)));

    connect(ui->top, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));
    connect(ui->left, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));
    connect(ui->width, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));
    connect(ui->height, SIGNAL(valueChanged(int)), this, SLOT(updateCrop()));

    ui->aspect->addItem("None"); //0
    ui->aspect->addItem("Custom"); //1
    ui->aspect->addItem("Square"); //2
    ui->aspect->addItem("4:3 Photo"); //3
    ui->aspect->addItem("3:2 Postcard"); //4
    ui->aspect->addItem("16:10 Widescreen"); //5
    ui->aspect->addItem("16:9 Widescreen");    //6
    ui->aspect->addItem("2:3 Postcard portrait"); //7
    ui->aspect->addItem("3:4 Photo portrait"); //8
    connect(ui->aspect, SIGNAL(currentIndexChanged(int)), this, SLOT(setAspectRatio(int)));

    ui->cropview->hideHandle();
    ui->cropview->setBackgroundColor( Qt::lightGray );
    ui->cropview->setCroppingRectBorderColor( Qt::white);
    changeBasis(0);
}

void RtiExport::close() {
    server.stop();
    QDialog::close();
}

RtiExport::~RtiExport() {
    delete ui;
}

void RtiExport::setTabIndex(int index) {
    ui->export_frame->setCurrentIndex(index);
}

void RtiExport::setImages(QStringList _images) {
    images = _images;
}

void RtiExport::setCrop(QRect rect) {
    crop = rect;
    if(crop.isValid()) {
        ui->cropview->setCrop(rect);
        ui->cropview->showHandle();
    }
}

ostream& operator<<(ostream& os, const QRectF& r) {
    os << "top: " << r.top() << " left: " << r.left() << " width: " << r.width() << " height: " << r.height();
    return os;
}
ostream& operator<<(ostream& os, const QRect& r) {
    os << "top: " << r.top() << " left: " << r.left() << " width: " << r.width() << " height: " << r.height();
    return os;
}

void RtiExport::showImage(QPixmap pix) {
    ui->cropview->setImage(pix);
}

void RtiExport::changeBasis(int n) {
    switch(n) {
    case 0: //ptm
        ui->planes->setValue(18);
        break;
    case 1: //hsh 4
        ui->planes->setValue(12);
        break;
    case 2: //hsh 27
        ui->planes->setValue(27);
        break;
    case 3: //bilinear
        break;
    case 4: //rbf
        break;
    case 5: //yrbf
        break;
    }
    ui->chroma->setEnabled(n == 5 || n == 6);
    ui->planes->setEnabled(n >= 3);
}

void RtiExport::changePlanes(int n) {
    ui->chroma->setMaximum(n);
}

Rti::Type basis(int index) {
    //int b =  ui->basis->currentIndex();
    Rti::Type table[] =       { Rti::PTM, Rti::HSH, Rti::HSH, Rti::BILINEAR,  Rti::RBF, Rti::BILINEAR, Rti::RBF, Rti::DMD, Rti::SH, Rti::H };
    return table[index];
}

Rti::ColorSpace  colorSpace(int index) {
    //int b =  ui->basis->currentIndex();
    Rti::ColorSpace table[] = { Rti::RGB, Rti::RGB, Rti::RGB, Rti::MRGB,     Rti::MRGB, Rti::MYCC, Rti::MYCC };
    return table[index];

}


/*bool RtiExport::callback(std::string s, int n) {
    QString str(s.c_str());
    emit progressText(str);
    emit progress(n);

    if(cancel)
        return false;
    return true;
} */

/*void RtiExport::makeRti(QString output, QRect rect, Format format, bool means, bool normals, bool highNormals) {

    try {
        uint32_t ram = uint32_t(ui->ram->value());

        RtiBuilder builder;

        builder.samplingram = ram;
        builder.type         = basis(ui->basis->currentIndex());
        builder.colorspace   = colorSpace(ui->basis->currentIndex());
        builder.nplanes      = uint32_t(ui->planes->value());
        builder.yccplanes[0] = uint32_t(ui->chroma->value());
        //builder.sigma =

        if( builder.colorspace == Rti::MYCC) {
            builder.yccplanes[1] = builder.yccplanes[2] = (builder.nplanes - builder.yccplanes[0])/2;
            builder.nplanes = builder.yccplanes[0] + 2*builder.yccplanes[1];
        }
        builder.crop[0] = rect.left();
        builder.crop[1] = rect.top();
        builder.crop[2] =  rect.width();
        builder.crop[3] = rect.height();
        builder.imageset.images = images;
        builder.lights = builder.imageset.lights = lights;
        builder.imageset.initImages(path.toStdString().c_str());
        builder.imageset.crop(rect.left(), rect.top(), rect.width(), rect.height());

        builder.width  = builder.imageset.width;
        builder.height = builder.imageset.height;

        builder.savemeans = means;
        builder.savenormals = normals;

        cancel = false;
        std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->callback(s, n); };

        if(!builder.init(&callback)) {
            QMessageBox::critical(this, "We have a problem!", QString(builder.error.c_str()));
            return;
        }

        int quality= ui->quality->value();
        builder.save(output.toStdString(), quality);


        if(format == DEEPZOOM || format == TARZOOM) {
            for(uint32_t i = 0; i < builder.nplanes/3; i++) {
                callback("Deepzoom creation...", 100*i/((builder.nplanes/3)-1));
                Scripts::deepzoom(QString("%1/plane_%2").arg(output).arg(i), quality);
            }
            if(format == TARZOOM) {
                for(uint32_t i = 0; i < builder.nplanes/3; i++) {
                    callback("Tarzoom creation...", 100*i/((builder.nplanes/3)-1));
                    Scripts::tarzoom(QString("%1/plane_%2").arg(output).arg(i));
                }
            }
        }
    } catch(int status) {
        if(status == 1) { //was canceled.
            emit progressText("Canceling...");
            emit progress(0);
        }
    } catch(std::exception e) {
        QMessageBox::critical(this, "We have a problem 1!", e.what());
    } catch(const char *str) {
        QMessageBox::critical(this, "We have a problem 2 !",str);
    } catch(...) {
        cout << "Something went wrong!" << endl;
        QMessageBox::critical(this, "We have a problem 3!", "Unknown error!");

    }
} */

void RtiExport::createNormals() {
    // Get export location
    QString output = QFileDialog::getSaveFileName(this, "Select an output file for normal:");
    if(output.isNull()) return;
    if(!output.endsWith(".png"))
        output += ".png";

    // Get normal method
    unsigned int method = 0; //least squares
    if(ui->l2_solver->isChecked())
        method = 0;
    if(ui->sbl_solver->isChecked())
        method = 4;
    if(ui->rpca_solver->isChecked())
        method = 5;

    ProcessQueue &queue = ProcessQueue::instance();

    queue.addTask(new NormalsTask(path, output, crop, method));
    queue.start();

    close();
}

void RtiExport::createRTI() {
    QString output = QFileDialog::getSaveFileName(this, "Select an output directory", QString(), tr("Images (*.png)"));
    if(output.isNull()) return;
    createRTI1(output);
    close();
}

void RtiExport::createRTI1(QString output) {
	QString format;
	if(ui->formatRTI->isChecked())
		format = "rti";
	else if(ui->formatRelight->isChecked())
		format = "relight";
	else if(ui->formatDeepzoom->isChecked())
		format = "deepzoom";
	else if(ui->formatTarzoom->isChecked())
		format = "tarzoom";
	else if(ui->formatItarzoom->isChecked())
		format = "itarzoom";

	RtiTask *task = new RtiTask;
    task->input_folder = path;
	task->output = output;
    task->label = "RTI"; //should use

	uint32_t ram = uint32_t(ui->ram->value());
	task->addParameter("ram", Parameter::INT, ram);
	task->addParameter("path", Parameter::FOLDER, path);
    task->addParameter("output", Parameter::FOLDER, output);
    task->addParameter("images", Parameter::STRINGLIST, images);

	QList<QVariant> slights;
	for(auto light: lights)
		for(int k = 0; k < 3; k++)
			slights << QVariant(light[k]);
	task->addParameter("lights", Parameter::DOUBLELIST, slights);

	task->addParameter("type", Parameter::INT,  basis(ui->basis->currentIndex()));
	task->addParameter("colorspace", Parameter::INT, colorSpace(ui->basis->currentIndex()));
	task->addParameter("nplanes", Parameter::INT, ui->planes->value());
	
	int chroma = ui->chroma->value();
	int nplanes = ui->planes->value();
	if(chroma*3 > nplanes) {
		QMessageBox::critical(this, "Invalid value", "Chroma planes cannot be larger than number of planes / 3");
		return;
	}
	int yplanes = nplanes - chroma*2;
	task->addParameter("yplanes", Parameter::INT, yplanes);

	QRect rect = QRect(0, 0, 0, 0);
	if(ui->cropview->handleShown()) {
		rect = ui->cropview->croppedRect();
		task->addParameter("crop", Parameter::RECT,  rect);
	}

	task->addParameter("quality", Parameter::INT, ui->quality->value());


	QStringList steps;
    if (ui->chkDStretch->isChecked())
        steps << "dstretch";

    steps << "relight";

	if(format == "RTI")
		steps << "toRTI";

	if(format == "deepzoom")
		steps << "deepzoom";
	if(format == "tarzoom")
		steps << "deepzoom" << "tarzoom";
	if(format == "itarzoom")
		steps << "deepzoom" << "tarzoom" << "itarzoom";
	if(ui->openlime->isChecked())
        steps << "openlime";
	task->addParameter("steps", Parameter::STRINGLIST, steps);
	//task->addParameter("format", Parameter::STRING, format);
	//task->addParameter("openlime", Parameter::BOOL, ui->openlime->isChecked());

	ProcessQueue &queue = ProcessQueue::instance();
	queue.addTask(task);
}


/* OLD now unused, kept for progress bar and watcher */
/*void RtiExport::createRTI(QString output) {
    outputFolder = output;


    progressbar = new QProgressDialog("Building RTI...", "Cancel", 0, 100, this);
    progressbar->setAutoClose(false);
    progressbar->setAutoReset(false);
    progressbar->setWindowModality(Qt::WindowModal);
    progressbar->show();
    connect(progressbar, SIGNAL(canceled()), this, SLOT(cancelProcess()));

    QRect rect = QRect(0, 0, 0, 0);
    if(ui->cropview->handleShown()) {
        rect = ui->cropview->croppedRect();
    }

    Format format = RELIGHT;
    if(ui->formatDeepzoom->isChecked())
        format = DEEPZOOM;
    if(ui->formatTarzoom->isChecked())
        format = TARZOOM;


    QFuture<void> future = QtConcurrent::run([this, output, rect, format]() {
        this->makeRti(output, rect, format, true, true);
    } );

    watcher.setFuture(future);
    connect(&watcher, SIGNAL(finished()), this, SLOT(finishedProcess()));
    connect(this, SIGNAL(progress(int)), progressbar, SLOT(setValue(int)));
    connect(this, SIGNAL(progressText(const QString &)), progressbar, SLOT(setLabelText(const QString &)));
}

void RtiExport::cancelProcess() {
    cancel = true;
}

void RtiExport::finishedProcess() {
    if(progressbar == nullptr)
        return;
    progressbar->close();
    delete progressbar;
    progressbar = nullptr;

    if(viewAfter) {
        server.start(outputFolder);
        server.show();
    }
} */

void RtiExport::showCrop() {
    ui->cropview->showHandle();
    ui->export_frame->hide();
    ui->crop_frame->show();
}

void RtiExport::acceptCrop() {
    crop = ui->cropview->croppedRect();
    ui->crop_frame->hide();
    ui->export_frame->show();
}

void RtiExport::resetCrop() {
    ui->cropview->resetCrop();
    cropChanged(ui->cropview->croppedRect());
}

void RtiExport::rejectCrop() {
    ui->cropview->hideHandle();
    ui->export_frame->show();
    ui->crop_frame->hide();
}

void RtiExport::cropChanged(QRect rect) {
    ui->width->setValue(rect.width());
    ui->height->setValue(rect.height());
    ui->left->setValue(rect.left());
    ui->top->setValue(rect.top());
}

void RtiExport::updateCrop() {
    ui->cropview->setCrop(QRect(ui->left->value(), ui->top->value(),
                                ui->width->value(), ui->height->value()));
}

void RtiExport::setAspectRatio(int aspect) {
    ui->cropview->setProportionFixed(aspect > 0);

    double aspects[9][2] = { {1, 1}, {1, 1}, {1, 1}, {4, 3} , {3, 2}, {16, 10}, {16, 9}, {2, 3}, {3, 4} };

    switch(aspect) {
    case 0: return; //none
    case 1:
        aspects[1][0] = ui->aspect_width->value();
        aspects[1][1] =  ui->aspect_height->value();
        break;
    }

    double *s = aspects[aspect];
    ui->cropview->setProportion(QSizeF(s[0], s[1]));
}
