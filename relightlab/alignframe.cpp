#include "alignframe.h"
#include "relightapp.h"
#include "markerdialog.h"
#include "alignrow.h"
#include "../src/align/align.h"
#include "../src/align/autoalign.h"
#include "../src/task.h"
#include "../src/project.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QResizeEvent>
#include <QFont>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QThread>

#include <algorithm>
#include <cmath>
#include <exception>

class AutoAlignTask: public Task {
public:
	Project &project;
	std::vector<QRect> regions;
	std::vector<AutoAlignCorrespondence> correspondences;

	explicit AutoAlignTask(Project &project): project(project) {
		label = "Automatically aligning images.";
		visible = false;
		owned = true;
	}

	void run() override {
		setStatus(RUNNING);
		try {
			std::vector<int> used;
			for(size_t i = 0; i < project.images.size(); ++i)
				if(!project.images[i].skip)
					used.push_back(int(i));
			if(used.size() < 2)
				throw QString("At least two images are required for automatic alignment.");

			AutoAlign aligner(project.imgsize);
			std::vector<QString> image_paths;
			image_paths.reserve(used.size());
			for(int index: used)
				image_paths.push_back(project.dir.filePath(
					project.images[size_t(index)].filename));
			std::function<bool(QString, int)> callback =
				[this](QString stage, int percent) { return progressed(stage, percent); };
			aligner.run(image_paths, &callback, &regions, &correspondences);
			if(status == STOPPED)
				return;
			for(AutoAlignCorrespondence &correspondence: correspondences) {
				correspondence.first = used[size_t(correspondence.first)];
				correspondence.second = used[size_t(correspondence.second)];
			}
			setStatus(DONE);
	} catch(const QString &message) {
		if(status != STOPPED) {
			error = message;
			setStatus(FAILED);
		}
	} catch(const std::exception &exception) {
		if(status != STOPPED) {
			error = QString::fromUtf8(exception.what());
			setStatus(FAILED);
		}
		}
	}
};

AlignFrame::AlignFrame(QWidget *parent): QFrame(parent) {

	stack = new QStackedWidget;

	{
		QFrame *align_rows = new QFrame;
		QVBoxLayout *content = new QVBoxLayout(align_rows);
		content->addSpacing(10);
		{
			QHBoxLayout *buttons = new QHBoxLayout;
			automatic_align_button = new QPushButton("Auto align");
			automatic_align_button->setProperty("class", "large");
			automatic_align_button->setMinimumWidth(200);
			automatic_align_button->setMaximumWidth(300);
			automatic_align_button->setToolTip("Automatically select textured regions and solve image translations.");
			connect(automatic_align_button, SIGNAL(clicked()), this, SLOT(automaticAlign()));
			buttons->addWidget(automatic_align_button);
			QPushButton *new_align = new QPushButton("New alignment...");
			new_align->setToolTip("Select a square region up to 400 px; its centered 40x40 patch is shown for verification.");
			connect(new_align, SIGNAL(clicked()), this, SLOT(newAlign()));
			buttons->addWidget(new_align);
			automatic_align_status = new QLabel;
			buttons->addWidget(automatic_align_status, 1);
			automatic_align_progress = new QProgressBar;
			automatic_align_progress->setRange(0, 100);
			automatic_align_progress->setFixedWidth(140);
			automatic_align_progress->hide();
			buttons->addWidget(automatic_align_progress);
			buttons->addStretch();
			content->addLayout(buttons);
		}
		summary_scene = new QGraphicsScene(this);
		summary_view = new QGraphicsView(summary_scene, align_rows);
		summary_view->setFixedHeight(200);
		summary_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		summary_view->setRenderHint(QPainter::Antialiasing);
		summary_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		summary_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		summary_view->setToolTip("Each 40x40 cell is an image; colored dots show the translation estimated from each alignment row.");
		content->addWidget(summary_view);
		{
			QFrame *aligns_frame = new QFrame;
			content->addWidget(aligns_frame, 0);
			aligns = new QVBoxLayout(aligns_frame);
		}
		content->addStretch(1);

		stack->addWidget(align_rows);
	}

	{
		marker_dialog = new MarkerDialog(MarkerDialog::ALIGN, this);
		marker_dialog->setWindowFlags(Qt::Widget);
		connect(marker_dialog, SIGNAL(accepted()), this, SLOT(okMarker()));
		connect(marker_dialog, SIGNAL(rejected()), this, SLOT(cancelMarker()));
		stack->addWidget(marker_dialog);
	}

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(stack);
}

