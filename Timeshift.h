#pragma once
#include <Ogre.h>

using namespace Ogre;

class Timeshift : public Singleton<Timeshift>
{
public:
	Timeshift();
	~Timeshift();
	void init();
	void setTimeK(Real k);
	void toggleTStop();

	Real getTimeK(void){return mK;};
private:
	SceneNode* nod;
	Entity* ent;
	Real mK;
};
