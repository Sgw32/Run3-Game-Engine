#pragma once
#include "Ogre.h"
#include "Timeshift.h"
/*CLASS(Modulator) PISDOSINGLETON(Modulator)
BEGIN
ALL
CONSTRUCT(Modulator)
DESTROY(Modualtor)
FUNCTION(init)();
ROOT
SceneManager* mSceneMgr;
END*/

enum MODULATOR_TYPE
{
	INCREMENT_MODULATE,
	DECREMENT_MODULATE,
	GEOM_PROGRESSION
};

using namespace std;
using namespace Ogre;

//CLASS(Modulator) PISDOSINGLETON(Modulator)
//class Modulator:public Ogre::Singleton<Modulator>
//CLASS(Modulator) PISDOSINGLETON(Modulator)
/*class Modulator
BEGIN
ALL
CONSTRUCT(Modulator)
DESTROY(Modulator)
FUNCTION(init)();
FUNCTION_R(modulate_value)(int x,int type,int step,int param);
FUNCTION(modulate_l_color_value)(int x,int type,int step,int param);
FUNCTION(modulate_scenenode_pos)(SceneNode* node,Vector3 dest, Real sec);
bool frameStarted(const Ogre::FrameEvent &evt);
ROOT
SceneManager* mSceneMgr;
vector<SceneNode*> scenenodes_modulate;
vector<Vector3> scenenodes_dest;
vector<Real> scenenodes_sec;
END*/

class Modulator:public Ogre::Singleton<Modulator>
{
public:
	Modulator();
	~Modulator();
	void init();
	Real modulate_value(int x,int type,int step,int param);
	void modulate_l_color_value(Light* l, ColourValue c1,ColourValue c2);
	void modulate_scenenode_pos(SceneNode* node,Vector3 dest, Real sec);
	void modulate_FOV(Real start,Real end, Real sec);
	void modulate_SceneNode(SceneNode* a ,Real amp);
	void modulator_complete_fov();
	bool logic_erase(SceneNode* node);
	bool frameStarted(const Ogre::FrameEvent &evt);
private:
SceneManager* mSceneMgr;
vector<SceneNode*> scenenodes_modulate;
vector<Vector3> scenenodes_dest;
vector<Vector3> scenenodes_spos;
vector<Real> scenenodes_sec;
map<SceneNode*,Real> scenenodes_wave;
bool start_modulate_fov;
Real fovs,fovend,fovvel,cvel,ffe1,fs1,time;
};