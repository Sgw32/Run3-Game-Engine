#pragma once
#include "Ogre.h"
#include "OgreNewt.h"
#include "ExplosionManager.h"
#include "GibManager.h"
#include "Run3SoundRuntime.h"
#include "Timeshift.h"
using namespace Ogre;
using namespace OgreNewt;
using namespace std;


class TrainMatOther : public OgreNewt::ContactCallback
{
public:
	TrainMatOther(void){};
	~TrainMatOther(void){};
	void train_derail( OgreNewt::Body* me )
	{
		Ogre::Real mass; 
	Ogre::Vector3 inertia; 
	   
	me->getMassMatrix(mass, inertia); 
	Ogre::Vector3 force(0,-500*TIME_SHIFT*TIME_SHIFT,0); 
	force *= mass; 
		me->addForce(force);
	}
	int userProcess()
	{
	
	return true;
	}
	World* mWorld;
};



class TrainMat : public OgreNewt::ContactCallback
{
public:
	TrainMat(void){};
	~TrainMat(void){};
	void train_derail( OgreNewt::Body* me )
{
Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,-500*TIME_SHIFT*TIME_SHIFT,0); 
   force *= mass; 
	me->addForce(force);
}
	int userProcess()
	{
		Vector3 p,n;
		getContactPositionAndNormal(p,n);
		if ((m_body0->getType()==6)&&(m_body1->getType()==6))
		{
			ExplosionManager::getSingleton().spawnExplosionNosound(p,Vector3(3,3,3));
			Run3SoundRuntime::getSingleton().emitSound("run3/sounds/explode_3.wav",4,false,p,200,50,800);
			GibManager::getSingleton().spawnGibs("metal_chunk01.mesh",p,Vector3(3,3,3),10.0f,30,500);
			/*m_body0->setCustomForceAndTorqueCallback<TrainMat>( &TrainMat::train_derail ,this);
			m_body1->setCustomForceAndTorqueCallback<TrainMat>( &TrainMat::train_derail ,this);*/
			m_body0->setType(4);
			m_body1->setType(4);
			m_body0->setMaterialGroupID(mWorld->getDefaultMaterialID());
			m_body1->setMaterialGroupID(mWorld->getDefaultMaterialID());
			//m_body0->setAutoFreeze(1);
			//m_body1->setAutoFreeze(1);
			/*Train* t1 = (Train*)m_body0->getUserData();
			t1->derail();
			t1 = (Train*)m_body1->getUserData();
			t1->derail();*/

		}
		Vector3 force = getContactForce();
		Real y = force.y;
		if (y>100000*TIME_SHIFT*TIME_SHIFT)
		{
		Run3SoundRuntime::getSingleton().emitSound("run3/sounds/trbr.wav",5.0f,false,p,200,50,800);
		
	}

	/*if ((m_body0->getType()==1))
	{
		global::getSingleton().getPlayer()->tr_v=m_body0->getVelocity();
	}
	if ((m_body0->getType()==1))
	{
		global::getSingleton().getPlayer()->tr_v=m_body0->getVelocity();
	}
	if ((!(m_body0->getType()==1))&&(!(m_body0->getType()==1)))
	{
		global::getSingleton().getPlayer()->tr_v=Vector3::ZERO;
	}*/
		if (m_body0->getType()==4) 
			return false;
	if (m_body1->getType()==4) 
			return false;
		
	
	return true;
	}
World* mWorld;
};

#define MAX_PRECISION_NEAR 50

class Train
{
public:
	Train();
	~Train();
	void init(String name,SceneNode* sn,Real speed);
	void init_nosetup(String name,SceneNode* sn,Real speed);
	void addUpVectors()
	{
		World* w = global::getSingleton().getWorld();
		OgreNewt::BasicJoints::UpVector* uv1 = new OgreNewt::BasicJoints::UpVector(w,bod,Vector3::UNIT_X);
		OgreNewt::BasicJoints::UpVector* uv3 = new OgreNewt::BasicJoints::UpVector(w,bod,Vector3::UNIT_Z);
	}
	void setYawAngle(Real angle)
	{
		mYawAngle=angle;
	}
	void addDetail(Entity* ent, Vector3 pos,Vector3 scale, Quaternion rot);
	void addNoCollide(Entity* ent, Vector3 pos,Vector3 scale, Quaternion rot);
	void mainObjectSetVisible(bool visible)
	{
		//TODO:
	}
	void finish_build();

