#pragma once
#include <Ogre.h>
#include "func_door.h"
#include "HUD.h"

using namespace Ogre;

class SequenceLuaCallback:public Ogre::Singleton<SequenceLuaCallback>
{
public:
	SequenceLuaCallback();
	~SequenceLuaCallback();
	void setDoors(vector<func_door*> doors){mDoors=doors;}
	void openDor(String door)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				mDoors[i]->Fire("open");
			}
		}
	}
	void closeDor(String door)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				mDoors[i]->Fire("close");
			}
		}
	}
	void lockDor(String door)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				LogManager::getSingleton().logMessage("Locking door "+door);
				mDoors[i]->Fire("lock");
			}
		}
	}
	void unlockDor(String door)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				LogManager::getSingleton().logMessage("UnLocking door "+door);
				mDoors[i]->Fire("unlock");
			}
		}
	}
	void toggleDoor(String door)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				mDoors[i]->Fire("toggle");
			}
		}
	}
	
	void setAccDoor(String door,Real aac)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				mDoors[i]->setAcceleration(aac);
			}
		}
	}

	void setSpeedRot(String door,Real speed)
	{
		int i;
		for (i=0;i!=mDoors.size();i++)
		{
			if (mDoors[i]->getname()==door)
			{
				mDoors[i]->setRotSpeed(speed);
			}
		}
	}
	void flashT(String text)
	{
		HUD::getSingleton().flashPanel(text);
		
	}
	void startCutScene(String name);
	void processDotScene(String name);
private:
	vector<func_door*> mDoors;
	String pDoor;
};