void AlignFrame::clear() {
	stopAutomaticAlign();
	if(automatic_align_status)
		automatic_align_status->clear();
	if(provisional_align)
		delete provisional_align;
	provisional_align = nullptr;
	while(aligns->count() > 0) {
		QLayoutItem *item = aligns->takeAt(0);
		AlignRow *row =  dynamic_cast<AlignRow *>(item->widget());
		row->stopFinding();
		delete row;
	}
	if(summary_scene) {
		summary_scene->clear();
		summary_scene->setSceneRect(QRectF());
	}
}

void AlignFrame::automaticAlign() {
	if(!qRelightApp || !qRelightApp->m_project || automatic_align_task)
		return;

	updateSummary();
	automatic_align_button->setEnabled(false);
	automatic_align_status->setText("Starting automatic alignment...");
	automatic_align_progress->setValue(0);
	automatic_align_progress->show();

	automatic_align_task = new AutoAlignTask(qRelightApp->project());
	AutoAlignTask *task = automatic_align_task;
	connect(task, &Task::progress, this, [this, task](const QString &stage, int percent) {
		if(task != automatic_align_task)
			return;
		automatic_align_status->setText(stage);
		automatic_align_progress->setValue(percent);
	}, Qt::QueuedConnection);
	connect(task, &QThread::finished, this, [this, task]() {
		finishAutomaticAlign(task);
	}, Qt::QueuedConnection);
	task->start();
}

void AlignFrame::finishAutomaticAlign(AutoAlignTask *task) {
	if(!task || task != automatic_align_task)
		return;
	task->wait();
	if(task->status == Task::DONE) {
		Align *align = new Align(int(qRelightApp->project().images.size()));
		align->regions = task->regions;
		align->correspondences = task->correspondences;
		qRelightApp->project().aligns.push_back(align);
		AlignRow *row = addAlign(align);
		row->findAlignment(false);
		automatic_align_status->setText("Automatic alignment complete.");
		projectUpdate();
	} else if(task->status == Task::FAILED) {
		automatic_align_status->setText(task->error);
		updateSummary();
	} else {
		automatic_align_status->setText("Automatic alignment cancelled.");
	}
	automatic_align_progress->hide();
	automatic_align_button->setEnabled(true);
	automatic_align_task = nullptr;
	delete task;
}

void AlignFrame::stopAutomaticAlign() {
	if(!automatic_align_task)
		return;
	AutoAlignTask *task = automatic_align_task;
	automatic_align_task = nullptr;
	disconnect(task, nullptr, this, nullptr);
	task->stop();
	task->wait();
	delete task;
	if(automatic_align_button)
		automatic_align_button->setEnabled(true);
	if(automatic_align_progress)
		automatic_align_progress->hide();
}

void AlignFrame::init() {
	for(Align *align: qRelightApp->project().aligns) {
		AlignRow *row = addAlign(align);
		// Rebuild matches only for older projects that did not save correspondences.
		row->findAlignment(align->correspondences.empty());
	}
}

void AlignFrame::okMarker() {
	provisional_align->regions = {marker_dialog->getAlign().toRect()};
	Project &project = qRelightApp->project();
	project.aligns.push_back(provisional_align);
	AlignRow *row = addAlign(provisional_align);
	project.cleanAlignCache();
	row->findAlignment();

	provisional_align = nullptr;
	stack->setCurrentIndex(0);

}

void AlignFrame::cancelMarker() {
	delete provisional_align;
	provisional_align = nullptr;
	stack->setCurrentIndex(0);
}

void AlignFrame::newAlign() {
	Project &project = qRelightApp->project();
	int side = std::min(400, std::min(project.imgsize.width(), project.imgsize.height()));
	provisional_align = new Align(int(project.images.size()));
	provisional_align->regions.emplace_back(
		(project.imgsize.width() - side)/2,
		(project.imgsize.height() - side)/2,
		side,
		side
	);
	stack->setCurrentIndex(1);
	marker_dialog->setAlign(provisional_align);
}

