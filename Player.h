/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////I give permission to use this class, but    //////////
///////////////Copyright 2010 (c)						    //////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>
#include <OgreNewt.h>
#include "OgreConsole.h"
#include <OIS/OIS.h>
#include "SoundManager.h"
#include "CWeapon.h"
#include "HUD.h"
#include "ogreconsole.h"
#include "PlayerContactCallback.h"
#include "PlayerProps.h"
#include "Inventory.h"
#include <stdlib.h>
#include <time.h>
#include "ImprovedConfigFile.h"

#define NEWTBODYTYPE_PLAYER 1
#define KEY_DOWNR(x) global::getSingleton().getOISKeyboard()->isKeyDown(x)
//#define NEWTBODYTYPE_PLAYER 1

class Player
{
public:
	Player();
	~Player();
	void init(SceneManager *SceneMgr,OgreNewt::World* mWorld,Ogre::Camera* Camera,SoundManager* soundMgr,Ogre::Root* _root);
	void camera_force_callback( OgreNewt::Body* body );
	void MouseMove(const OIS::MouseEvent &arg,Ogre::Real time);
	void MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	void MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    void FCPress(const OIS::KeyEvent &arg);
    void FCRelease(const OIS::KeyEvent &arg);
	void FCUpdate(const FrameEvent &evt);
	void Duck();
	void multiply(Real multyplier);
	void detatch_camera(bool freeze = true);
	void attach_camera(bool unfreeze = true);
	void UnDuck();
	void shutdown();
	void set_additional_force(Vector3 force);
	void remove_additional_force();
	void set_position(Vector3 pos);
	void freeze();
	Ogre::Camera* get_camera();
	void unfreeze();
	void unfreeze2();
	void look_at(Vector3 pos);
	void cnoclip();
	void show_overlay();
	void hide_overlay();
	void resetNearClip()
	{
		mCamera->setNearClipDistance(mProps.nearClip);
	}
	void resetFarClip()
	{
		mCamera->setFarClipDistance(mProps.farClip);
	}
	void resetFov()
	{
		mCamera->setFOVy(Degree(mProps.fov));
	}
	void teleport(Vector3 dest);
	void setParentRelation(SceneNode* pPar);
	void setParentRelation(String node);
	void setFullParentRelation(bool allow)
	{
		mFullP=allow;
	}
	void stopParentRelation(){parentRelation=false;}
	void processUse();
	void changeFOV(Real fov){mCamera->setFOVy(Degree(fov));}
	Real getFOV(void){return mCamera->getFOVy().valueAngleUnits();}
	Vector3 checkNotAvailable(void);
	void destroyFlashLight()
	{
		if (flashLight&&(mSceneMgr->hasLight("flashLighter")))
		{

			mViewNode->detachObject(flashLight);
			LogManager::getSingleton().logMessage(StringConverter::toString(mSceneMgr->hasLight("flashLighter"))+"PLAYERB!!!");
			if (flashLight)
				mSceneMgr->destroyLight(flashLight);
			flashLight=0;
		}
	}
	void allowFlashLight(bool allow)
	{
		if (!allow)
		{
		destroyFlashLight();
		}
		mProps.allowflashlight=allow;
	}
	void toggleFlashLight();
	
	Vector3 get_direction();
	OgreNewt::Body* get_ray_shoot(void);
	Ogre::Quaternion Player::get_body_orientation(OgreNewt::Body* bod);
	Ogre::Vector3 Player::get_body_position(OgreNewt::Body* bod);
	Ogre::Vector3 Player::get_location();
	Ogre::Vector3 Player::get_cur_size();
	Ogre::Quaternion Player::get_orientation();
	

	SceneNode* getViewNode(void){return mViewNode;}

	Real getDistFObject(void)
	{
		Ogre::Vector3 myPos = get_location();
Ogre::Vector3 direction,end;
direction = get_direction();


end=myPos + direction * 100000.0f;

OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
if (shootRay.getHitCount()>0)
{
return shootRay.getFirstHit().mDistance;
}
else
{
	return 1.0f;
}
	}

	Vector3 getFObject(void)
	{
		Ogre::Vector3 myPos = get_location();
Ogre::Vector3 direction,end;
direction = get_direction();


end=myPos + direction * 100000.0f;

OgreNewt::BasicRaycast shootRay(mWorld,myPos,end);
	if (shootRay.getHitCount()>0)
{
return myPos +shootRay.getFirstHit().mDistance*direction * 100000.0f;
}
else
{
	return Vector3(1,1,1);
}
	}

	OgreNewt::Body* getBodyOnBottom()
	{
		Vector3 pos = get_location();
		OgreNewt::BasicRaycast wallRay1(mWorld,pos,pos-Vector3(0,cur_size.y*2+50,0));
		if ((wallRay1.getFirstHit().mBody))
			return wallRay1.getFirstHit().mBody;
		return 0;
	}