	void train_movement( OgreNewt::Body* body );
	void train_derail( OgreNewt::Body* body );
	void addKeyPoint(Vector3 pos,String script,bool startup,bool pause);
	void start();
	void stop();
	void pause();
	void resume();
	void reverse(){rev=!rev;}
	void cleanup();
	void addDetail(SceneNode* det)
	{
//		det->setParent(
	}
	void setAcceleration(Real acc);
	Quaternion getDirectionQuat(const Ogre::Vector3& vec, const Ogre::Vector3& localDirectionVector)
	{
		 // Do nothing if given a zero vector
		if (vec == Vector3::ZERO) return Quaternion::IDENTITY;
		Quaternion targetOrientation;
        // The direction we want the local direction point to
        Vector3 targetDir = vec.normalisedCopy();


        // Calculate target orientation relative to world space

            const Quaternion& currentOrient = getrot(bod);

            // Get current local direction relative to world space
            Vector3 currentDir = currentOrient * localDirectionVector;

            if ((currentDir+targetDir).squaredLength() < 0.00005f)
            {
                // Oops, a 180 degree turn (infinite possible rotation axes)
                // Default to yaw i.e. use current UP
                targetOrientation =
                    Quaternion(-currentOrient.y, -currentOrient.z, currentOrient.w, currentOrient.x);
            }
            else
            {
                // Derive shortest arc to new direction
                Quaternion rotQuat = currentDir.getRotationTo(targetDir);
                targetOrientation = rotQuat * currentOrient;
            }
			return targetOrientation;
	}
	void setFixOrient(bool x);
	void derail()
	{
		bod->setCustomForceAndTorqueCallback<Train>( &Train::train_derail ,this);
	}
	bool checkNear(Ogre::Vector3 p1,Ogre::Vector3 p2)
	{
		if (p1.distance(p2)<=MAX_PRECISION_NEAR)
			return true;
		else
			return false;
	}
	void setMaterialGroupID(const OgreNewt::MaterialID* enemyMat)
	{
		if (bod)
		{
			bod->setMaterialGroupID(enemyMat);
		}
	}

	void addParticleSystem(String name, Vector3 pos, Vector3 scale);

	Ogre::Vector3 getpos(OgreNewt::Body* bod)
	{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return pos;
	}
	Ogre::Quaternion getrot(OgreNewt::Body* bod)
	{
		Vector3 pos;
		Quaternion rot;
		bod->getPositionOrientation(pos,rot);
		return rot;
	}
	void setInfinite(bool inf)
	{
		infinite=inf;
	}

	void enablePositionSaveMode()
	{
	positionSaveMode=true;
	saveposition=getpos(bod);
	}

	void disablePositionSaveMode()
	{
	positionSaveMode=false;
	}

	String getName(){return mName;}
	String stopSound;
	String movingSound;
	String startSound;
	Real mSpeed;
	Real soundDuration;
private:
	bool moving;
	bool rev;
	bool setor;
	bool infinite;
	bool hasParticleSystem;
	bool positionSaveMode;
	Vector3 saveposition;
	unsigned int curKPoint;
	OgreNewt::BasicJoints::UpVector* dirV;
	String mName;
	Body* bod;
	SceneNode* nod;
	SceneNode* particleNode;
	//Entity* main;
	World* w;
	Real gravity;
	Real mAcceleration;
	Real blastUpVector;
	Real blastOmega;
	Real mYawAngle;
	vector<Vector3> path;
	SoundManager* sMgr;
	unsigned int sound;
	vector<String> scripts;
	std::vector<OgreNewt::Collision*> vc;
	OgreNewt::Collision* col1;
};