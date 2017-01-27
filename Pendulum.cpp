/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2015 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Pendulum.h"

Pendulum::Pendulum()
{
	nowfire = false;
	opened=false;
	closed=true;
	funcrot=false;
	mTime=0;
	rotDoor=false;
	sop=false;
	mp1=0;
	my1=0;
	mr1=0;
	mp=0;
	my=0;
	mr=0;
	scl=false;
mInteract=false;
	mAngle=0;
	mCurAngle=0;
	lDoorFopened=lDoorFclosed=lDoorS="";
}

Pendulum::~Pendulum()
{
}

void Pendulum::setup(Ogre::SceneNode* door_node,bool box)
{
	mNode = door_node;
	aab = AxisAlignedBox(mNode->getPosition() - (mNode->getScale()),mNode->getPosition() + (mNode->getScale()));
	OgreNewt::Collision* col;
	if (!box)
	{
	col = new OgreNewt::CollisionPrimitives::TreeCollision(global::getSingleton().getWorld(), mNode,true);
	}
	else
	{
	AxisAlignedBox aab=((Entity*)mNode->getAttachedObject(0))->getBoundingBox();
	Ogre::Vector3 scale=mNode->getScale();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*mNode->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	col = new OgreNewt::CollisionPrimitives::Box(global::getSingleton().getWorld(),size,Quaternion::IDENTITY,center);
	}

	
	door_bod = new OgreNewt::Body( global::getSingleton().getWorld(), col );
	door_bod->attachToNode( mNode );
	door_bod->setPositionOrientation( mNode->getPosition(), mNode->getOrientation() );

	sPosition = mNode->getPosition();
	mStartAngle = mNode->getOrientation();
	sMgr=global::getSingleton().getSoundManager();
	//door_bod->getm
	delete col;
}

void Pendulum::setup(Ogre::SceneNode* door_node)
{
	mNode = door_node;
	aab = AxisAlignedBox(mNode->getPosition() - (mNode->getScale()),mNode->getPosition() + (mNode->getScale()));
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::TreeCollision(global::getSingleton().getWorld(), mNode,true);
	door_bod = new OgreNewt::Body( global::getSingleton().getWorld(), col );
	door_bod->attachToNode( mNode );
	door_bod->setPositionOrientation( mNode->getPosition(), mNode->getOrientation() );
	sPosition = mNode->getPosition();
	mStartAngle = mNode->getOrientation();
	sMgr=global::getSingleton().getSoundManager();
	//door_bod->getm
	delete col;
	basePos=get_position();
}

void Pendulum::Fire(String event)
{
	if (funcrot)
	{
		if (event=="toggle")
		{
			LogManager::getSingleton().logMessage("starting pendulum...");
			nowfire = !nowfire;
		}
		if (event=="open")
		{
			nowfire = true;
		}
		if (event=="close")
		{
			nowfire = false;
		}
	}
}

Vector3 Pendulum::get_position()
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
return oldpos;
}

void Pendulum::set_orientation(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat);
}

void Pendulum::rotateBody(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat+quat2);
}

void Pendulum::setname(String name)
{
	mName = name;
}

String Pendulum::getname()
{
	return mName;
}


