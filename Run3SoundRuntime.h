///////////////////////////////////////////////////////////////////////
//		Sound class for Run3										 //
//		Started 25.02.2010											 //
//		by Sgw32													 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include "SoundManager.h"
#include "global.h"

using namespace Ogre;
using namespace std;

enum COLLIDE_SOUND_TYPE
{
	CONCRETE,
	METAL,
	BODY
};

class AmbientSound
{
public:
	AmbientSound(SoundManager* mgr,String sound, Vector3 pos,Real dist, Real fallofdist)
	{
		sMgr=mgr;
		mSound=sound;
		mPos=pos;
		mDist=dist;
		exists=false;
		disabled=false;
		mFallOfDist=fallofdist;
		mMinGain=-1.0f;
		phase=0;
	}
	AmbientSound(SoundManager* mgr,String sound, Vector3 pos,Real dist, Real fallofdist,Real minGain)
	{
		sMgr=mgr;
		mSound=sound;
		mPos=pos;
		mDist=dist;
		exists=false;
		disabled=false;
		mFallOfDist=fallofdist;
		mMinGain=minGain;
		phase=0;
	}
	~AmbientSound(){};

	bool getAvailability() //should it sound or not
	{
		if ((global::getSingleton().getPlayer()->get_location()-mPos).length()<mDist)
			return true;
		return false;
	}

	void spawn()
	{
		if (disabled)
			return;
		if (!exists)
		{
		sMgr->loadAudio(mSound,&mSoundAL,true);
		if (mMinGain>0)
		{
			sMgr->setSound(mSoundAL,mPos,Vector3(1,1,1),Vector3(1,1,1),mDist,false,false,mMinGain,1,mFallOfDist);
		}
		else
		{
			sMgr->setSound(mSoundAL,mPos,Vector3(1,1,1),Vector3(1,1,1),mDist,false,false,mDist,1,mFallOfDist);
		}
		restorePhase();
		sMgr->playAudio(mSoundAL,false);
		exists=true;
		}
	}
	
	void disableAmbientSound()
	{
		destroy();
		disabled=true;
	}
	void enableAmbientSound()
	{
		disabled=false;
	}
	void setName(String name)
	{
		mName=name;
	}
	String getName()
	{
		return mName;
	}
	void destroy()
	{
		
		if (disabled)
			return;
		if (exists)
		{
			phase = sMgr->getPhase(mSoundAL);
			exists=false;
			sMgr->releaseAudio(mSoundAL);
		}
	}

	void restorePhase()
	{
		if (!phase) 
			return;
		if (!sMgr->setPhase(mSoundAL,phase))
			LogManager::getSingleton().logMessage("Error restoring phase!");
		LogManager::getSingleton().logMessage("Phase restored to "+ StringConverter::toString(phase));
	}

	unsigned int getSoundAL()
	{
		return mSoundAL;
	}
private:
	bool disabled;
	bool exists;
	SoundManager* sMgr;
	unsigned int mSoundAL;
	String mSound;
	String mName;
	Vector3 mPos;
	Real mDist, mFallOfDist;
	Real mMinGain;
	Real phase;
};

class Run3SoundRuntime:public Singleton<Run3SoundRuntime>
{
public:
	Run3SoundRuntime();
	~Run3SoundRuntime();
	void init();
	void emitSound(String sound, Real duration, bool loop);
	void emitSound(String sound, Real duration, bool loop,Vector3 pos,Real dist,Real fallofDist);
	void emitSound(String sound, Real duration, bool loop,Vector3 pos,Real minGain,Real rolloff,Real fallofDist);
	void emitSoundOp(String sound, Real duration, bool loop,Vector3 pos,Real dist,Real fallofDist);
	void addAmbientSound(String sound, Vector3 pos, Real dist, Real fallofDist);
	void addAmbientSound(String name,String sound, Vector3 pos, Real dist, Real fallofDist);
	void addAmbientSound(String name,String sound, Vector3 pos, Real dist, Real fallofDist,Real gain);
	void clearAmbientSounds();
	void clearAllAmbientSounds();
	void clearAllSounds();
	void disableAmbientSound(String name);
	void enableAmbientSound(String name);
	void emitCollideSound(int type, Vector3 pos);
	bool frameStarted(const Ogre::FrameEvent &evt);
	vector<String> fileNames;
private:
	SoundManager* sMgr;
	Player* ply;
	vector<String> collides;
	//vector<String> fileNames;
	vector<Real> durations;
	vector<Real> dists;
	vector<Vector3> poss;
	vector<bool> optim;
	vector<AmbientSound> asounds;
	unsigned int soundsInUse;
	vector<unsigned int> sounds;
};