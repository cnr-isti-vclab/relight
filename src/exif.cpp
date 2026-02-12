/* The implementation taken and extensively modified from Qt Exif implementation (now abandoned) */

#include "exif.h"
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <cstring>

#include <iostream>
using namespace std;
void IfdHeader::parse(QDataStream &stream, quint32 startPos) {
	stream >> tag;
	stream >> type;
	stream >> count;

	quint32 pos = stream.device()->pos() + 4;
	uint32_t needsOffset = 1;
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
	//if(tag == Exif::ExifIfdPointer)
	//	cout << type << endl;
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

void Exif::parse(const QString &filename) {
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		throw QString("Could not open file: " + filename);

	QDataStream stream(&file);
	stream.setByteOrder( QDataStream::BigEndian);
	if(file.read(2) != "\xFF\xD8")
		throw QString("Not a jpeg file: " + filename);

	while(file.read(2) != "\xFF\xE1" ) {
		if(file.atEnd() )
			return; //no exif
		quint16 length;
		stream >> length;
		file.seek(file.pos() + length - 2);
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

namespace {

uint16_t exifReadU16(const QByteArray &b, int pos, bool little_endian) {
	if(pos + 1 >= b.size())
		return 0;
	const uint8_t *d = reinterpret_cast<const uint8_t *>(b.constData());
	if(little_endian)
		return uint16_t(d[pos]) | (uint16_t(d[pos + 1]) << 8);
	return (uint16_t(d[pos]) << 8) | uint16_t(d[pos + 1]);
}

uint32_t exifReadU32(const QByteArray &b, int pos, bool little_endian) {
	if(pos + 3 >= b.size())
		return 0;
	const uint8_t *d = reinterpret_cast<const uint8_t *>(b.constData());
	if(little_endian)
		return uint32_t(d[pos]) |
				(uint32_t(d[pos + 1]) << 8) |
				(uint32_t(d[pos + 2]) << 16) |
				(uint32_t(d[pos + 3]) << 24);
	return (uint32_t(d[pos]) << 24) |
			(uint32_t(d[pos + 1]) << 16) |
			(uint32_t(d[pos + 2]) << 8) |
			uint32_t(d[pos + 3]);
}

void exifWriteU16(QByteArray &b, int pos, bool little_endian, uint16_t value) {
	if(pos + 1 >= b.size())
		return;
	uint8_t *d = reinterpret_cast<uint8_t *>(b.data());
	if(little_endian) {
		d[pos] = uint8_t(value & 0xFF);
		d[pos + 1] = uint8_t((value >> 8) & 0xFF);
	} else {
		d[pos] = uint8_t((value >> 8) & 0xFF);
		d[pos + 1] = uint8_t(value & 0xFF);
	}
}

}

quint16 Exif::rotateOrientation(quint16 orientation, bool clockwise) {
	uint16_t right[9] = {8, 8, 7, 6, 5, 2, 1, 4, 3};
	uint16_t left[9] = {6, 6, 5, 8, 7, 4, 3, 2, 1};
	if(clockwise)
		return right[orientation -1];
	return left[orientation];
}

bool Exif::extractApp1Payload(const QString &filename, QByteArray &payload) {
	payload.clear();
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		return false;

	QByteArray data = file.readAll();
	if(data.size() < 4)
		return false;

	const uint8_t *d = reinterpret_cast<const uint8_t *>(data.constData());
	if(d[0] != 0xFF || d[1] != 0xD8)
		return false;

	int p = 2;
	while(p + 4 <= data.size()) {
		if(d[p] != 0xFF)
			break;
		while(p < data.size() && d[p] == 0xFF)
			p++;
		if(p >= data.size())
			break;

		uint8_t marker = d[p++];
		if(marker == 0xD9 || marker == 0xDA)
			break;
		if(p + 2 > data.size())
			break;

		uint16_t len = (uint16_t(d[p]) << 8) | uint16_t(d[p + 1]);
		if(len < 2 || p + len > data.size())
			break;

		if(marker == 0xE1 && len >= 8) {
			const char *seg = data.constData() + p + 2;
			if(std::memcmp(seg, "Exif\0\0", 6) == 0) {
				payload = data.mid(p + 2, len - 2);
				return true;
			}
		}
		p += len;
	}
	return false;
}

bool Exif::patchOrientation(QByteArray &payload, quint16 newOrientation) {
	if(payload.size() < 14)
		return false;
	if(std::memcmp(payload.constData(), "Exif\0\0", 6) != 0)
		return false;

	const int tiff = 6;
	const bool little_endian = (payload[tiff] == 'I' && payload[tiff + 1] == 'I');
	if(!(little_endian || (payload[tiff] == 'M' && payload[tiff + 1] == 'M')))
		return false;
	if(exifReadU16(payload, tiff + 2, little_endian) != 42)
		return false;

	uint32_t ifd0_offset = exifReadU32(payload, tiff + 4, little_endian);
	int ifd0 = tiff + int(ifd0_offset);
	if(ifd0 + 2 > payload.size())
		return false;

	uint16_t count = exifReadU16(payload, ifd0, little_endian);
	int entries = ifd0 + 2;
	for(uint16_t i = 0; i < count; i++) {
		int e = entries + int(i) * 12;
		if(e + 12 > payload.size())
			return false;

		uint16_t tag = exifReadU16(payload, e, little_endian);
		uint16_t type = exifReadU16(payload, e + 2, little_endian);
		uint32_t n = exifReadU32(payload, e + 4, little_endian);

		if(tag != Exif::Orientation)
			continue;

		if(type != IfdHeader::Short || n < 1)
			return false;

		if(n == 1) {
			exifWriteU16(payload, e + 8, little_endian, newOrientation);
			return true;
		}

		uint32_t off = exifReadU32(payload, e + 8, little_endian);
		int value_pos = tiff + int(off);
		if(value_pos + 2 > payload.size())
			return false;
		exifWriteU16(payload, value_pos, little_endian, newOrientation);
		return true;
	}

	return false;
}

bool Exif::injectApp1Payload(const QString &filename, const QByteArray &payload) {
	if(payload.isEmpty() || payload.size() + 2 > 65535)
		return false;

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		return false;
	QByteArray jpeg = file.readAll();
	file.close();

	if(jpeg.size() < 2 || (uint8_t)jpeg[0] != 0xFF || (uint8_t)jpeg[1] != 0xD8)
		return false;

	QByteArray out;
	out.reserve(jpeg.size() + payload.size() + 8);
	out.append(char(0xFF));
	out.append(char(0xD8));
	out.append(char(0xFF));
	out.append(char(0xE1));
	uint16_t len = uint16_t(payload.size() + 2);
	out.append(char((len >> 8) & 0xFF));
	out.append(char(len & 0xFF));
	out.append(payload);

	// Copy original JPEG content, skipping existing APP1 Exif segments.
	const uint8_t *d = reinterpret_cast<const uint8_t *>(jpeg.constData());
	int p = 2;
	while(p + 1 < jpeg.size()) {
		if(d[p] != 0xFF) {
			// Entropy-coded data or malformed stream: copy remaining bytes as-is.
			out.append(jpeg.constData() + p, jpeg.size() - p);
			break;
		}

		int marker_start = p;
		while(p < jpeg.size() && d[p] == 0xFF)
			p++;
		if(p >= jpeg.size()) {
			out.append(jpeg.constData() + marker_start, jpeg.size() - marker_start);
			break;
		}

		uint8_t marker = d[p++];

		// Standalone markers without length field.
		if(marker == 0xD8 || marker == 0xD9 || (marker >= 0xD0 && marker <= 0xD7) || marker == 0x01) {
			out.append(jpeg.constData() + marker_start, p - marker_start);
			if(marker == 0xD9)
				break;
			continue;
		}

		if(p + 2 > jpeg.size()) {
			out.append(jpeg.constData() + marker_start, jpeg.size() - marker_start);
			break;
		}

		uint16_t seg_len = (uint16_t(d[p]) << 8) | uint16_t(d[p + 1]);
		if(seg_len < 2 || p + seg_len > jpeg.size()) {
			out.append(jpeg.constData() + marker_start, jpeg.size() - marker_start);
			break;
		}

		bool skip_segment = false;
		if(marker == 0xE1 && seg_len >= 8) {
			const char *seg = jpeg.constData() + p + 2;
			if(std::memcmp(seg, "Exif\0\0", 6) == 0)
				skip_segment = true;
		}

		if(!skip_segment)
			out.append(jpeg.constData() + marker_start, (p + seg_len) - marker_start);

		if(marker == 0xDA) {
			// Start of Scan: copy compressed image data to the end unchanged.
			out.append(jpeg.constData() + p + seg_len, jpeg.size() - (p + seg_len));
			break;
		}

		p += seg_len;
	}

	if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	if(file.write(out) != out.size()) {
		file.close();
		return false;
	}
	file.close();
	return true;
}