void Pendulum::unload()
{
	//deleteSound();
	//global::getSingleton().getRoot()->removeFrameListener(this);
	//DefaultAEnt::root->removeFrameListener(this);
	nowfire=false;
	delete door_bod;
	door_bod=0;
	/*
	Ogre::MovableObject* aEnt = mNode->getAttachedObject(0);
	global::getSingleton().getPlayer()->remove_additional_force();
	if (global::getSingleton().getSceneManager()->hasEntity(aEnt->getName()))
	{
	mNode->detachAllObjects();
	global::getSingleton().getSceneManager()->destroySceneNode(mNode);
	global::getSingleton().getSceneManager()->destroyEntity(aEnt->getName());
	}*/
	

	Entity* ent = (Entity*)mNode->getAttachedObject(0);
	mNode->detachAllObjects();
	LogManager::getSingleton().logMessage("Pendulum: remove children");
	for (int i=0;i!=mNode->numChildren();i++)
	{
		if ((SceneNode*)mNode->getChild(i))
			{
				if (((SceneNode*)mNode->getChild(i))->getAttachedObject(0))
				{
					if (global::getSingleton().getSceneManager()->hasEntity((((SceneNode*)mNode->getChild(i))->getAttachedObject(0)->getName())))
					{
						LogManager::getSingleton().logMessage("Train: destroying entity child");
					global::getSingleton().getSceneManager()->destroyEntity((((SceneNode*)mNode->getChild(i))->getAttachedObject(0)->getName()));
					}
					else
					{
						LogManager::getSingleton().logMessage("Train: destroying particle system");
						
							if (global::getSingleton().getSceneManager()->hasParticleSystem((((SceneNode*)mNode->getChild(i))->getAttachedObject(0)->getName())))
							global::getSingleton().getSceneManager()->destroyParticleSystem((((SceneNode*)mNode->getChild(i))->getAttachedObject(0)->getName()));
							
						
					}
				}
			}
		//global::getSingleton().getSceneManager()->destroyMovableObject((((SceneNode*)nod->getChild(i))->getAttachedObject(0)->getName()));
	}
	mNode->removeAndDestroyAllChildren();

	if (ent)
	global::getSingleton().getSceneManager()->destroyEntity(ent);
	   global::getSingleton().getSceneManager()->destroySceneNode(mNode);
}

void Pendulum::set_position(Vector3 pos)
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
door_bod->setPositionOrientation(pos,quat);
}

bool Pendulum::frameStarted(const Ogre::FrameEvent &evt)
{
	if (!door_bod)
		return true;
	if (evt.timeSinceLastFrame>3.0f)
		return true;
	velocity = mspeed;

	if (mInteract)
	{
		if (door_bod->getUsing())
			Fire("toggle");
	}





	if (nowfire)
	{
				if (funcrot)
				{
							if (rotDoor)
							{
								/*Real mpt1= -rotSpeed*evt.timeSinceLastFrame*5*Ogre::Math::Sign(mp1)*TIME_SHIFT;
								Real myt1= -rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(my1);
								Real mrt1= -rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(mr1);
								mNode->pitch(Degree(mpt1));
								mNode->yaw(Degree(myt1));
								mNode->roll(Degree(mrt1));*/
								//LogManager::getSingleton().logMessage("step");
								mp1+=evt.timeSinceLastFrame*rotSpeed;
								my1+=evt.timeSinceLastFrame*rotSpeed;
								mr1+=evt.timeSinceLastFrame*rotSpeed;
								//LogManager::getSingleton().logMessage("step");
								if (mp1>360.0f)
									mp1=0;
								if (my1>360.0f)
									my1=0;
								if (mr1>360.0f)
									mr1=0;
								//LogManager::getSingleton().logMessage("step");
								Quaternion pitch(Degree(mp*Math::Sin(mp1)),Vector3::UNIT_X);
								Quaternion yaw(Degree(my*Math::Sin(my1)),Vector3::UNIT_Y);
								Quaternion roll(Degree(mr*Math::Sin(mr1)),Vector3::UNIT_Z);
								Quaternion rot = mQuatAngle*pitch*yaw*roll;
								//LogManager::getSingleton().logMessage("step");
								set_orientation(rot);
								set_position(basePos+Vector3(positionPend.x*Math::Sin(mp1),positionPend.y*Math::Sin(mp1),positionPend.z*Math::Sin(mp1)));
							}
							else
							{ 
//								set_position(get_position()-vDirection*evt.timeSinceLastFrame*TIME_SHIFT/0.2f);
							}
				}
	}
	return true;
}
