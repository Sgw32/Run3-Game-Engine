#include "Train.h"
#include "global.h"

Train::Train()
{
	moving=false;
	setor=true;
	curKPoint=0;
	sound=777;
	blastUpVector=0;
	blastOmega=0;
	soundDuration=9.0f;
	hasParticleSystem=false;
	mAcceleration=0;
	positionSaveMode=false;
	infinite=false;
	mYawAngle=180;
	particleNode=0;
}

Train::~Train()
{

}

void Train::addParticleSystem(String name, Vector3 pos, Vector3 scale)
{
	LogManager::getSingleton().logMessage("Train: creating attached particle");
	ParticleSystem *pParticles = global::getSingleton().getSceneManager()->createParticleSystem(mName+"particle", name);
	particleNode = nod->createChildSceneNode(pos);
	particleNode->setScale(scale);
	particleNode->attachObject(pParticles);
	particleNode->setVisible(false);
}

void Train::setAcceleration(Real acc)
{
	mAcceleration=acc;
}

void Train::init_nosetup(String name,SceneNode* sn,Real speed)
{
	gravity=GRAVITY;
	rev=true;
	mName=name;
	nod=sn;
	mSpeed=speed;
	LogManager::getSingleton().logMessage("Starting nosetup");
	AxisAlignedBox aab=sn->getAttachedObject(0)->getBoundingBox();
	w = global::getSingleton().getWorld();

	LogManager::getSingleton().logMessage("Convex hull process");
	col1 = new OgreNewt::CollisionPrimitives::ConvexHull(w,nod);

	sMgr=global::getSingleton().getSoundManager();

	stopSound="run3/sounds/tram_squeak.wav";
	movingSound="run3/sounds/train2.wav";
	startSound="run3/sounds/train_horn2.wav";
	LogManager::getSingleton().logMessage("Adding detail 0");
	vc.push_back(col1);
//	delete col;
}

void Train::addDetail(Entity* ent, Vector3 pos,Vector3 scale, Quaternion rot)
{
	LogManager::getSingleton().logMessage("3");
	SceneNode* d = nod->createChildSceneNode(pos,rot);
	LogManager::getSingleton().logMessage("4");
	d->attachObject(ent);
	LogManager::getSingleton().logMessage("5");
	d->setScale(scale*nod->getScale());
	LogManager::getSingleton().logMessage("2");
	OgreNewt::Collision* col2 = new OgreNewt::CollisionPrimitives::ConvexHull(w,d,rot,pos);
	//OgreNewt::co
	//OgreNewt::CollisionPrimitives::ConvexHull(
	//vc.clear();
	
vc.push_back(col2);
		
		d->setScale(scale);
		//col1=col3;
}


void Train::addNoCollide(Entity* ent, Vector3 pos,Vector3 scale, Quaternion rot)
{
	LogManager::getSingleton().logMessage("3");
	SceneNode* d = nod->createChildSceneNode(pos,rot);
	LogManager::getSingleton().logMessage("4");
	d->attachObject(ent);
	LogManager::getSingleton().logMessage("5");
	d->setScale(scale*nod->getScale());
	LogManager::getSingleton().logMessage("2");
	//OgreNewt::Collision* col2 = new OgreNewt::CollisionPrimitives::ConvexHull(w,d,rot,pos);
	//OgreNewt::co
	//OgreNewt::CollisionPrimitives::ConvexHull(
	//vc.clear();
	
//vc.push_back(col2);
		
		d->setScale(scale);
		//col1=col3;
}

void Train::finish_build()
{
	AxisAlignedBox aab=nod->getAttachedObject(0)->getBoundingBox();
	OgreNewt::Collision* col3 = new OgreNewt::CollisionPrimitives::CompoundCollision(w,vc);
	bod = new OgreNewt::Body( w, col3 );
	bod->attachToNode( nod );
	bod->setName(nod->getName());
	
	bod->setPositionOrientation( nod->getPosition(),nod->getOrientation() );//,Quaternion::IDENTITY);// nod->getOrientation() );
	Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( 2000, aab.getSize() );
	bod->setMassMatrix( 2000, inertia );
	OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(w,bod,Vector3::UNIT_Y);

	bod->setType(PHYSOBJECT_TRAIN);
	bod->setCustomForceAndTorqueCallback<Train>( &Train::train_movement ,this);
	bod->setAutoFreeze(0);
	delete col1;
	//bod->setPositionOrientation(getpos(bod),Quaternion(Degree(90),Vector3::UNIT_Y));
}

