/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "SeqScript.h"
//#include "Event.h"

SeqScript::SeqScript()
{
	time = 0;
	tim2=0;
	mInf=false;
	started =false;
	wait_before_start=0;
	disposed = false;
	first=false;
	elapsedt=0;
	lastt=0;
	time=0;
}

SeqScript::~SeqScript()
{
}

void SeqScript::assign(Vector3 pos,Real length,String name,bool freezebefore,bool unfreezeafter,bool splineanims,bool hidehud)
{
	mName = name;
	this->length=length;
	mStartPosition =pos;
	fb=freezebefore;
	
	unfreeze=unfreezeafter;
	//mCamera->setPosition(pos);
	mSceneMgr = global::getSingleton().getSceneManager();
        // Spline it for nice curves
        // Create a track to animate the camera's node
	hhud=hidehud;
}

String SeqScript::getname()
{
return mName;
}

void SeqScript::setWait(Real wait)
{
	wait_before_start = wait;
}

void SeqScript::addFrame(Real seconds)
{
	
}

void SeqScript::start()
{
		first=true;
		started =true;
		time=0;
}

void SeqScript::stop()
{
	if (started)
	{
		time=0;
started=false;
	}
}

bool SeqScript::frameStarted(const Ogre::FrameEvent &evt)
{
	// if we called START 
	/*if (started)
		mAnimState->addTime(evt.timeSinceLastFrame);

	if (!(tim2>wait_before_start))
		tim2=tim2+evt.timeSinceLastFrame;

	if (tim2>wait_before_start)
	{
		if (!started)
			start();
		time=time+evt.timeSinceLastFrame;
		if (!(time>length))
		{
			
		}
		if (time>length)
		{
			dispose();
		}
	}*/

	if (wait_before_start==0)
	{
		if (started)
		{


			/*if (first)
			{*/
			vector<Real>::iterator j;
			vector<String>::iterator k;
				for (i=0;i!=scripts.size();i++)
				{
					if (time>scripts_s[i])
					{
						LogManager::getSingleton().logMessage("SeqScript: Running lua script!");
						RunLuaScript(global::getSingleton().getLuaState(), scripts[i].c_str());
						scripts_s[i]=-1;
						//first=false;
					}
				}
				
				
				bool stop=true;
				
				while (stop)
				{
					stop=false;
				i=0;
				for (j=scripts_s.begin();j!=scripts_s.end();j++)
				{
					if (!stop)
					{
					if ((*j)==-1)
					{
						LogManager::getSingleton().logMessage("Erasing a lua script...");
						scripts_s.erase(j);
						k=scripts.begin();
						advance(k,i);
						scripts.erase(k);
						stop=true;
						
					}	
					i++;
					if (stop)
						break;
					}
				
				
				}
				}
			//}
			if (evt.timeSinceLastFrame<3.0f) //discrapency!!! 
				time+=evt.timeSinceLastFrame;
		//}

		if ((time>=length)&&!mInf)
		{
			dispose();
		}
		if ((time>length)&&mInf)
		{
			time=0.1f;
		}
		}
	}
	else
	{
		if (!(tim2>wait_before_start))
			tim2+=evt.timeSinceLastFrame;
		if (tim2>wait_before_start)
		{
			start();
			wait_before_start=0;
		}
	}
		
	return true;
}

void SeqScript::dispose()
{
	if (!disposed)
	{
	if (first)
	{
		LogManager::getSingleton().logMessage("Disposing SeqScript. It has been used ingame");
	started =false;
	disposed=true;
	}
	else
	{
		LogManager::getSingleton().logMessage("Disposing SeqScript. It wasn't used ingame");
	disposed=true;
	}
	}
}
