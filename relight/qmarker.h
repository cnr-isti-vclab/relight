#ifndef QMARKER_H
#define QMARKER_H

#include <QWidget>

class QLabel;
class QToolButton;
class QGraphicsScene;
class QGraphicsView;

class QMarker : public QWidget {
	Q_OBJECT
	Q_PROPERTY(bool selected MEMBER selected)

public:

	QLabel *label = nullptr;
	QToolButton *edit = nullptr;
	QToolButton *remove = nullptr;
	QGraphicsView *view = nullptr;
	QGraphicsScene *scene = nullptr;

	bool selected = false;
	bool editing = false;

	explicit QMarker(QGraphicsView *view, QWidget *parent = nullptr);
	~QMarker();


	virtual void setSelected(bool value = true);
	virtual void setEditing(bool value = true) { editing = value;  }
	virtual void cancelEditing() {}
	virtual void click(QPointF) {}
	virtual void doubleClick(QPointF) {}

public slots:
	virtual void onEdit() {}

protected:
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;

	void HighlightBackground(bool high);


};


#endif // QMarker_H
