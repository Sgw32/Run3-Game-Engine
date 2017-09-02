/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "func_door.h"

func_door::func_door()
{
	nowfire = false;
	opened=false;
	closed=true;
	funcrot=false;
	mTime=0;
	rotDoor=false;
	sop=false;
	allowPReg=false;
	scl=false;
	locked=false;
mInteract=false;
	mAngle=0;
	mCurAngle=0;
	lDoorFopened=lDoorFclosed=lDoorS="";
	lLocked="";
	mAcceleration=0;
	//mpt2=0;
	//myt2=0;
	//mrt2=0;
}

func_door::~func_door()
{
}

void func_door::setup(Ogre::SceneNode* door_node,bool box)
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

void func_door::setup(Ogre::SceneNode* door_node)
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

void func_door::setDistanceDirection(Ogre::Real distance,Ogre::Vector3 dir,Ogre::Real speed) 
{
	dist = distance;
	direction = dir;
	mspeed = speed;
}

void func_door::Fire(String event)
{
	if (event=="lock")
	{
		locked=true;
		return;
	}
	if (event=="unlock")
	{
		LogManager::getSingleton().logMessage("Unlocking door");
		locked=false;
		return;
	}
	if (locked)
	{
		LogManager::getSingleton().logMessage("DOOR IS LOCKED!");
		if (!(lLocked.empty()))
		{
			RunLuaScript(global::getSingleton().getLuaState(),lLocked.c_str());
		}
		return;
	}
	if (funcrot)
	{
		if (event=="toggle")
		{
			nowfire = !nowfire;
		}
		if (event=="open")
		{
			//if (closed)
			nowfire = true;
		}
		if (event=="close")
		{
			//if (opened)
			nowfire = false;
		}
	}

	if ((!nowfire)&&(!funcrot))
	{
		LogManager::getSingleton().logMessage("Running script:"+lDoorS);
		if (!lDoorS.empty())
		RunLuaScript(global::getSingleton().getLuaState(),lDoorS.c_str());
	if (event=="toggle")
	{
		if (opened)
			cur_event="close";
		if (closed)
			cur_event="open";
		nowfire = true;
	}
	else
	{

		LogManager::getSingleton().logMessage("Opened:"+StringConverter::toString(opened));
		LogManager::getSingleton().logMessage("Closed:"+StringConverter::toString(closed));
		LogManager::getSingleton().logMessage(event);
		if ((opened)&&(event=="close"))
		{
			cur_event="close";
			nowfire = true;
		}
		if ((closed)&&(event=="open"))
		{
			cur_event="open";
			nowfire = true;
		}
	}
	fPosition = mNode->getPosition() + (direction * dist);
	time = dist / mspeed;
	
	if (cur_event=="close" && scl && opened)
	{
		deleteSound();
		if (mOnClose!="none")
		{
		sMgr->loadAudio(mOnClose,&sound,true);
		sMgr->setSound(sound,fPosition,Vector3::ZERO,Vector3::ZERO,1000,false,false,200,50,800);
		sMgr->playAudio(sound,false);
		}
		//Run3SoundRuntime::getSingleton().emitSound(mOnClose,time,true);
	}
	if (cur_event=="open" && sop && closed)
	{
		
		deleteSound();
		if (mOnClose!="none")
		{
		sMgr->loadAudio(mOnClose,&sound,true);
		sMgr->setSound(sound,fPosition,Vector3::ZERO,Vector3::ZERO,1000,false,false,200,50,800);
		sMgr->playAudio(sound,false);
		}
		//Run3SoundRuntime::getSingleton().emitSound(mOnOpen,time,true);
	}
	}
}

Vector3 func_door::get_position()
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
return oldpos;
}

void func_door::set_position(Vector3 pos)
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
door_bod->setPositionOrientation(pos,quat);
}

void func_door::set_orientation(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat);
}

void func_door::rotateBody(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat+quat2);
}

void func_door::setname(String name)
{
	mName = name;
}

String func_door::getname()
{
	return mName;
}

Ogre::Real func_door::getMass(OgreNewt::Body* bod)
{
	Vector3 inertia;
	Real mass;
	bod->getMassMatrix(mass,inertia);
	return mass;
}