void AlignFrame::projectUpdate() {
	auto &project = qRelightApp->project();
	project.computeOffsets();
	updateSummary();
}

AlignRow *AlignFrame::addAlign(Align *align) {
	AlignRow *row = new AlignRow(align);
	aligns->addWidget(row);

	connect(row, SIGNAL(removeme(AlignRow *)), this, SLOT(removeAlign(AlignRow *)));
	connect(row, SIGNAL(updated()),            this, SLOT(projectUpdate()));
	connect(row, &AlignRow::offsetsEdited, this, [this]() { updateSummary(); });
	updateSummary();
	return row;
}

void AlignFrame::removeAlign(AlignRow *row) {
	layout()->removeWidget(row);
	delete row;

	Project &project = qRelightApp->project();
	if(project.aligns.empty()) {
		project.align_offsets.assign(project.images.size(), QPointF(0, 0));
		project.needs_saving = true;
		updateSummary();
	} else {
		projectUpdate();
	}
}

void AlignFrame::resizeEvent(QResizeEvent *event) {
	QFrame::resizeEvent(event);
	updateSummary();
}

void AlignFrame::updateSummary() {
	if(!summary_scene || !summary_view || !qRelightApp)
		return;

	const Project &project = qRelightApp->project();
	const qreal cellSize = 80.0;
	const int imageCount = int(project.images.size());
	const int columns = std::max(1, summary_view->viewport()->width() / int(cellSize));
	const int rows = imageCount == 0 ? 0 : (imageCount + columns - 1) / columns;
	const int usedColumns = std::min(columns, imageCount);
	summary_scene->clear();
	summary_scene->setSceneRect(0, 0, usedColumns * cellSize, rows * cellSize);
	if(imageCount == 0)
		return;

	qreal maxTranslation = 0.0;
	for(size_t i = 0; i < project.align_offsets.size() && i < project.images.size(); ++i) {
		if(project.images[i].skip)
			continue;
		maxTranslation = std::max(maxTranslation,
			std::max(std::abs(project.align_offsets[i].x()), std::abs(project.align_offsets[i].y())));
	}
	const qreal translationScale = maxTranslation > 0.0 ? 14.0 / maxTranslation : 0.0;

	QFont numberFont;
	numberFont.setPixelSize(10);
	for(int i = 0; i < imageCount; ++i) {
		const int column = i % columns;
		const int row = i / columns;
		const qreal left = column * cellSize;
		const qreal top = row * cellSize;
		const QRectF cellRect(left, top, cellSize, cellSize);
		const bool skipped = project.images[size_t(i)].skip;
		QGraphicsRectItem *cellItem = summary_scene->addRect(
			cellRect, QPen(QColor("#B8C1CC"), 0.7),
			QBrush(skipped ? QColor("#E7E9EC") : QColor("#FFFFFF")));
		QString tooltip = QString("Image %1: %2%3")
			.arg(i + 1)
			.arg(project.images[size_t(i)].filename)
			.arg(skipped ? " (skipped)" : "");
		if(!skipped) {
			if(size_t(i) < project.align_offsets.size()) {
				const QPointF translation = project.align_offsets[size_t(i)];
				tooltip += QString("\nCombined alignment: dx %1 px, dy %2 px")
					.arg(translation.x(), 0, 'f', 2)
					.arg(translation.y(), 0, 'f', 2);
			}
		}
		cellItem->setToolTip(tooltip);

		QGraphicsTextItem *number = summary_scene->addText(QString::number(i + 1), numberFont);
		number->setDefaultTextColor(skipped ? QColor("#9299A1") : QColor("#333A43"));
		number->setPos(left + 3.0, top + 1.0);
		number->setAcceptedMouseButtons(Qt::NoButton);
		number->setZValue(3.0);

		if(skipped)
			continue;
		if(size_t(i) < project.align_offsets.size()) {
			const QPointF translation = project.align_offsets[size_t(i)];
			const qreal x = left + cellSize / 2.0 + translation.x() * translationScale;
			const qreal y = top + cellSize / 2.0 + translation.y() * translationScale;
			QGraphicsEllipseItem *dot = summary_scene->addEllipse(
				x - 2.0, y - 2.0, 4.0, 4.0,
				QPen(QColor("#202124"), 0.7), QBrush(QColor("#202124")));
			dot->setZValue(2.0);
		}
	}
}
