/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Rotating.h"

Rotating::Rotating()
{
	nowfire = false;
	opened=false;
	closed=true;
	funcrot=false;
	mTime=0;
	rotDoor=false;
	sop=false;
	scl=false;
mInteract=false;
	mAngle=0;
	mCurAngle=0;
	lDoorFopened=lDoorFclosed=lDoorS="";
}

Rotating::~Rotating()
{
}

void Rotating::setup(Ogre::SceneNode* door_node)
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
}

void Rotating::setDistanceDirection(Ogre::Real distance,Ogre::Vector3 dir,Ogre::Real speed) 
{
	dist = distance;
	direction = dir;
	mspeed = speed;
}

void Rotating::Fire(String event)
{
	if (!funcrot)
	{
		LogManager::getSingleton().logMessage("Running script:"+lDoorS);
		if (!lDoorS.empty())
		RunLuaScript(global::getSingleton().getLuaState(),lDoorS.c_str());
	if (event=="toggle")
	{
		if (opened)
			cur_event="stop";
		if (closed)
			cur_event="open";
	}
	else
	{
	cur_event = event;
	}
	fPosition = mNode->getPosition() + (direction * dist);
	time = dist / mspeed;
	nowfire = true;
	if (cur_event=="open" && sop)
	{
		deleteSound();
		sMgr->loadAudio(mOnClose,&sound,true);
		sMgr->setSound(sound,fPosition,Vector3::ZERO,Vector3::ZERO,1000,false,false,200,50,800);
		sMgr->playAudio(sound,false);
		//Run3SoundRuntime::getSingleton().emitSound(mOnOpen,time,true);
	}
	}
}

Vector3 Rotating::get_position()
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
return oldpos;
}

void Rotating::set_position(Vector3 pos)
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
door_bod->setPositionOrientation(pos,quat);
}

void Rotating::set_orientation(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat);
}

void Rotating::rotateBody(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat+quat2);
}

void Rotating::setname(String name)
{
	mName = name;
}

String Rotating::getname()
{
	return mName;
}

Ogre::Real Rotating::getMass(OgreNewt::Body* bod)
{
	Vector3 inertia;
	Real mass;
	bod->getMassMatrix(mass,inertia);
	return mass;
}

OgreNewt::Body* Rotating::getBodyOnTop()
{

	Vector3 pos = global::getSingleton().getPlayer()->get_location();
	Vector3 size = global::getSingleton().getPlayer()->get_cur_size();
 	Ogre::AxisAlignedBox plyaab = Ogre::AxisAlignedBox(pos-size-Vector3(100,100,100),pos+size+Vector3(100,100,100));
	OgreNewt::Body* l_bod = 0;
	if (aab.intersects(plyaab))
		l_bod = global::getSingleton().getPlayer()->bod;
	return l_bod;

}

void Rotating::unload()
{
	//global::getSingleton().getRoot()->removeFrameListener(this);
	//DefaultAEnt::root->removeFrameListener(this);
	delete door_bod;
	Ogre::MovableObject* aEnt = mNode->getAttachedObject(0);
	global::getSingleton().getPlayer()->remove_additional_force();
	if (global::getSingleton().getSceneManager()->hasEntity(aEnt->getName()))
	{
	mNode->detachAllObjects();
	global::getSingleton().getSceneManager()->destroySceneNode(mNode);
	global::getSingleton().getSceneManager()->destroyEntity(aEnt->getName());
	}
}

bool Rotating::frameStarted(const Ogre::FrameEvent &evt)
{
	velocity = mspeed;
	if (mInteract)
	{
		if (door_bod->getUsing())
			Fire("toggle");
	}
	if (nowfire)
	{
		if (cur_event=="open")
		{
			if (!opened)
			{
				vDirection = direction * velocity;
				Vector3 rel_pos = get_position()*direction;
				Vector3 rel_pos_f = fPosition*direction;
				Vector3 next_pos = dist*direction;
				//Vector3
				//Real dist1 = rel_pos.distance(rel_pos_f);
				if (funcrot)
				{
							if (rotDoor)
							{
								Real mpt1= rotSpeed*evt.timeSinceLastFrame*5*Ogre::Math::Sign(mp1)*TIME_SHIFT;
								Real myt1= rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(my1);
								Real mrt1= rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(mr1);
								mNode->pitch(Degree(mpt1));
								mNode->yaw(Degree(myt1));
								mNode->roll(Degree(mrt1));
							}
							else
							{ 
								set_position(get_position()+vDirection*evt.timeSinceLastFrame*TIME_SHIFT/0.2f);
							}
				}
				else
				{
				if (rotDoor)
				{


					if (!PRIMERNO_RAVNO(mp1,0,DEGREE_DISCRAPENCY))
					{
						Real mpt1= rotSpeed*evt.timeSinceLastFrame*5*Ogre::Math::Sign(mp1)*TIME_SHIFT;
						
						if (fabs(mp1-mpt1)>=DEGREE_DISCRAPENCY)
						{
						mNode->pitch(Degree(mpt1));
						mp1-=mpt1;
						}
						else
						{
							
						mNode->pitch(Degree(mp1));
						mp1=0;
						}
						
							set_orientation(mNode->getOrientation());
					}
					if (!PRIMERNO_RAVNO(my1,0,DEGREE_DISCRAPENCY))
					{
						Real myt1= rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(my1);
						
						if (fabs(my1-myt1)>=DEGREE_DISCRAPENCY)
						{
						mNode->yaw(Degree(myt1));
						my1-=myt1;
						}
						else
						{
							
						mNode->yaw(Degree(my1));
						my1=0;
						}
						
							set_orientation(mNode->getOrientation());
					}
					if (!PRIMERNO_RAVNO(mr1,0,DEGREE_DISCRAPENCY))
					{
						Real mrt1= rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(mr1);
						
						if (fabs(mr1-mrt1)>=DEGREE_DISCRAPENCY)
						{
						mNode->roll(Degree(mrt1));
						mr1-=mrt1;
						}
						else
						{
							
						mNode->roll(Degree(mr1));
						mr1=0;
						}
						
							set_orientation(mNode->getOrientation());
					}

					if (PRIMERNO_RAVNO(mp1,0,DEGREE_DISCRAPENCY)&&
						PRIMERNO_RAVNO(my1,0,DEGREE_DISCRAPENCY)&&
						PRIMERNO_RAVNO(mr1,0,DEGREE_DISCRAPENCY))
					{
						mp1=0;
						my1=0;
						mr1=0;
						nowfire =false;
					opened=true;
					closed=false;
					mTime = 0;
					}
					
				}
			}
		}
		}
		
	}
	else
	{
		deleteSound();
	}
	return true;
}

void Rotating::fullOpened()
{
	if (!lDoorFopened.empty())
	{
	LogManager::getSingleton().logMessage("Running script:"+lDoorFopened);
RunLuaScript(global::getSingleton().getLuaState(),lDoorFopened.c_str());
	}
}

void Rotating::fullClosed()
{
	if (!lDoorFclosed.empty())
	{
	LogManager::getSingleton().logMessage("Running script:"+lDoorFclosed);
RunLuaScript(global::getSingleton().getLuaState(),lDoorFclosed.c_str());
	}
}