/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "CutScene.h"
//#include "Event.h"

CutScene::CutScene()
{
	time = 0;
	tim2=0;
	mInf=false;
	started =false;
	wait_before_start=0;
	skipping=false;
	disposed = false;
	first=false;
	elapsedt=0;
	lastt=0;
	speedcoef=3;
	skipMessage=false;
	musicSync=false;
	timeBefore=0;
	mAnimState=0;
	mNoDestroy=false;
	anim=0;
}

CutScene::~CutScene()
{
}

void CutScene::assign(Vector3 pos,Real length,String name,bool freezebefore,bool unfreezeafter,bool splineanims,bool hidehud)
{
	mName = name;
	this->length=length;
	mStartPosition =pos;
	fb=freezebefore;
	mPlayer = global::getSingleton().getPlayer();
	
	unfreeze=unfreezeafter;
	mCamera = mPlayer->get_camera();
	//mCamera->setPosition(pos);
	mSceneMgr = global::getSingleton().getSceneManager();
	camera_scenenode = mSceneMgr->getRootSceneNode()->createChildSceneNode(pos);
	anim = mSceneMgr->createAnimation("CameraTrack"+getname(), length);
        // Spline it for nice curves
	if (splineanims)
		anim->setInterpolationMode(Animation::IM_SPLINE);
        // Create a track to animate the camera's node
	track = anim->createNodeTrack(0,camera_scenenode);
	key = track->createNodeKeyFrame(0); // startposition
	global::getSingleton().getRoot()->addFrameListener(this);
	hhud=hidehud;
	if (hidehud)
		HUD::getSingleton().Hide();
}

String CutScene::getname()
{
return mName;
}

void CutScene::setWait(Real wait)
{
	wait_before_start = wait;
}

void CutScene::addFrame(Vector3 pos,Real seconds,Quaternion quat)
{
	key = track->createNodeKeyFrame(seconds);
	if (pos!=Ogre::Vector3::ZERO)
		key->setTranslate(pos);
	if (quat!=Quaternion::IDENTITY)
		key->setRotation(quat);
}

void CutScene::start()
{
	//LogManager::getSingleton().logMessage("Okay");
	Quaternion rot = mCamera->getDerivedOrientation();
	Vector3 pos = mCamera->getDerivedPosition();
		mPlayer->detatch_camera(fb);
		//LogManager::getSingleton().logMessage("Okay");
		camera_scenenode->attachObject(mCamera);


		//WARNING!!!

		//mCamera->setPosition(pos);
		//mCamera->setOrientation(rot);


		//LogManager::getSingleton().logMessage("Okay");
		if (!mAnimState)
		{
		//	LogManager::getSingleton().logMessage("Okay");
		mAnimState = mSceneMgr->createAnimationState("CameraTrack"+getname());
        mAnimState->setEnabled(true);
		mAnimState->setLoop(false);
		//LogManager::getSingleton().logMessage("Okay");
		}
		else
		{
			mAnimState->setTimePosition(0.0f);
			mAnimState->setEnabled(true);
			mAnimState->setLoop(false);
		}
		//LogManager::getSingleton().logMessage("Okay");
		first=true;
		started =true;
		//LogManager::getSingleton().logMessage("Okay");
		//disposed=false;
		if (musicSync)
		{
			MusicPlayer::getSingleton().cleanup();
			MusicPlayer::getSingleton().playMusicOverride(musicName,false);
		}
		//LogManager::getSingleton().logMessage("Okay");
}

void CutScene::stop()
{
	if (started)
		mAnimState->setEnabled(false);
}

