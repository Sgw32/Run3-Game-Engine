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
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>

class SeqScript
{
public:
	SeqScript();
	~SeqScript();
	void assign(Vector3 pos,Real length,String name,bool freezebefore,bool unfreezeafter,bool splineanims,bool hidehud);
	String getname();
	void addFrame(Real second);
	//void setSpeed(Real speed);
	void start();
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
private:
	bool started,unfreeze,disposed,first,hhud;
	bool fb;
	int i;
	 NodeAnimationTrack* track;
	 TransformKeyFrame* key;
	Real time,length,wait_before_start,tim2,elapsedt,lastt;
	bool mInf;
	//Real time;
	String mName;
	SceneManager* mSceneMgr;
	Vector3 mStartPosition;
	SceneNode* camera_scenenode;
	vector<String> scripts;
	vector<Real> scripts_s;
};