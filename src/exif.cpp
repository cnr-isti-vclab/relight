/* The implementation taken and extensively modified from Qt Exif implementation (now abandoned) */

#include "exif.h"
#include <QFile>
#include <QDataStream>
#include <QBuffer>

#include <iostream>
using namespace std;
void IfdHeader::parse(QDataStream &stream, quint32 startPos) {
	stream >> tag;
	stream >> type;
	stream >> count;

	quint32 pos = stream.device()->pos() + 4;
	int needsOffset = 1;
	if(tag != Exif::ExifIfdPointer && tag != Exif::GpsInfoIfdPointer) {
		if(type == Ascii || type == Undefined || type == Byte)
			needsOffset = 4;
		else if(type == Short)
			needsOffset = 2;
		if(count >= needsOffset) {
			quint32 offset;
			stream >> offset;
			stream.device()->seek(startPos + offset);
		}
	}
	if(tag == Exif::ExifIfdPointer)
		cout << type << endl;
	switch(type) {
	case Undefined:
	case Byte:
	case Ascii: {
		QByteArray bytes;
		bytes.resize(count);
		stream.readRawData( bytes.data(), count);

		if(type == Byte)
			value = QVariant(bytes);
		else
			value = QVariant(QString::fromLatin1(bytes.data(), bytes.size()-1));
		break;
	}
	default: {
		QList<QVariant> data;
		for(quint32 i = 0; i < count; i++) {
			switch(type) {
			case Short: {
				quint16 v;
				stream >> v;
//				cout << "Tag: " << tag << " " << v << endl;
				data.push_back(v);
				break;
			}
			case Long: {
				quint32 v;
				stream >> v;
				data.push_back(v);
//				cout << "Tag: " << tag << " " << v << endl;
				break;
			}
			case Rational: {
				quint32 v0, v1;
				stream >> v0 >> v1;
				data.push_back((double)v0/(double)v1);
//				cout << "Tag: " << tag << " " << v0 << " " << v1 << " -> " << (double)v0/(double)v1 << endl;
				break;
			}
			case SignedLong: {
				qint32 v;
				stream >> v;
				data.push_back(v);
//				cout << "Tag: " << tag << " " << v << endl;
				break;
			}
			case SignedRational: {
				qint32 v0, v1;
				stream >> v0 >> v1;
				data.push_back((double)v0/(double)v1);
//				cout << "Tag: " << tag << " " << v0 << " " << v1 << " -> " << (double)v0/(double)v1 << endl;
				break;
			}
			}
		}
		if(count == 1)
			value = QVariant(data[0]);
		else
			value = QVariant(data);
		break;
	}
	}
	stream.device()->seek(pos);
}

void Exif::parse(QString &filename) {
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		throw QString("Could not open file: " + filename);

	QDataStream stream(&file);
	stream.setByteOrder( QDataStream::BigEndian);
	if(file.read(2) != "\xFF\xD8")
		throw QString("Failed parsing file: " + filename);

	while(file.read(2) != "\xFF\xE1" ) {
		if(file.atEnd() )
			return; //no exif
		quint16 length;
		stream >> length;
		file.seek(file.pos() + length - 2);
		break;
	}
	quint16 length;
	stream >> length;
	if(file.read(4) != "Exif" )
		return;
		//throw QString("Failed parsing file: " + filename);
	file.read(2);


	quint32 startPos = file.pos();

	QByteArray byteOrder = file.read(2);
	if (byteOrder == "II") {
		stream.setByteOrder( QDataStream::LittleEndian );
	} else if (byteOrder == "MM") {
		stream.setByteOrder( QDataStream::BigEndian );
	} else {
		throw QString("Failed parsing file: " + filename);
	}
	quint16 id;
	quint32 offset;
	stream >> id;
	stream >> offset;
	if (id != 0x002A)
		throw QString("Failed parsing file: " + filename);

	file.seek(startPos + offset);

	//reading standard ifd
	readHeaders(stream, startPos);
	stream >> offset;

	//read extended ifd
	quint32 exifIfdPointer = (*this)[ExifIfdPointer].toUInt();
	if(exifIfdPointer) {
		stream.device()->seek(startPos + exifIfdPointer);
		readHeaders(stream, startPos);
	}


	//read gps ifd
	quint32 gpsIfdPointer = (*this)[GpsInfoIfdPointer].toUInt();
	if(gpsIfdPointer) {
		stream.device()->seek(startPos + gpsIfdPointer);
		readHeaders(stream, startPos);
	}

	//thumbnails!
	/*(if (offset) {
		device->seek(startPos + offset);
		// in readIfdHeaders(...) d->exifIfdValues will be corrupted for unknown reason
		QList<ExifIfdHeader> thumbnailHeaders = readIfdHeaders(stream);
		d->thumbnailIfdValues = readIfdValues<quint16>(stream, startPos, thumbnailHeaders);
		QExifValue jpegOffset = d->thumbnailIfdValues.value(JpegInterchangeFormat);
		QExifValue jpegLength = d->thumbnailIfdValues.value(JpegInterchangeFormatLength);
		//  qDebug() << "Nach: Exifversion = " << d->exifIfdValues.value(ExifVersion).toByteArray();
		if (jpegOffset.type() == QExifValue::Long && jpegOffset.count() == 1
				&& jpegLength.type() == QExifValue::Long && jpegLength.count() == 1)
		{
			device->seek(startPos + jpegOffset.toLong());
			d->thumbnailData = device->read( jpegLength.toLong() );
			d->thumbnailXResolution = d->thumbnailIfdValues.value(XResolution);
			d->thumbnailYResolution = d->thumbnailIfdValues.value(YResolution);
			d->thumbnailResolutionUnit = d->thumbnailIfdValues.value(ResolutionUnit);
			d->thumbnailOrientation = d->thumbnailIfdValues.value(Orientation);
		}
	} */




}

void Exif::readHeaders(QDataStream &stream, quint32 startPos) {
	quint16 count;
	stream >> count;
	for (quint16 i = 0; i < count; i++) {
		IfdHeader header;
		header.parse(stream, startPos);
		(*this)[header.tag] = header.value;
	}
}