void Train::init(String name,SceneNode* sn,Real speed)
{
	gravity=GRAVITY;
	rev=true;
	mName=name;
	nod=sn;
	mSpeed=speed;
	AxisAlignedBox aab=sn->getAttachedObject(0)->getBoundingBox();
	w = global::getSingleton().getWorld();
	//OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(w,aab.getSize()*nod->getScale());
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::ConvexHull(w,nod);
	//OgreNewt::CollisionPrimitives::CompoundCollision(
	bod = new OgreNewt::Body( w, col );
	bod->attachToNode( nod );
	bod->setName(nod->getName());
	//bod->reactByRot=false;
	bod->setPositionOrientation( nod->getPosition(),nod->getOrientation());// nod->getOrientation() );
	Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( 2000, aab.getSize()*nod->getScale() );
	bod->setMassMatrix( 2000, inertia );
	OgreNewt::BasicJoints::UpVector* uv2 = new OgreNewt::BasicJoints::UpVector(w,bod,Vector3::UNIT_Y);
	sMgr=global::getSingleton().getSoundManager();
	/*OgreNewt::BasicJoints::UpVector* uv1 = new OgreNewt::BasicJoints::UpVector(w,bod,Vector3::UNIT_X);
	OgreNewt::BasicJoints::UpVector* uv3 = new OgreNewt::BasicJoints::UpVector(w,bod,Vector3::UNIT_Z);
/*TrainMat* mPhysCallback = new TrainMat;
const OgreNewt::MaterialID* mMatDefault = global::getSingleton().getPlayer()->getPlayerMat();
  const OgreNewt::MaterialID* trainMat= new OgreNewt::MaterialID( w );
  OgreNewt::MaterialPair* mMatPair = new OgreNewt::MaterialPair( w, mMatDefault, trainMat );
  mMatPair->setContactCallback( mPhysCallback );
    mMatPair = new OgreNewt::MaterialPair( w, w->getDefaultMaterialID(), trainMat );
  mMatPair->setContactCallback( mPhysCallback );*/
  //mMatPair->setDefaultFriction(0,0);
	stopSound="run3/sounds/tram_squeak.wav";
	movingSound="run3/sounds/train2.wav";
	startSound="run3/sounds/train_horn2.wav";
	//bod->setMaterialGroupID( trainMat );
	bod->setType(PHYSOBJECT_TRAIN);
	//bod->setUserData(this);
	bod->setCustomForceAndTorqueCallback<Train>( &Train::train_movement ,this);
	bod->setAutoFreeze(0);
	delete col;
	//bod->setPositionOrientation(getpos(bod),Quaternion(Degree(90),Vector3::UNIT_Y));
}

void Train::setFixOrient(bool x)
{
	setor=x;
	if (setor)
	{
		Quaternion myRot = getrot(bod);
		Vector3 myDir = Quaternion(Radian(Degree(180)),Vector3::UNIT_Y)*myRot*Vector3::NEGATIVE_UNIT_Z;
		dirV = new OgreNewt::BasicJoints::UpVector(w,bod,myDir);
	}
}

