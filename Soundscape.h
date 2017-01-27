///////////////////////////////////////////////////////////////////////
//		Soundscape module ported from HL2                            //
//		Started 24.12.2016											 //
//		by Sgw32													 //
//		leave this comment if you want to use it in your own code!	 //
///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////

#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include <vector>
//#include "DotScene.h"
using namespace Ogre;

class Soundscape
{
public:
	Soundscape();
	~Soundscape();
	void init();

	void cleanup();

	void upd(const FrameEvent &evt);
};