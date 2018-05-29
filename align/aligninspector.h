#ifndef ALILGNINSPECTOR_H
#define ALILGNINSPECTOR_H

#include <QDialog>
#include <vector>

namespace Ui {
	class AlignInspector;
}

class QGraphicsPixmapItem;
class QGraphicsScene;

class AlignInspector : public QDialog
{
	Q_OBJECT

public:
	int current_image = 0;
	std::vector<QPointF> offsets;
	std::vector<QImage> samples;
	size_t reference = 0;
	QGraphicsPixmapItem *item;
	QGraphicsScene *scene = NULL;


	explicit AlignInspector(std::vector<QPointF> _offsets, std::vector<QImage> _samples, size_t _reference, QWidget *parent = 0);
	~AlignInspector();

private slots:
	void setImage(int id);
	void previous();
	void next();
	void setOffset();
	void up();
	void down();
	void right();
	void left();

private:
	void update();
	Ui::AlignInspector *ui;
};

#endif // ALILGNINSPECTOR_H