void Train::train_movement( OgreNewt::Body* body )
{
	if (positionSaveMode)
	{
		body->setPositionOrientation(saveposition,getrot(body));
	}
	Real stepevt = global::getSingleton().getWorld()->getTimeStep();
	mSpeed+=mAcceleration*stepevt;

	if (body->getType()==4)
		pause();
if (moving)
{
	Vector3 myPos = getpos(body);
	Vector3 dest = path[curKPoint];
	
	//nod->lookAt(dest,Node::TS_LOCAL,Vector3::UNIT_X);
	if (checkNear(myPos,dest))
	{
		if (scripts[curKPoint]!="none")
		{
			RunLuaScript(global::getSingleton().getLuaState(),scripts[curKPoint].c_str());
		}

		if (rev) 
		{
			if (curKPoint!=path.size()-1)
				curKPoint++;
			else
			{
				moving=false;
				stop();
				LogManager::getSingleton().logMessage("Train finished path!1");
				return;
			}
		}
		else
		{
			if (curKPoint!=0)
				curKPoint--;
			else
			{
				moving=false;
				stop();
				LogManager::getSingleton().logMessage("Train finished path!2");
				return;
			}
		}
	}
	dest = path[curKPoint];

	
	
	//LogManager::getSingleton().logMessage(StringConverter::toString(dest));
	Vector3 vel = (dest-myPos).normalisedCopy()*mSpeed*TIME_SHIFT;
	body->setVelocity(vel);
	//body->setPositionOrientation(getpos(body),Quaternion(Degree(90),Vector3::UNIT_Y));
	if (setor)
	{
		Vector3 next = Vector3::ZERO;
		Vector3 deltaDir = Vector3::ZERO;
		Vector3 deltaOr;
		Quaternion deltaRot;
		Quaternion myRot = getrot(body);

		if ((curKPoint>1)&&(curKPoint<path.size()-1))
		{
			if (rev) 
			{
				next= dest;
				dest = path[curKPoint-1];
			}
			else
			{
				next= dest;
				dest = path[curKPoint+1];
			}
		}
		if (next!=Vector3::ZERO)
		{
			//Direction of a train
			/*LogManager::getSingleton().logMessage("Passed node is "+StringConverter::toString(dest));
			LogManager::getSingleton().logMessage("Next node is "+StringConverter::toString(next));
			LogManager::getSingleton().logMessage("My orientation is "+StringConverter::toString(myRot));*/

			Vector3 myDir = Quaternion(Radian(Degree(mYawAngle)),Vector3::UNIT_Y)*myRot*Vector3::NEGATIVE_UNIT_Z;
			Vector3 pathDir = (next-dest).normalisedCopy();
			//(next-dest).normalisedCopy() is the estimated direction
			deltaDir=(pathDir-myDir.normalisedCopy());
			if (deltaDir.length()>0.01f)
			{
				deltaDir=deltaDir;
			//A little move to compensate it
			deltaOr=deltaDir*stepevt*10.0f;
			
			deltaRot = myDir.getRotationTo(myDir+deltaOr);
			//deltaRot.Inverse();
			//myRot=myRot*deltaRot;
			//dirV->setPin(pathDir);
			dirV->setPin(myDir+deltaOr);
			}
			//body->setPositionOrientation(getpos(body),myRot);

			/*LogManager::getSingleton().logMessage("My direction is "+StringConverter::toString(myDir));
			LogManager::getSingleton().logMessage("Path direction is "+StringConverter::toString(pathDir));
			LogManager::getSingleton().logMessage("All delta direction "+StringConverter::toString(deltaDir));
			LogManager::getSingleton().logMessage("Time-byp delta "+StringConverter::toString(deltaOr));
			LogManager::getSingleton().logMessage("Delta quat "+StringConverter::toString(deltaRot));
			LogManager::getSingleton().logMessage("Final orientation is "+StringConverter::toString(myRot));*/
		}
		//Sophisticated algorithm to estimate delta-orientation.
		//body->setPositionOrientation(getpos(body),getDirectionQuat(dest,Vector3::NEGATIVE_UNIT_Z)*Quaternion(Degree(90),Vector3::UNIT_Y));
	}
	else
	{	
		body->setPositionOrientation(getpos(body),nod->getOrientation());
	}
	sMgr->setSound(sound,getpos(body),Vector3(1,1,1),Vector3(1,1,1),1000,false,false,200,50,800);
}
else
{
	if (body->getType()==6)
	{
	body->setVelocity(Vector3::ZERO);
//	body->setPositionOrientation(getpos(body),Quaternion(Degree(90),Vector3::UNIT_Y));
	}
	else
	{
		Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   body->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,gravity,0);
	if (blastUpVector!=0)
	{
		blastUpVector+=gravity/2;
		if (blastUpVector<0)
			blastUpVector=0;
	}
   force *= mass; 
	body->addForce(force*TIME_SHIFT*TIME_SHIFT);
	body->setOmega(Vector3(0,0,blastOmega*4*TIME_SHIFT));
	if (blastOmega!=0)
	{
		blastOmega+=gravity/10000;
		if (blastOmega<0)
			blastOmega=0;
	}
	}
}
}

void Train::train_derail( OgreNewt::Body* me )
{
Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,gravity,0); 
   force *= mass; 
	me->addForce(force);
}

