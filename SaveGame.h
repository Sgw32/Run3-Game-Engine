#pragma once
#include <Ogre.h>
#include "tinyxml.h"

using namespace Ogre;
using namespace std;

// Static Game Save File
//By Sgw32 2014 (after HDD crash)

class SaveGame: public Singleton<SaveGame>
{
public:
	SaveGame();
	~SaveGame();
	void init();
	void saveLast(String mapName);
	void restoreRegisters();
	void loadRegisters();
	void restoreHealthParameters(Real mul, Real maxHealth);
	String openLast();
private:
	Real rx1;
	Real rx2;
	Real ry1;
	Real ry2;
	Real rz1;
	Real rz2;
	Real a;
	Real ra;
	Real rb;
	Real rc;

	bool first;
	TiXmlDocument   *SequenceDoc;
	TiXmlElement   *SequenceRoot;
};


