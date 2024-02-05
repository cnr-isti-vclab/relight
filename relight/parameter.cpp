#include "parameter.h"


QStringList Parameter::arguments() {
	QStringList list;
	if(!option.isNull())
		list << "-" + option;

	switch(type) {
	case FILENAME:
	case FOLDER:
	case STRING:
		list << value.toString();
		break;
	case SWITCH:
		if(!value.toBool())
			return QStringList();
		break;
	case BOOL:
		list << (value.toBool() ? "1" : "0");
		break;
	case INT:
		list << QString::number(value.toInt());
		break;
	case DOUBLE:
		list << QString::number(value.toDouble());
		break;
	case TMP_FILE: //Script will fill it in.
		break;
	default:
		throw "parameter type not supported";
		break;
	}
	return list;
}
