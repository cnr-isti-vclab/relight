#ifndef ALIGNDIALOG_H
#define ALIGNDIALOG_H

#include <QDialog>
#include <vector>

namespace Ui {
class AlignDialog;
}

class Align;
class Project;
class AlignMarker;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsEllipseItem;

class AlignDialog : public QDialog {
	Q_OBJECT

public:
	explicit AlignDialog(AlignMarker *marker, Project *project, QWidget *parent = 0);
	~AlignDialog();

public slots:
	void resized();
protected:
	void resizeEvent(QResizeEvent *);

private:
	float scale = 2.0f;
	int margin = 2;
	AlignMarker *align;
	Project *project;
	Ui::AlignDialog *ui;
	QGraphicsScene *scene;
	std::vector<QGraphicsPixmapItem *> thumbs;
	std::vector<QGraphicsEllipseItem *> offsets;



};

#endif // ALIGNDIALO
