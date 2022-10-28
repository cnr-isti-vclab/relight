#include "ui_dstretchdialog.h"
#include "dstretchdialog.h"
#include <dstretchtask.h>
#include <QFileDialog>
#include <QMessageBox>
#include <processqueue.h>
#include <qlineedit.h>
#include <QIntValidator>

DStretchDialog::DStretchDialog(QWidget *parent) :
    QDialog(parent),
    m_Ui(new Ui::dstretchdialog)
{
    m_Ui->setupUi(this);
    // MAX: 3200
    m_Ui->textMinSamples->setValidator(new QIntValidator(1000, 10000000, this));

    connect(m_Ui->buttonBrowse, SIGNAL(clicked()), this, SLOT(buttonBrowseClicked()));
    connect(m_Ui->buttonGenerate, SIGNAL(clicked()), this, SLOT(buttonGenerateClicked()));
    connect(m_Ui->textMinSamples, &QLineEdit::textChanged, [=](QString text){m_NSamples = text.toUInt();});
}

DStretchDialog::~DStretchDialog()
{
    delete m_Ui;
}

void DStretchDialog::buttonBrowseClicked()
{
    QString input = QFileDialog::getOpenFileName(this, "Select files to d-stretch", "", "*.jpg");
    if (input == nullptr) return;

    m_Ui->textFilePath->setText(input);
    m_InputFile = input;
}

void DStretchDialog::buttonGenerateClicked()
{
    QString output = QFileDialog::getSaveFileName(this, "Select d-stretch output file", m_OutputFile, "*.jpg");
    if (output == nullptr || output.compare("") == 0) {
        QMessageBox::critical(this, "No output folder", "Cannot d-stretch, unspecified output file");
        return;
    }
    if (m_InputFile.compare("") == 0) {
        QMessageBox::critical(this, "No input file", "Cannot d-stretch, unspecified input file");
        return;
    }

    m_OutputFile = output;

	DStretchTask* task = new DStretchTask(this);
    QStringList files;

    files << m_InputFile;

    task->output = output;
    task->input_folder = m_InputFile;
    task->addParameter("min_samples", Parameter::INT, m_NSamples);

    ProcessQueue& instance = ProcessQueue::instance();
    instance.addTask(task);

    close();
}