bool CutScene::frameStarted(const Ogre::FrameEvent &ev)
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
//LogManager::getSingleton().logMessage("l1");
	Real deltaTime;
	if (!musicSync)
	{
		deltaTime = ev.timeSinceLastFrame;
	}
	else
	{
		deltaTime = MusicPlayer::getSingleton().getPosition()*musicLength-timeBefore;
		//LogManager::getSingleton().logMessage(StringConverter::toString(deltaTime));
		timeBefore=MusicPlayer::getSingleton().getPosition()*musicLength;
	}

	if (wait_before_start==0)
	{
		if (started)
		{
		if (mAnimState->getTimePosition()<mAnimState->getLength())
		{


			/*if (first)
			{*/
			vector<Real>::iterator j;
			vector<String>::iterator k;
				for (i=0;i!=scripts.size();i++)
				{
					
					//TODO NOT CORRECT FOR MUSIC SYNC
					if (!musicSync)
					{
						if (mAnimState->getTimePosition()>scripts_s[i])
						{
							LogManager::getSingleton().logMessage("Cutscene: Running lua script!");
							RunLuaScript(global::getSingleton().getLuaState(), scripts[i].c_str());
							scripts_s[i]=-1;
							//first=false;
						}
					}
					else
					{
						if (MusicPlayer::getSingleton().getPosition()*musicLength>scripts_s[i])
						{
							LogManager::getSingleton().logMessage("Cutscene: Running lua script!");
							RunLuaScript(global::getSingleton().getLuaState(), scripts[i].c_str());
							scripts_s[i]=-1;
							//first=false;
						}
					}

					
				}
				
				/*i=0;
				for (j=scripts_s.begin();j!=scripts_s.end();j++)
				{
					i++;
					if ((*j)==-1)
					{
						LogManager::getSingleton().logMessage("Erasing a lua script...");
						scripts_s.erase(j);
						k=scripts.begin();
						advance(k,i);
						scripts.erase(k);
					}
				}*/
				// Oh my fucking shit!!! I need to use my "Sgw32" deleter...
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
				//Process frames
			if (deltaTime<3.0f)
			{
				//delta error
				if (skipping)
				{
					//LogManager::getSingleton().logMessage("Skipping cutscene at:"+StringConverter::toString(deltaTime*speedcoef));
					
					if (musicSync)
						mAnimState->addTime(deltaTime);
					else
						mAnimState->addTime(deltaTime*speedcoef);
				}
				else
				{
					mAnimState->addTime(deltaTime);
				}
			}
				
			//D-trigger(not)
			if ((!skipping)&&mPlayer->getSkippingCutScene())
			{
				
				skipping=true;
				Timeshift::getSingleton().setTimeK(speedcoef);
			}
			if ((skipping)&&(!(mPlayer->getSkippingCutScene())))
			{
				Timeshift::getSingleton().setTimeK(1.0f);
				skipping=false;
			}
			//skipping=mPlayer->getSkippingCutScene();
		//}




		}
		if ((mAnimState->getTimePosition()>=mAnimState->getLength())&&mInf)
		{
			mAnimState->setTimePosition(0.1f);
		}
		if ((mAnimState->getTimePosition()>=mAnimState->getLength())&&!mInf)
		{
			if (hhud)
				HUD::getSingleton().Show();

			if (!mNoDestroy)
			{
				dispose();
			}
			else
			{
				dispose2();
			}
		}
		
		}
	}
	else
	{
		if (!(tim2>wait_before_start))
			tim2=tim2+deltaTime;
		if (tim2>wait_before_start)
		{
			start();
			wait_before_start=0;
		}
	}
		
	return true;
}


void CutScene::dispose2()
{
	Timeshift::getSingleton().setTimeK(1.0f);
	
		LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	started =false;
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	camera_scenenode->detachAllObjects();
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	//mSceneMgr->destroyAnimation("CameraTrack"+getname());
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	mSceneMgr->destroyAnimationState("CameraTrack"+getname());
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	//if (time<=length)
	//	mSceneMgr->destroySceneNode(camera_scenenode);
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	mPlayer->attach_camera(unfreeze);
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
	mAnimState=0;
	//LogManager::getSingleton().logMessage("Disposing CutScene 2. It has been used ingame");
}


void CutScene::dispose()
{
	Timeshift::getSingleton().setTimeK(1.0f);
	LogManager::getSingleton().logMessage("1t");
	if (!disposed)
	{
		LogManager::getSingleton().logMessage("1t");
	if (first)
	{
		LogManager::getSingleton().logMessage("Disposing CutScene. It has been used ingame");
	started =false;
	camera_scenenode->detachAllObjects();
	mSceneMgr->destroyAnimation("CameraTrack"+getname());
	mSceneMgr->destroyAnimationState("CameraTrack"+getname());
	if (time<=length)
		mSceneMgr->destroySceneNode(camera_scenenode);
	mPlayer->attach_camera(unfreeze);
	global::getSingleton().getRoot()->removeFrameListener(this);
	disposed=true;
	}
	else
	{
		LogManager::getSingleton().logMessage("Disposing CutScene. It wasn't used ingame");
		camera_scenenode->detachAllObjects();
	mSceneMgr->destroyAnimation("CameraTrack"+getname());
	mSceneMgr->destroyAnimationState("CameraTrack"+getname());
	mSceneMgr->destroySceneNode(camera_scenenode);
	global::getSingleton().getRoot()->removeFrameListener(this);
	disposed=true;
	}
	}
}