	bool checkStairEffect(Vector3 pos, Vector3 direction)
	{
		//LogManager::getSingleton().logMessage("Position:"+StringConverter::toString(pos-Vector3(0,cur_size.y,0))+"Dir:"+StringConverter::toString(pos-Vector3(0,cur_size.y,0)+direction*40));
		OgreNewt::BasicRaycast wallRay1(mWorld,pos-Vector3(0,cur_size.y,0),pos-Vector3(0,cur_size.y,0)+direction*50);
		OgreNewt::BasicRaycast wallRay2(mWorld,pos-Vector3(0,cur_size.y-mProps.stairEffect,0),pos-Vector3(0,cur_size.y-mProps.stairEffect,0)+direction*40);
		//LogManager::getSingleton().logMessage("Position:"+StringConverter::toString(pos-Vector3(0,cur_size.y-mProps.stairEffect,0))+"Dir:"+StringConverter::toString(pos-Vector3(0,cur_size.y-mProps.stairEffect,0)+direction*40));
		
		if ((wallRay1.getFirstHit().mBody)&&(!wallRay2.getFirstHit().mBody))
		{
			//LogManager::getSingleton().logMessage("Stair effect possible");
			//LogManager::getSingleton().logMessage(wallRay1.getFirstHit().mBody->getName());
			return true;
		}
		else
		{
			//LogManager::getSingleton().logMessage("Stair effect not possible");
			//LogManager::getSingleton().logMessage(StringConverter::toString(wallRay1.getHitCount()));
			//LogManager::getSingleton().logMessage(StringConverter::toString(wallRay2.getHitCount()));
			
			return false;
		}
	}

	void reloadPlayer()
	{
		mFullP=false;
		health=100;
		mCamera->setFOVy(Degree(mProps.fov));
		y_rotation_cont = 0;
		bod->setPositionOrientation(get_location(),Quaternion::IDENTITY);
		mViewNode->setOrientation(Quaternion::IDENTITY);
		if (!alive)
		{
			mViewNode->roll(Degree(60));
			//bod->setPositionOrientation(get_location(),Quaternion(Degree(-90),Vector3::UNIT_Z));
		}
		alive=true;
		if (!uv2)
		uv2 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Y);
		/*uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_X);
	uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Z);*/
		/*if (!uv1&&mProps.upvectors)
		{
		uv1 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_X);
		uv3 = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Z);
		}*/
		CWeapon::getSingleton().reload();
		m_FrontVelocity = 0;
		m_StrafeVelocity = 0;
		bod->setVelocity(Vector3::ZERO);
	}
	void playerDie();
	
	const OgreNewt::MaterialID* getPlayerMat(){return playerMat;}
	bool Player::isOnEarth();
	bool isTop();
	std::string _tostr(float a);
	bool noclip;
	bool dump;
	Vector3 tr_v;

	void playerShake(Real ma0,Real mb,Real mw)
	{
		if (st=-1.0f)
		{
			st=0;
			pa=0;
			a0=ma0*Ogre::Math::PI/180;
			b=mb;
			w=mw;
			//stdA=mViewNode->_getDerivedOrientation().getRoll().valueDegrees();
			stdA=mCamera->getOrientation().getRoll().valueDegrees();
			//mViewNode->roll(Radian(a0));
			LogManager::getSingleton().logMessage("playerShakez");
		}
	}
	void shakeAmplitude()
	{
		//LogManager::getSingleton().logMessage("shakeAmp");
		
		Real a = a0*exp(-b*st)*cos(sqrt(w*w-b*b)*st); //harmonic oscilator a0-the amplitude in the beginning, b=2*Velocity of rotating / mass, w-cycle frequency; 
		da=a-pa;
		pa=a;
		// all this ^ is from the differetial expression  d2x/dt^2+2*b*dx/dt+w^2*x=0
		
		//LogManager::getSingleton().logMessage("shakend");
		
		//Real r = a+stdA;
		//mViewNode->roll(Degree(da));
		mCamera->roll(Degree(da));
		mCamera->setPosition(Vector3(Ogre::Math::RangeRandom(-fabs(a),fabs(a)),Ogre::Math::RangeRandom(-fabs(a),fabs(a)),Ogre::Math::RangeRandom(-fabs(a),fabs(a))));
		//LogManager::getSingleton().logMessage(StringConverter::toString(a));

		if (exp(-b*st)<0.02)
		{
			mCamera->setPosition(Vector3(0,0,0));
			LogManager::getSingleton().logMessage("shakend");
			//Real stdB = mViewNode->_getDerivedOrientation().getRoll().valueDegrees();
			Real stdB = mCamera->getOrientation().getRoll().valueDegrees();
			//mViewNode->setOrientation(mViewNode->getOrientation()*Quaternion(Radian(Degree(-stdB)),Vector3::NEGATIVE_UNIT_Z));
			//LogManager::getSingleton().logMessage(StringConverter::toString(stdB));
			/*if (fabs(stdB)>=0 && stdB<90)
				mViewNode->roll(Degree(stdB+180));
			if (stdB<=180 && stdB>90)
				mViewNode->roll(Degree(stdB));*/
			st=-1.0f;
		}
	}

	void shockPlayerCutScene();

	bool getSkippingCutScene()
	{
		return skippingCutScene;
	}
	String getCurrentFootstepPrefix();
	void setCurrentFootstepPrefix(String footstep)
	{
		mFootstep=footstep;
	}
	Real getMouseSensivity()
	{
		return mRotate;
	}
	void setMouseSensivity(Real mSens)
	{
		mRotate=mSens;
		LogManager::getSingleton().logMessage("Now sens is:"+StringConverter::toString(mRotate));
		String data = StringConverter::toString(mSens);
		String nam = "mouseSensivity";
		cf.setSetting(nam,data);
		cf.save();
	}
	
	void setHealthRegeneration(Real rMul,Real maxHealth = 100.0f);
	inline Real getRegenerationMultiplier()
	{
		return regen_Mul;
	}
	inline Real getRegenerationHealth()
	{
		return mMaxHealth;
	}
	void setAllowSubtitles(bool enable);
	bool getAllowSubtitles();
	unsigned int debug;
	Real camera_rotation_x; 
    Real camera_rotation_y;
	Ogre::SceneManager* mSceneMgr;
	OgreNewt::Body* bod;
	Vector3 cur_size;
	Real razbros;
	bool god;
