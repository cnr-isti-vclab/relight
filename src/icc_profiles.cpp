#include "icc_profiles.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QString>

namespace {

std::vector<uint8_t> loadDisplayP3Profile() {
	QFile file(":/Display P3.icc");
	if(!file.open(QIODevice::ReadOnly)) {
		throw QString("Unable to open Display P3 ICC profile" + file.fileName() + ':' + file.errorString());
	}
	QByteArray data = file.readAll();
	return std::vector<uint8_t>(data.begin(), data.end());
}

std::vector<uint8_t> createSRGBProfileData() {
	cmsHPROFILE profile = cmsCreate_sRGBProfile();
	cmsUInt32Number size = 0;
	cmsSaveProfileToMem(profile, nullptr, &size);
	std::vector<uint8_t> data(size);
	cmsSaveProfileToMem(profile, data.data(), &size);
	cmsCloseProfile(profile);
	return data;
}

}

const std::vector<uint8_t> &ICCProfiles::displayP3Data() {
	static std::vector<uint8_t> data = loadDisplayP3Profile();
	return data;
}

cmsHPROFILE ICCProfiles::openDisplayP3Profile() {
	const auto &data = displayP3Data();
	if(data.empty())
		return nullptr;
	return cmsOpenProfileFromMem(data.data(), data.size());
}

const std::vector<uint8_t> &ICCProfiles::sRGBData() {
	static std::vector<uint8_t> data = createSRGBProfileData();
	return data;
}
