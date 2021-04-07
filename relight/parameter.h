#ifndef PARAMETER_H
#define PARAMETER_H

#include <QString>
#include <QMap>
#include <QList>
#include <QVariant>

class Parameter {
public:
	QString id;
	QString label;

	enum Type { STRING, STRINGLIST, SWITCH, BOOL, INT, DOUBLE, DOUBLELIST, RECT, FILENAME, TMP_FILE, FOLDER };
	Type type;

	QString option; //eg. 'c' => -c <value>
	QVariant value;

	Parameter() {}
	Parameter(const QString &_id, Type _type, QVariant _value = QVariant()): id(_id), type(_type), value(_value) {}
	QStringList arguments();
};

#endif // PARAMETER_H