void Train::addKeyPoint(Vector3 pos,String script,bool startup,bool pause)
{
	curKPoint=1;
path.push_back(pos);
scripts.push_back(script);
LogManager::getSingleton().logMessage(StringConverter::toString(pos));
if (startup)
{
	curKPoint=path.size();
//	bod->setPositionOrientation( pos, nod->getOrientation() );
}
}

void Train::start()
{
	LogManager::getSingleton().logMessage("Train started!");
	if (movingSound!="none")
	{
	if (sound==777)
	{
	sMgr->loadAudio(movingSound,&sound,true);
	sMgr->playAudio(sound,true);
	}
	}
	if (startSound!="none")
	Run3SoundRuntime::getSingleton().emitSound(startSound,soundDuration,false,getpos(bod),200,50,800);
	moving=true;
	if (particleNode)
		particleNode->setVisible(true);
}

void Train::resume()
{
	if (sound==777)
	{
			sMgr->loadAudio(movingSound,&sound,true);
	sMgr->playAudio(sound,true);
	}
	if (startSound!="none")
	Run3SoundRuntime::getSingleton().emitSound(startSound,soundDuration,false,getpos(bod),200,50,800);
	moving=true;
}

void Train::cleanup()
{
	LogManager::getSingleton().logMessage("Train: Disposing train");
delete bod;
	   bod = 0;
	Entity* ent = (Entity*)nod->getAttachedObject(0);
	nod->detachAllObjects();
	LogManager::getSingleton().logMessage("Train: remove children");
	for (int i=0;i!=nod->numChildren();i++)
	{
		if ((SceneNode*)nod->getChild(i))
			{
				if (((SceneNode*)nod->getChild(i))->getAttachedObject(0))
				{
		if (global::getSingleton().getSceneManager()->hasEntity((((SceneNode*)nod->getChild(i))->getAttachedObject(0)->getName())))
		{
			LogManager::getSingleton().logMessage("Train: destroying entity child");
		global::getSingleton().getSceneManager()->destroyEntity((((SceneNode*)nod->getChild(i))->getAttachedObject(0)->getName()));
		}
		else
		{
			LogManager::getSingleton().logMessage("Train: destroying particle system");
			
				if (global::getSingleton().getSceneManager()->hasParticleSystem((((SceneNode*)nod->getChild(i))->getAttachedObject(0)->getName())))
				global::getSingleton().getSceneManager()->destroyParticleSystem((((SceneNode*)nod->getChild(i))->getAttachedObject(0)->getName()));
				
			
		}
				}
			}
		//global::getSingleton().getSceneManager()->destroyMovableObject((((SceneNode*)nod->getChild(i))->getAttachedObject(0)->getName()));
	}
	nod->removeAndDestroyAllChildren();
	if (movingSound!="none")
	{
		if (moving)
		{
			sMgr->stopAudio(sound);
			sMgr->releaseAudio(sound);
			sound=777;
		}
	}
	if (ent)
	global::getSingleton().getSceneManager()->destroyEntity(ent);
	   global::getSingleton().getSceneManager()->destroySceneNode(nod);
}

void Train::stop()
{
		//if (moving)
	//{
	if (infinite)
	{
		curKPoint=0;
		moving=true;
		bod->setPositionOrientation(path[curKPoint],nod->getOrientation());
		return;
	}

	if (stopSound!="none")
	Run3SoundRuntime::getSingleton().emitSound(stopSound,4,false,getpos(bod),200,50,800);
curKPoint=1;
	if (movingSound!="none")
	{
		sMgr->stopAudio(sound);
		sMgr->releaseAudio(sound);
		sound=777;
	}
	moving=false;
	if (particleNode)
		particleNode->setVisible(false);
//	}
}

void Train::pause()
{
	if (moving)
	{
			if (bod->getType()==4)
	{
		//body derailed...
		blastUpVector=1000;
blastOmega=3;
Entity* e = (Entity*)nod->getAttachedObject(0);
e->setMaterialName("ExplodedTrash");
	}
	if (stopSound!="none")
	Run3SoundRuntime::getSingleton().emitSound(stopSound,4,false,getpos(bod),200,50,800);
	if (movingSound!="none")
	{
		sMgr->stopAudio(sound);
		sMgr->releaseAudio(sound);
		sound=777;
		moving=false;
	}
	}
}

