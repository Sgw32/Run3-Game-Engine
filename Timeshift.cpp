#include "Timeshift.h"
#include "global.h"
#include "MusicPlayer.h"

template<> Timeshift *Ogre::Singleton<Timeshift>::ms_Singleton=0;

Timeshift::Timeshift()
{
	mK=1.0f;
}

Timeshift::~Timeshift()
{
}

void Timeshift::toggleTStop()
{
		if (mK!=0)
		{
			mK=0;
			nod->setVisible(true);
			nod->setPosition(global::getSingleton().getPlayer()->get_location());
			nod->setOrientation(global::getSingleton().getPlayer()->get_body_orientation(global::getSingleton().getPlayer()->bod));
			//nod->rotate(Vector3::UNIT_Y,Degree(180));
		}	
		else
		{
			mK=1;
			nod->setVisible(false);
		}
}

void Timeshift::setTimeK(Real k)
{
	MusicPlayer::getSingleton().setMusicPitch(k);
	mK=k;
}

void Timeshift::init()
{
	nod=global::getSingleton().getSceneManager()->getRootSceneNode()->createChildSceneNode();
	ent=global::getSingleton().getSceneManager()->createEntity("playerent_RUN3_RES","marazm01.mesh");
	nod->attachObject(ent);
	nod->setScale(80,80,80);
	AnimationState* mAnimState = ent->getAnimationState("Idle1");
	mAnimState->setEnabled(true);
	nod->setVisible(false);
}

