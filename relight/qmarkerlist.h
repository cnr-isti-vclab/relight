/* Taken from https://github.com/pierrebai/QtAdditions */

#ifndef ListMarker_H
#define ListMarker_H

#include <QFrame>
#include <vector>

/* similar to a QListWidget, but with widgets inside */

class Marker;
class QBoxLayout;

class ListMarker: public QFrame {
	Q_OBJECT
public:
	explicit ListMarker(QWidget *parent = nullptr);

	void clear();
	Marker* addItem(Marker* item, int index = -1);
	void removeItem(Marker* item);

	Marker *itemAt(const QPoint& pt) const;
	std::vector<Marker*> getItems(bool selected = false) const;
	std::vector<Marker*> getSelectedItems() const;

	void setSelected(Marker *marker);

protected:
	void mouseReleaseEvent(QMouseEvent* event) override;

	//void propagateMinimumDimension();

	QBoxLayout* _layout = nullptr;
};

#endif // QWIDGETLIST_H