OgreNewt::Body* func_door::getBodyOnTop()
{

	Vector3 pos = global::getSingleton().getPlayer()->get_location();
	Vector3 size = global::getSingleton().getPlayer()->get_cur_size();
 	Ogre::AxisAlignedBox plyaab = Ogre::AxisAlignedBox(pos-size-Vector3(100,100,100),pos+size+Vector3(100,100,100));
	OgreNewt::Body* l_bod = 0;
	if (aab.intersects(plyaab))
		l_bod = global::getSingleton().getPlayer()->bod;
	return l_bod;

}

void func_door::unload()
{
	deleteSound();
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

bool func_door::frameStarted(const Ogre::FrameEvent &evt)
{

	if (evt.timeSinceLastFrame>3.0f)
		return true;
	mspeed+=mAcceleration*evt.timeSinceLastFrame;
	rotSpeed+=mAcceleration*evt.timeSinceLastFrame;
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
								Real mpt1= -rotSpeed*evt.timeSinceLastFrame*5*Ogre::Math::Sign(mp1)*TIME_SHIFT;
								Real myt1= -rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(my1);
								Real mrt1= -rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(mr1);
								mNode->pitch(Degree(mpt1));
								mNode->yaw(Degree(myt1));
								mNode->roll(Degree(mrt1));
							}
							else
							{ 
								set_position(get_position()-vDirection*evt.timeSinceLastFrame*TIME_SHIFT/0.2f);
							}
				}


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
					/*Quaternion tRot = Quaternion(Radian(Degree(velocity)),mAxis);
					rotateBody(tRot);
					Vector3 pos;ll
					door_bod->getPositionOrientation(pos,tRot);*/
					/*mNode->rotate(mAxis,Radian(Degree(velocity)));
					set_orientation(mNode->getOrientation());
					Quaternion tRot = mNode->getOrientation();
					Quaternion resul = mStartAngle+mQuatAngle;
					Degree dAngle;
					Degree dAngleF;
					Degree dAngleS;
					Vector3 axis;
					tRot.ToAngleAxis(dAngle,axis);
					resul.ToAngleAxis(dAngleF,axis);
					mStartAngle.ToAngleAxis(dAngleS,axis);
					Real angle4=mNode->get
					Real angle1 = dAngle.valueAngleUnits();
					Real angle2 = dAngleF.valueAngleUnits();
					Real angle3 = dAngleS.valueAngleUnits();
					
					if ((angle1+angle3)>(angle3+mAngle))
					{
						nowfire =false;
						opened=true;
						closed=false;
						global::getSingleton().getPlayer()->remove_additional_force();
						mTime = 0;
					}
					/*if (tRot.equaˆls(mStartAngle+mQuatAngle,Radian(Degree(20))))
					{
						nowfire =false;
						opened=true;
						closed=false;nion t
						global::getSingleton().getPlayer()->remove_additional_force();
						mTime = 0;
					}*/
					/*mNode->rotate(mAxis,Radian(Degree(mAngle)));
					set_orientation(mNode->getOrientation());
					nowfire =false;
					opened=true;
					closed=false;
					//global::getSingleton().getPlayer()->remove_additional_force();
					mTime = 0;
					
					/*Quaternion tRot = mNode->getOrientation();
					Real angle1 = tRot.*/

					if (allowPReg)
					{
						//P-regulator code.
						//Pitch channel
						//LogManager::getSingleton().logMessage("Opening rotdoor using P-reg, ");
						Real mpt1=0;
						if (mp)
							mpt1 = fabs((mp1)/mp)*rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(mp); // Delta pitch
						Real myt1=0;
						if (my)
							myt1= fabs((my1)/my)*rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(my); // Delta yaw
						Real mrt1=0;
						if (mr)
							mrt1= fabs((mr1)/mr)*rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(mr); // Delta roll
						mp1-=mpt1; // mp1 - Real
						my1-=myt1; // mp1 - Real
						mr1-=mrt1; // mp1 - Real
						//LogManager::getSingleton().logMessage(StringConverter::toString(mpt1));
						//LogManager::getSingleton().logMessage(StringConverter::toString(myt1));
						//LogManager::getSingleton().logMessage(StringConverter::toString(mrt1));
						mNode->pitch(Degree(mpt1)); // use differencial regulation.
						mNode->yaw(Degree(myt1)); // use differencial regulation.
						mNode->roll(Degree(mrt1)); // use differencial regulation.
						

						if ((fabs(mpt1)<DEGREE_DISCRAPENCY/1000)&&(fabs(myt1)<DEGREE_DISCRAPENCY/1000)&&(fabs(mrt1)<DEGREE_DISCRAPENCY/1000))
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
					else
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
				if (!rotDoor)
				{
					if (getBodyOnTop())
						global::getSingleton().getPlayer()->set_additional_force(vDirection);
					if (allowPReg)
					{
						Vector3 myPosXYZ=get_position();
						//Real errorx=fPosition.x-myPosXYZ.x;
						//Real errory=fPosition.x-myPosXYZ.y;
						//Real errorz=fPosition.x-myPosXYZ.z;
						Vector3 error = fPosition-myPosXYZ;
						set_position(myPosXYZ+error*velocity/100*evt.timeSinceLastFrame*TIME_SHIFT/0.2f); // P-regulator
						if (error.length()<10.0f)
						{
							fullOpened();
							nowfire =false;
							opened=true;
							closed=false;
							global::getSingleton().getPlayer()->remove_additional_force();
							mTime = 0;
						}
					}
					else
					{
					bool x = rel_pos.x>rel_pos_f.x;
					bool y = rel_pos.y>rel_pos_f.y;
					bool z = rel_pos.z>rel_pos_f.z;
					if (!x&&!y&&!z)
					{
						set_position(get_position()+vDirection*evt.timeSinceLastFrame*TIME_SHIFT/0.2f);
					}
					
					//mTime=mTime+evt.timeSinceLastFrame;
					if (x||y||z)
					{
						fullOpened();
						nowfire =false;
						opened=true;
						closed=false;
						global::getSingleton().getPlayer()->remove_additional_force();
						mTime = 0;
					}
					}
				}
			}
		}
		}
		if (cur_event=="close")
		{
		if (!closed)
		{
			vDirection = direction * velocity;
			Vector3 rel_pos = get_position()*direction;
			Vector3 rel_pos_f = sPosition*direction;

			if (funcrot)
				{
							if (rotDoor)
							{
								Real mpt1= -rotSpeed*evt.timeSinceLastFrame*5*Ogre::Math::Sign(mp1)*TIME_SHIFT;
								Real myt1= -rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(my1);
								Real mrt1= -rotSpeed*evt.timeSinceLastFrame*5*TIME_SHIFT*Ogre::Math::Sign(mr1);
								mNode->pitch(Degree(mpt1));
								mNode->yaw(Degree(myt1));
								mNode->roll(Degree(mrt1));
							}
							else
							{ 
								set_position(get_position()-vDirection*evt.timeSinceLastFrame*TIME_SHIFT/0.2f);
							}
				}
				else
				{
			if (rotDoor)
			{
					/*Quaternion tRot = Quaternion(Radian(Degree(-velocity)),mAxis);
					rotateBody(tRot);
					Vector3 pos;
					door_bod->getPositionOrientation(pos,tRot);
					mNode->rotate(mAxis,Radian(Degree(-velocity)));
					set_orientation(mNode->getOrientation());
					Quaternion tRot = mNode->getOrientation();
					if (tRot.equals(mStartAngle+mQuatAngle,Radian(Degree(20))))
					{
					nowfire =false;
					opened=false;
					global::getSingleton().getPlayer()->remove_additional_force();
					closed=true;
					mTime = 0;
					}*/
				//LogManager::getSingleton().logMessage("Closing a door!");
				if (allowPReg)
				{
					//P-regulator code.
					//Pitch channel
					
					Real mpt1=0;
					if (mp)
						mpt1 = -fabs((mp-mp1)/mp)*rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(mp); // Delta pitch
					Real myt1=0;
					if (my)
						myt1 = -fabs((my-my1)/my)*rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(my); // Delta yaw
					Real mrt1=0;
					if (mr)
						mrt1= -fabs((mr-mr1)/mr)*rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(mr); // Delta roll
					mp1-=mpt1; // mp1 - Real
					my1-=myt1; // mp1 - Real
					mr1-=mrt1; // mp1 - Real
					
					mNode->pitch(Degree(mpt1)); // use differencial regulation.
					mNode->yaw(Degree(myt1)); // use differencial regulation.
					mNode->roll(Degree(mrt1)); // use differencial regulation.
					

					if ((fabs(mpt1)<DEGREE_DISCRAPENCY/1000)&&(fabs(myt1)<DEGREE_DISCRAPENCY/1000)&&(fabs(mrt1)<DEGREE_DISCRAPENCY/1000))
					{
						nowfire =false;
						opened=false;
						closed=true;
						mTime = 0;
					}
				}
				else
				{
				if (!PRIMERNO_RAVNO(mp1,mp,DEGREE_DISCRAPENCY))
					{
						Real mpt1= -rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(mp);
						//LogManager::logMessage(
						//LogManager::getSingleton().logMessage(StringConverter::toString(mp1));
						//LogManager::getSingleton().logMessage(StringConverter::toString(mp));
						if (fabs(fabs(mp-mp1)-mpt1)>=DEGREE_DISCRAPENCY)
						{
						mNode->pitch(Degree(mpt1));
						mp1-=mpt1;
						}
						else
						{
							
						mNode->pitch(Degree(mp-mp1));
						mp1=mp;
						}
						
							set_orientation(mNode->getOrientation());
					}
					if (!PRIMERNO_RAVNO(my1,my,DEGREE_DISCRAPENCY))
					{
						Real myt1= -rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(my);
						//LogManager::getSingleton().logMessage(StringConverter::toString(my1));
						//LogManager::getSingleton().logMessage(StringConverter::toString(my));
						if (fabs(fabs(my-my1)-myt1)>=DEGREE_DISCRAPENCY)
						{
						mNode->yaw(Degree(myt1));
						my1-=myt1;
						}
						else
						{
							
						mNode->yaw(Degree(my-my1));
						my1=my;
						}
						
							set_orientation(mNode->getOrientation());
					}
					if (!PRIMERNO_RAVNO(mr1,mr,DEGREE_DISCRAPENCY))
					{
						Real mrt1= -rotSpeed*evt.timeSinceLastFrame*TIME_SHIFT*5*Ogre::Math::Sign(mr);
						LogManager::getSingleton().logMessage(StringConverter::toString(mr1));
						LogManager::getSingleton().logMessage(StringConverter::toString(mr));
						if (fabs(fabs(mr-mr1)-mrt1)>=DEGREE_DISCRAPENCY)
						{
						mNode->roll(Degree(mrt1));
						mr1-=mrt1;
						}
						else
						{
							
						mNode->roll(Degree(mr-mr1));
						mr1=mr;
						}
						
							set_orientation(mNode->getOrientation());
					}
					if (PRIMERNO_RAVNO(mp1,mp,DEGREE_DISCRAPENCY)&&
						PRIMERNO_RAVNO(my1,my,DEGREE_DISCRAPENCY)&&
						PRIMERNO_RAVNO(mr1,mr,DEGREE_DISCRAPENCY))
					{
						nowfire =false;
					opened=false;
					//LogManager::logMessage("DOOR CLOSED");
					closed=true;
					mTime = 0;
					}
				}
			}
				}
			if (!rotDoor)
			{
				if (getBodyOnTop())
					getBodyOnTop()->addForce(getMass(getBodyOnTop())*vDirection);
				if (allowPReg)
				{
						Vector3 myPosXYZ=get_position();
						//Real errorx=fPosition.x-myPosXYZ.x;
						//Real errory=fPosition.x-myPosXYZ.y;
						//Real errorz=fPosition.x-myPosXYZ.z;
						Vector3 error = sPosition-myPosXYZ;
						set_position(myPosXYZ+error*velocity/100*evt.timeSinceLastFrame*TIME_SHIFT/0.2f); // P-regulator
						if (error.length()<10.0f)
						{
							fullClosed();
							nowfire =false;
							opened=false;
							global::getSingleton().getPlayer()->remove_additional_force();
							closed=true;
							mTime = 0;
						}
				}
				else
				{
				bool x = rel_pos.x<rel_pos_f.x;
				bool y = rel_pos.y<rel_pos_f.y;
				bool z = rel_pos.z<rel_pos_f.z;
				if (!x&&!y&&!z)
				{
					set_position(get_position()-vDirection*evt.timeSinceLastFrame*TIME_SHIFT/0.2f);
				}
				
				//mTime=mTime+evt.timeSinceLastFrame;
				if (x||y||z)
				{
					fullClosed();
					nowfire =false;
					opened=false;
					global::getSingleton().getPlayer()->remove_additional_force();
					closed=true;
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

void func_door::fullOpened()
{
	if (!lDoorFopened.empty())
	{
	LogManager::getSingleton().logMessage("Running script:"+lDoorFopened);
	RunLuaScript(global::getSingleton().getLuaState(),lDoorFopened.c_str());
	}
}

void func_door::fullClosed()
{
	if (!lDoorFclosed.empty())
	{
	LogManager::getSingleton().logMessage("Running script:"+lDoorFclosed);
RunLuaScript(global::getSingleton().getLuaState(),lDoorFclosed.c_str());
	}
}