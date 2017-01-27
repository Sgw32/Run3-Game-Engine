/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
#include "Player.h"
#include "global.h"
#include "HUD.h"
#include "MusicPlayer.h"
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>

class CutScene:public FrameListener
{
public:
	CutScene();
	~CutScene();
	void assign(Vector3 pos,Real length,String name,bool freezebefore,bool unfreezeafter,bool splineanims,bool hidehud);
	String getname();
	void addFrame(Vector3 pos,Real second,Quaternion quat);
	//void setSpeed(Real speed);
	void start();
	
	void setMusicSync(String fileName,Real length)
	{
		musicSync=true;
		musicName=fileName;
		musicLength=length;
		speedcoef=2.0f; //Because music doesnt't support 3.0f
	}
	void setNoDestroy(bool noD)
	{
		mNoDestroy=noD;
	}
	void enableSkipAbility()
	{
		skipMessage=true;
	}
	void setFasterCoefficient(Real k)
	{
		if ((k>2.0f)&&(musicSync))
			speedcoef=2.0f;
		else
			speedcoef = k;
	}
	void setInf(bool inf)
	{mInf=inf;
	}
	void addLuaRun(Real second,String script)
	{
		scripts.push_back(script);
		scripts_s.push_back(second);
	}
	void setWait(Real wait);
	void stop();
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	void dispose();
	void dispose2();
private:
	AnimationState* mAnimState;
	Animation* anim;
	Camera* mCamera;
	bool started,unfreeze,disposed,first,hhud;
	bool musicSync;
	Real timeBefore;
	Real musicLength;
	String musicName;
	bool fb;
	int i;
	 NodeAnimationTrack* track;
	 TransformKeyFrame* key;
	Player* mPlayer;
	Real time,length,wait_before_start,tim2,elapsedt,lastt;
	bool mInf;
	bool skipMessage;
	bool skipping;
	bool mNoDestroy;
	Real speedcoef;
	String mName;
	SceneManager* mSceneMgr;
	Vector3 mStartPosition;
	SceneNode* camera_scenenode;
	vector<TransformKeyFrame*> frames;
	vector<String> scripts;
	vector<Real> scripts_s;
};