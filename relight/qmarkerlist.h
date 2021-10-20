/* Taken from https://github.com/pierrebai/QtAdditions */

#ifndef QMarkerList_H
#define QMarkerList_H

#include <QFrame>
#include <vector>

/* similar to a QListWidget, but with widgets inside */

class QMarker;
class QBoxLayout;

class QMarkerList: public QFrame {
	Q_OBJECT
public:
	explicit QMarkerList(QWidget *parent = nullptr);

	void clear();
	QMarker* addItem(QMarker* item, int index = -1);
	void removeItem(QMarker* item);

	QMarker *itemAt(const QPoint& pt) const;
	std::vector<QMarker*> getItems(bool selected = false) const;
	std::vector<QMarker*> getSelectedItems() const;

	void setSelected(QMarker *marker);

protected:
	void mouseReleaseEvent(QMouseEvent* event) override;

	//void propagateMinimumDimension();

	QBoxLayout* _layout = nullptr;
};

#endif // QWIDGETLIST_H
