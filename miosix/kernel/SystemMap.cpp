#include "SystemMap.h"

using namespace std;

namespace miosix {

static Mutex initMapMutex;//To guard initialization of the singleton

SystemMap::SystemMap() {
}

SystemMap::SystemMap(const SystemMap& orig) {
}

SystemMap::~SystemMap() {
}

SystemMap &SystemMap::instance(){
	static SystemMap *pSingleton = 0;
	{
		Lock<Mutex> lock(initMapMutex);
		
		if(pSingleton == 0)
			pSingleton = new SystemMap();
	}
	
	return *pSingleton;
}

void SystemMap::addElfProgram(const char* name, const unsigned int *elf, unsigned int size){	
	string sName(name);
	if(mPrograms.find(sName) == mPrograms.end())
		mPrograms.insert(make_pair(sName, make_pair(elf, size)));
}

void SystemMap::removeElfProgram(const char* name){
	string sName(name);
	ProgramsMap::iterator it = mPrograms.find(sName);
	
	if(it != mPrograms.end())
		mPrograms.erase(it);
}

pair<const unsigned int*, unsigned int>  SystemMap::getElfProgram(const char* name) const {
	ProgramsMap::const_iterator it = mPrograms.find(string(name));
	
	if(it == mPrograms.end())
		return make_pair<const unsigned int*, unsigned int>(0, 0);
	
	return it->second;
}

unsigned int SystemMap::getElfCount() const{
	return mPrograms.size();
}

/*unsigned int SystemMap::hashString(const char* pStr) const {
	unsigned int hash = 0;
        
	for(; *pStr != '\0'; ++pStr){
		hash += *pStr;
		hash += (hash << 10);
		hash += (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}
*/

}