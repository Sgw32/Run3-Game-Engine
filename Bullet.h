/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
///////////////This class brings bullets in Run3 Game Engine/////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <OgreNewt.h>
#include <Ogre.h>
//#include "global.h"

class Bullet
{
public:
	Bullet();
	~Bullet();
	void init(Ogre::Vector3 pos,Ogre::Vector3 dir,Ogre::Real force,OgreNewt::World* world,Ogre::SceneManager* scene);
	void start();
	void bullet_force_callback(OgreNewt::Body* me);
private:
	OgreNewt::Body* bod;
	OgreNewt::World* mWorld;
	Ogre::Vector3 mPos;
	Ogre::Vector3 mDir;
	Ogre::Real mForce;
	Ogre::Vector3 inertia;
	bool startadd;
};