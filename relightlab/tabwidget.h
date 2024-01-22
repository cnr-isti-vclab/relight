#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QStyleOptionTab>
#include <QStylePainter>
#include <QTabBar>
#include <QTabWidget>

class TabBar: public QTabBar {
public:
	QSize tabSizeHint(int index) const;

protected:
	void paintEvent(QPaintEvent * /*event*/);
};

class TabWidget: public QTabWidget {
public:
	TabWidget(QWidget *parent = nullptr);
};


#endif // TABWIDGET_H
