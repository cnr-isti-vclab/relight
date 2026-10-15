#include "alignframe.h"
#include "relightapp.h"
#include "markerdialog.h"
#include "alignrow.h"
#include "../src/align/align.h"
#include "../src/project.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
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

#include <algorithm>
#include <cmath>

AlignFrame::AlignFrame(QWidget *parent): QFrame(parent) {

	stack = new QStackedWidget;

	{
		QFrame *align_rows = new QFrame;
		QVBoxLayout *content = new QVBoxLayout(align_rows);
		content->addSpacing(10);
		{
			QHBoxLayout *buttons = new QHBoxLayout;
			QPushButton *new_align = new QPushButton("New alignment...");
			new_align->setProperty("class", "large");
			new_align->setMinimumWidth(200);
			new_align->setMaximumWidth(300);
			new_align->setToolTip("Select a square region up to 400 px; its centered 40x40 patch is shown for verification.");
			connect(new_align, SIGNAL(clicked()), this, SLOT(newAlign()));
			buttons->addWidget(new_align);
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
		summary_view->setToolTip("Each 20x20 cell is an image; colored dots show its translation for each alignment region.");
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

void AlignFrame::init() {
	for(Align *align: qRelightApp->project().aligns) {
		AlignRow *row = addAlign(align);
		row->findAlignment(false);
	}
}

void AlignFrame::okMarker() {
	provisional_align->rect = marker_dialog->getAlign().toRect();
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
	provisional_align->rect = QRect(
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
	updateSummary();
	return row;
}

void AlignFrame::removeAlign(AlignRow *row) {

	layout()->removeWidget(row);
	delete row;

	projectUpdate();
}

void AlignFrame::resizeEvent(QResizeEvent *event) {
	QFrame::resizeEvent(event);
	updateSummary();
}

void AlignFrame::updateSummary() {
	if(!summary_scene || !summary_view || !qRelightApp)
		return;

	const Project &project = qRelightApp->project();
	const qreal cellSize = 40.0;
	const int imageCount = int(project.images.size());
	const int columns = std::max(1, summary_view->viewport()->width() / int(cellSize));
	const int rows = imageCount == 0 ? 0 : (imageCount + columns - 1) / columns;
	const int usedColumns = std::min(columns, imageCount);
	summary_scene->clear();
	summary_scene->setSceneRect(0, 0, usedColumns * cellSize, rows * cellSize);
	if(imageCount == 0)
		return;

	static const QColor palette[] = {
		QColor("#4E79A7"), QColor("#F28E2B"), QColor("#E15759"),
		QColor("#76B7B2"), QColor("#59A14F"), QColor("#EDC948"),
		QColor("#B07AA1"), QColor("#9C755F"), QColor("#FF9DA7"),
		QColor("#79706E")
	};
	constexpr int paletteSize = int(sizeof(palette) / sizeof(palette[0]));

	qreal maxTranslation = 0.0;
	for(const Align *align: project.aligns) {
		for(size_t i = 0; i < align->offsets.size() && i < project.images.size(); ++i) {
			if(project.images[i].skip)
				continue;
			maxTranslation = std::max(maxTranslation,
				std::max(std::abs(align->offsets[i].x()), std::abs(align->offsets[i].y())));
		}
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
			tooltip += "\nTranslations:";
			for(size_t alignmentIndex = 0; alignmentIndex < project.aligns.size(); ++alignmentIndex) {
				const Align *align = project.aligns[alignmentIndex];
				if(size_t(i) >= align->offsets.size())
					continue;
				const QPointF translation = align->offsets[size_t(i)];
				tooltip += QString("\nAlignment %1: dx %2 px, dy %3 px")
					.arg(alignmentIndex + 1)
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
		for(size_t a = 0; a < project.aligns.size(); ++a) {
			const Align *align = project.aligns[a];
			if(size_t(i) >= align->offsets.size())
				continue;

			const QPointF translation = align->offsets[size_t(i)];
			const qreal x = left + cellSize / 2.0 + translation.x() * translationScale;
			const qreal y = top	+ cellSize / 2.0  + translation.y() * translationScale;
			const QColor color = palette[a % paletteSize];
			QGraphicsEllipseItem *dot = summary_scene->addEllipse(
				x - 1.5, y - 1.5, 3.0, 3.0,
				QPen(color.darker(140), 0.6), QBrush(color));
			dot->setZValue(2.0 + a * 0.001);
		}
	}
}
