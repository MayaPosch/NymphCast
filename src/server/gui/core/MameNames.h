#pragma once
#ifndef ES_CORE_MAMENAMES_H
#define ES_CORE_MAMENAMES_H

#include <string>
#include <vector>

class MameNames
{
public:

	static void       init       ();
	static void       deinit     ();
	static MameNames* getInstance();
	std::string       getRealName(const std::string& _mameName);
	const bool        isBios(const std::string& _biosName);
	const bool        isDevice(const std::string& _deviceName);

private:

	struct NamePair
	{
		std::string mameName;
		std::string realName;
	};

	typedef std::vector<NamePair> namePairVector;

	 MameNames();
	~MameNames();

	static MameNames* sInstance;

	namePairVector mNamePairs;
	std::vector<std::string> mMameBioses;
	std::vector<std::string> mMameDevices;

	const bool find(const std::vector<std::string> devices, const std::string& name);

}; // MameNames

#endif // ES_CORE_MAMENAMES_H