private:
	bool allowsubtitles;
	String mFootstep;
	bool alive;
	bool parentRelation;
	bool mFullP;
	SceneNode* mPar;
	Vector3 pRelPosition;
	Vector3 mForce;
	OgreNewt::World* mWorld;
	Ogre::Camera* mCamera;
	Real a0,b,w,st;
	Real stdA,pa,da;
	/*Ogre::String debugstr1;
	Ogre::String debugstr2;
	Ogre::String debugstr3;
	Ogre::String debugstr4;
	Ogre::String debugstr5;
	Ogre::String debugstr6;*/
    Real mRotate;          
    Real mMove; 
    Real jump_power;
	Real y_rotation_cont;
	Real x_rotation_cont;
    Real y_limit_a;
    Real y_limit_b; 
	Real cam_height;
	Real fps_speed;
	Real gravity;
	Real stop_count;
	Real stop_count_std;
	Real regen_Mul;
	Real mMaxHealth;
	bool skippingCutScene;
	bool reset_vel,unpressed;
	bool front_jump;
	bool strafe_jump;
	Ogre::Real jumpTime; //time since jump start
	Ogre::Real jumpLandedTime; //time since landing
	Ogre::Real distToFloor;
	Overlay* debugOverlay;
	OverlayElement *textbox;
	Quaternion rot3;
	ImprovedConfigFile cf;
	SoundManager* sound;
    SceneNode *mCamNode;
    SceneNode *mViewNode;
	SceneNode *mNoclipNode;
	Light* flashLight;
	Ogre::Vector3 size;
	Ogre::Vector3 MyVel;        
	Ogre::Vector3 inertia;
    Vector3 mDirection;  
	Vector3 direction;
	Vector3 stra;
	/*Ogre::Vector3 m_FrontVelocity;
	Ogre::Vector3 m_StrafeVelocity;*/
	Real m_FrontVelocity;
	Real m_StrafeVelocity;
	Vector3 size_dc;
	Vector3 poo2;
	Vector3 acel;
	Vector3 force;
	Vector3 force2;
	

	Real health;

	const OgreNewt::MaterialID* mMatDefault;
	OgreNewt::MaterialPair* mMatPair;
	const OgreNewt::MaterialID* playerMat;
	PlayerContactCallback* mPhysCallback;
	
	bool nowonearth;
	bool jump;
	bool no_movement;
	bool initial;
	bool jumpStartFinished; //true if jumping start sequence finished
	bool jumpLanded; //true as soon as I land. importand for the landing sound
	bool jumping;//if I am jumping, is set to false on landing
	bool freezed;
	bool awaitingunduck;
	std::vector<unsigned int> footsteps;
	Real fstTimer;
	int fstindex;
	Real regen_Time;
	OgreNewt::Collision* duckcol;
	OgreNewt::Collision* col;
	Ogre::Root* root;
	OgreNewt::BasicJoints::UpVector* uv1;
	OgreNewt::BasicJoints::UpVector* uv2;
	OgreNewt::BasicJoints::UpVector* uv3;
	PlayerProps mProps;
};