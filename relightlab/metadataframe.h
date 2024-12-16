#ifndef METADATAFRAME_H
#define METADATAFRAME_H

#include <QFrame>

class MetadataFrame: public QFrame {
public:
	MetadataFrame(QWidget *parent = nullptr);
	void clear();
	void init();

};

#endif // METADATAFRAME_H
