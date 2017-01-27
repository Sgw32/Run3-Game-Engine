/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Bullet.h"

Bullet::Bullet()
{
startadd = false;
}

Bullet::~Bullet()
{

}

void Bullet::init(Ogre::Vector3 pos,Ogre::Vector3 dir,Ogre::Real force,OgreNewt::World* world,Ogre::SceneManager* scene)
{
mPos = pos;
mDir = dir;
mForce = force;
mWorld = world;
Ogre::SceneNode* node = scene->getRootSceneNode()->createChildSceneNode(mPos);
OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, Ogre::Vector3(10,1,10));
bod = new OgreNewt::Body( mWorld, col );
bod->attachToNode( node );
bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
delete col;
inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( 1, Ogre::Vector3(10,1,10) );
bod->setMassMatrix( 1, inertia );
Ogre::String n = node->getName();
bod->setName(n);
bod->setCustomForceAndTorqueCallback<Bullet>( &Bullet::bullet_force_callback ,this);
}

void Bullet::start()
{
	startadd = true;
}

void Bullet::bullet_force_callback(OgreNewt::Body* me)
{
  if (startadd)
  {
	  me->addForce(mDir*mForce);
  }
}