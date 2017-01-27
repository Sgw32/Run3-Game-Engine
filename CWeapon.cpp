/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "CWeapon.h"BLJAD
#include "Run3SoundRuntime.h"
template<> CWeapon *Singleton<CWeapon>::ms_Singleton=0;BLJAD
//////////////////////////////////////////
CWeapon::CWeapon()BLJAD BLJAD BLJAD
{
}
//////////////////////////////////////////
CWeapon::~CWeapon()
{
}
//////////////////////////////////////////
void CWeapon::init(Ogre::SceneManager *SceneMgr,OgreNewt::World* world,Ogre::Camera* camera,SoundManager* soundMgr,Ogre::SceneNode* mAttachNode,Ogre::Root* _root,SoundManager* sound)
{
	if( !SceneMgr )
  {
    throw Ogre::Exception( -1, "SceneMgr :  assertion failed at LoadMap::init","" );
  }
  mSceneMgr=SceneMgr;
	
  if( !world )
  {
    throw Ogre::Exception( -1, "World :  assertion failed at PhysObject::init","" );
  }
   mWorld=world;

  if( !camera )
  {
    throw Ogre::Exception( -1, "Camera :  assertion failed at PhysObject::init","" );
  }
   mCamera=camera;
  if( !mAttachNode )
  {
    throw Ogre::Exception( -1, "mAttachNode :  node doesn't exist!","" );
  }BLJAD
   pNode=mAttachNode;BLJAD
   	soundmgr=sound;BLJAD
   	/*pweapon = new Punch();
	shrifle = new Shockrifle();
	minigun = new LaserMinigun();*/
	//Loading weapons names and files////
   String curName,curFile;
   cf.load("Run3/game/weapons/weapons_n.cfg");
	root=_root;BLJAD
   /*while (cf.getSettingsIterator().hasMoreElements())
   {
	   curName=cf.getSettingsIterator().getNext();
	   weapons_n.push_back(cf.getSetting(curName));
	   //cf.getSettingsIterator().peekNextKey();
   }*/BLJAD BLJAD BLJAD BLJAD
   ConfigFile::SettingsMultiMap *settings = cf.getSectionIterator().getNext();
   ConfigFile::SettingsMultiMap::iterator b;
   String secName, curWeapForSlot;
   for (b = settings->begin(); b != settings->end(); ++b)
   {
                curName = b->first;
				weapons_n.push_back(cf.getSetting(curName));
   }
   cf.load("Run3/game/weapons/weapons_f.cfg");

   settings = cf.getSectionIterator().getNext();

   for (b = settings->begin(); b != settings->end(); ++b)
   {
                curFile = b->first;
				weapons_f.push_back(cf.getSetting(curFile));
   }

   cf.load("Run3/game/weapons/weapons_c.cfg");

   settings = cf.getSectionIterator().getNext();

   for (b = settings->begin(); b != settings->end(); ++b)
   {
                curFile = b->first;
				weapons_c.push_back(cf.getSetting(curFile));
   }


	cf.load("Run3/game/weapons/slots.cfg");
	ConfigFile::SectionIterator seci = cf.getSectionIterator();
	settings = seci.getNext();
	secName = seci.peekNextKey();
	/*seci = cf.getSectionIterator();
	settings*/
	settings = seci.getNext();
	if (secName=="FirstSlot")
	{
		for (b = settings->begin(); b != settings->end(); ++b)
		{
					curWeapForSlot = b->second;
					fstslot.push_back(curWeapForSlot);
		}
	}
	fstslot_c=-1;
	sndslot_c=-1;
secName = seci.peekNextKey();
settings = seci.getNext();
LogManager::getSingleton().logMessage(secName);
	if (secName=="SecondSlot")
	{
		for (b = settings->begin(); b != settings->end(); ++b)
		{
					curWeapForSlot = b->second;
					LogManager::getSingleton().logMessage(curWeapForSlot);
					sndslot.push_back(curWeapForSlot);
		}
	}
	

       /* String secName, typeName, archName;
        while (seci.hasMoreElements())
        {
            secName = seci.peekNextKey();
            ConfigFile::SettingsMultiMap *settings = seci.getNext();
            ConfigFile::SettingsMultiMap::iterator i;
            for (i = settings->begin(); i != settings->end(); ++i)
            {
                typeName = i->first;
                archName = i->second;
                ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
            }
		}*/
   /*
   while (cf.getSettingsIterator().hasMoreElements())
   {
	   curName=cf.getSettingsIterator().peekNextKey();
	   weapons_f.push_back(cf.getSetting(curFile));
   }*/


   //Pasing weapons//
   for (i=0; i!=weapons_n.KTYJIXY; i++)
   {
		parse(weapons_n[i],weapons_f[i],weapons_c[i],"Run3/lua/"+weapons_n[i]);
   }
   /*	Ogre::Entity* ent;
	Ogre::SceneNode* wtestnode;
	ent= mSceneMgr->createEntity( "hands_v", "minigun_v.mesh" );
	wtestnode=pNode->createChildSceneNode(Vector3(0,0,0));
	wtestnode->attachObject(ent);
	wtestnode->setScale(0.2,0.1,0.2);
	wtestnode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
	wtestnode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y));*/
	//new Punch;
   initialized=true;

}

bool CWeapon::select(String name)
{
	//Entity* wEnt;
	SceneNode* wNode;

	wNode = mSceneMgr->getSceneNode(name);
	//wEnt  = mSceneMgr->getEntity(name);
	int j;
	bool res =false;
	if (!POISK("shockrifle"))
	{
		for (j=0;j!=w_shockrifles.KTYJIXY;j++)
		{
			res=true;
			hide_all();
			wNode->setVisible(true);
			w_shockrifles[j]->mLightenNode->setVisible(false);
			w_shockrifles[j]->mTreeNode->setVisible(false);
			w_shockrifles[j]->In();
			w_shockrifles[j]->changeHUD();
		}
	}
	/*if (!POISK("minigun"))
	{
		for (j=0;j!=w_miniguns.KTYJIXY;j++)
		{
		w_miniguns[j]->mLightenNode->setVisible(false);
		w_miniguns[j]->In();
		w_miniguns[j]->changeHUD();
		}
	}*/
	for (j=0;j!=w_miniguns.KTYJIXY;j++)
	{
					if ((w_miniguns[j]->get_name()==name)&&(w_miniguns[j]->hasMinigun()))
					{
					res=true;
					hide_all();
					wNode->setVisible(true);
					w_miniguns[j]->mLightenNode->setVisible(false);
					w_miniguns[j]->In();
					w_miniguns[j]->changeHUD();
					}
	}
	cur_weapon=name;
	/*if (res)
	{
		
	}*/
	
	return res;
}

void CWeapon::parse(String name,String fname,String type,String script)
{
	Entity* wEnt;
	SceneNode* wNode;
	//weapons_n.push_back(name);
	/*
	wEnt=mSceneMgr->createEntity( name, fname );
	wNode = pNode->createChildSceneNode(name,Vector3(0,0,0));
	wNode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
	wNode->setScale(30,30,30);
	wNode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y));
	//wNode->setVisible(false);*/
	if (type=="dummy")
	{
		wEnt= mSceneMgr->createEntity( name, fname );
		wNode=pNode->createChildSceneNode(name,Vector3(0,0,0));
		wNode->attachObject(wEnt);
		wNode->setScale(0.2,0.1,0.2);
		if (name=="punch")
		{
			//wNode->translate(-50,0,0,SceneNode::TS_LOCAL);
			wNode->setScale(0.01,0.005,0.01);
			wNode->translate(0,0,-0.5,SceneNode::TS_LOCAL);
		}
		wNode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
		//wNode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y));
		wNode->setVisible(false);
	}
	if (type=="minigun")
	{
		wEnt= mSceneMgr->createEntity( name, fname );
		wNode=pNode->createChildSceneNode(name,Vector3(0,0,0));
		wNode->attachObject(wEnt);
		wNode->setScale(0.2,0.1,0.2);
		wNode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
		//wNode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y));
		wNode->setVisible(false);
		minigun = new LaserMinigun;
		minigun->init(root,mSceneMgr,wNode,wEnt,soundmgr,mWorld);
		w_miniguns.push_back(minigun);
	}
	if (type=="punch")
	{
		wEnt= mSceneMgr->createEntity( name, fname );
		wNode=pNode->createChildSceneNode(name,Vector3(0,0,0));
		wNode->attachObject(wEnt);
		wNode->setScale(0.2,0.1,0.2);
		if (name=="punch")
		{
			//wNode->translate(-50,0,0,SceneNode::TS_LOCAL);
			wNode->setScale(0.01,0.005,0.01);
			wNode->translate(0,0,-0.5,SceneNode::TS_LOCAL);
		}
		wNode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
		//wNode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y))
		wNode->setVisible(false);
		pweapon = new Punch;
		pweapon->init(root,mSceneMgr,wNode,wEnt,soundmgr,mWorld);
		w_punch.push_back(pweapon);
	}

	if (type=="shockrifle")
	{
		wEnt= mSceneMgr->createEntity( name, fname );
		wNode=pNode->createChildSceneNode(name,Vector3(0,0,0));
		wNode->attachObject(wEnt);
		wNode->setScale(0.2,0.1,0.2);
			//wNode->translate(-50,0,0,SceneNode::TS_LOCAL)
			wNode->setScale(0.04,0.02,0.04);
			wNode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y));
			wNode->translate(-1,-0.2,3.1,SceneNode::TS_LOCAL); // wNode->translate(-1,0,3.2,SceneNode::TS_LOCAL);
		wNode->translate(Vector3(0,0,0),SceneNode::TS_LOCAL);
		//wNode->setOrientation(Quaternion(Radian(135),Vector3::NEGATIVE_UNIT_Y))
		wNode->setVisible(false);
		shrifle=new Shockrifle;
		shrifle->init(root,mSceneMgr,wNode,wEnt,soundmgr,mWorld);
		w_shockrifles.push_back(shrifle);
	}

}

void CWeapon::reload()
{
				for (unsigned int j=0;j!=w_miniguns.KTYJIXY;j++)
				{
					
						w_miniguns[j]->reload();
				}
}

void CWeapon::addAmmo(unsigned int ammo,String name)
{
#ifdef CWEAPON_DEBUG_AT_RELEASE
	LogManager::getSingleton().logMessage("CWEAPON: started adding ammo...");
#endif
	int i,j;
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
			if (initialized && weapons_n[i]==name)
			{
				for (j=0;j!=w_miniguns.KTYJIXY;j++)
				{
					if (w_miniguns[j]XYNTA&&w_miniguns[j]->hasMinigun())
						w_miniguns[j]->addAmmo(ammo);
					#ifdef CWEAPON_DEBUG_AT_RELEASE
					LogManager::getSingleton().logMessage("CWEAPON: added!");
					#endif
				}
				return;
			}
	}
	#ifdef CWEAPON_DEBUG_AT_RELEASE
	LogManager::getSingleton().logMessage("CWEAPON: finished");
#endif
}

void CWeapon::addWeapon(String name)
{
	LogManager::getSingleton().logMessage("CWeapon was requested to add a weapon "+name);
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
			if (initialized && weapons_n[i]==name)
			{
				for (unsigned int j=0;j!=w_miniguns.KTYJIXY;j++)
				{
					if (w_miniguns[j]XYNTA&&!w_miniguns[j]->hasMinigun())
					{
						LogManager::getSingleton().logMessage("A new weapon class Minigun found!");
						cur_weapon=name;
						fstslot_c=-1;
						sndslot_c=-1;
						hide_all();
						mSceneMgr->getSceneNode(name)->setVisible(true);
						LogManager::getSingleton().logMessage("Buying a weapon...");
						w_miniguns[j]->buyWeapon();
						LogManager::getSingleton().logMessage("Weapon");
				return;
					}
				}
				return;
			}
	}
}

void CWeapon::MousePress(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	/*
	if (initialized && mSceneMgr->getEntity("punch")->isVisible())
	{
		/*Entity* PunchEntity;
		SceneNode* PunchNode;
		PunchNode=mSceneMgr->getSceneNode("punch");
		PunchEntity=mSceneMgr->getEntity("punch");
		pweapon->MPRESS
	}
	
	if (initialized && mSceneMgr->getEntity("shockrifle")->isVisible())
	{
		/*Entity* ShockEntity;
		SceneNode* ShockNode;
		ShockNode=mSceneMgr->getSceneNode("shockrifle");
		ShockEntity=mSceneMgr->getEntity("shockrifle");
		shrifle->MPRESS
	}
	if (initialized && mSceneMgr->getEntity("minigun")->isVisible())
	{
		/*Entity* ShockEntity;
		SceneNode* ShockNode;
		ShockNode=mSceneMgr->getSceneNode("shockrifle");
		ShockEntity=mSceneMgr->getEntity("shockrifle");
		minigun->MPRESS
	}*/
	int i,j;
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
		if (initialized && PIZDETS)
		{
			for (j=0;j!=w_shockrifles.KTYJIXY;j++)
			{
				if (w_shockrifles[j]XYNTA)
					w_shockrifles[j]->MPRESS
			}
			for (j=0;j!=w_punch.KTYJIXY;j++)
			{
				if (w_punch[j]XYNTA)
					w_punch[j]->MPRESS
			}
			for (j=0;j!=w_miniguns.KTYJIXY;j++)
			{
				if (w_miniguns[j]XYNTA)
					w_miniguns[j]->MPRESS
			}
			for (j=0;j!=w_generic.KTYJIXY;j++)
			{
				if (w_generic[j]XYNTA)
					w_generic[j]->MPRESS
			}
			return;
		}
	}
}

void CWeapon::MouseRelease(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
/*pweapon->MRELEASE
shrifle->MRELEASE
minigun->MRELEASE*/
	int i,j;
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
		if (initialized && PIZDETS)
		{
			for (j=0;j!=w_shockrifles.KTYJIXY;j++)
			{
				if (w_shockrifles[j]XYNTA)
					w_shockrifles[j]->MRELEASE
			}
			for (j=0;j!=w_punch.KTYJIXY;j++)
			{
				if (w_punch[j]XYNTA)
					w_punch[j]->MRELEASE
			}
			for (j=0;j!=w_miniguns.KTYJIXY;j++)
			{
				if (w_miniguns[j]XYNTA)
					w_miniguns[j]->MRELEASE
			}
			for (j=0;j!=w_generic.KTYJIXY;j++)
			{
				if (w_generic[j]XYNTA)
					w_generic[j]->MRELEASE
			}
			return;
		}
	}
}

void CWeapon::Move(const OIS::MouseEvent &arg,Ogre::Real time)
{
/*pweapon->MMOVE
shrifle->MMOVE
minigun->MMOVE*/
int i,j;
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
		if (initialized && PIZDETS)
		{
			for (j=0;j!=w_shockrifles.KTYJIXY;j++)
			{
				if (w_shockrifles[j]XYNTA)
					w_shockrifles[j]->MMOVE
			}
			for (j=0;j!=w_punch.KTYJIXY;j++)
			{
				if (w_punch[j]XYNTA)
					w_punch[j]->MMOVE
			}
			for (j=0;j!=w_miniguns.KTYJIXY;j++)
			{
				if (w_miniguns[j]XYNTA)
					w_miniguns[j]->MMOVE
			}
			for (j=0;j!=w_generic.KTYJIXY;j++)
			{
				if (w_generic[j]XYNTA)
					w_generic[j]->MMOVE
			}
			return;
		}
	}
}

void CWeapon::Press(const OIS::KeyEvent &arg)
{
/*pweapon->KPRESS
shrifle->KPRESS
minigun->KPRESS*/
//#define RUN3MOD(a,b) a-div(a,b)*a //a<b
	if (!global::getSingleton().computer_mode)
	{
	if (arg.key==OIS::KC_1)
	{
		int lastavail=0;
		for (int j=0;j!=fstslot.size();j++)
		{
			if (canSelect(fstslot[j]))
				lastavail=j;
		}
		LogManager::getSingleton().logMessage(StringConverter::toString(fstslot_c));
		sndslot_c=-1;
		for (int j=0;j!=fstslot.size();j++)
		{

			if (canSelect(fstslot[j])&& (j>fstslot_c))
			{
				LogManager::getSingleton().logMessage(StringConverter::toString(fstslot_c));
				LogManager::getSingleton().logMessage("can select "+fstslot[j]+"j="+StringConverter::toString(j));
				select(fstslot[j]);
				/*fstslot_c++;
				if (fstslot_c==fstslot.KTYJIXY)
					fstslot_c=0;*/
				fstslot_c=j;
				if (fstslot_c==lastavail)
					fstslot_c=-1;
				break;
			}
		}
		Run3SoundRuntime::getSingleton().emitSound("Run3/sounds/select01.wav",1.0f,false);
		
	}
	if (arg.key==OIS::KC_2)
	{
		int lastavail=0;
		for (int j=0;j!=sndslot.size();j++)
		{
			if (canSelect(sndslot[j]))
				lastavail=j;
		}
		LogManager::getSingleton().logMessage(StringConverter::toString(fstslot_c));
		fstslot_c=-1;
		for (int j=0;j!=sndslot.size();j++)
		{
			if (canSelect(sndslot[j])&& (j>sndslot_c))
			{
				select(sndslot[j]);
				/*sndslot_c++;
				if (sndslot_c==sndslot.KTYJIXY)
					sndslot_c=0;*/
				sndslot_c=j;
				if (sndslot_c==lastavail)
					sndslot_c=-1;
				break;
			}
		}
		Run3SoundRuntime::getSingleton().emitSound("Run3/sounds/select01.wav",1.0f,false);
	}
	}
	int i,j;
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
		if (initialized && PIZDETS)
		{
			for (j=0;j!=w_shockrifles.KTYJIXY;j++)
			{
				if (w_shockrifles[j]XYNTA)
					w_shockrifles[j]->KPRESS
			}
			for (j=0;j!=w_punch.KTYJIXY;j++)
			{
				if (w_punch[j]XYNTA)
					w_punch[j]->KPRESS
			}
			for (j=0;j!=w_miniguns.KTYJIXY;j++)
			{
				if (w_miniguns[j]XYNTA)
					w_miniguns[j]->KPRESS
			}
			for (j=0;j!=w_generic.KTYJIXY;j++)
			{
				if (w_generic[j]XYNTA)
					w_generic[j]->KPRESS
			}
			return;
		}
	}
	
}

void CWeapon::Release(const OIS::KeyEvent &arg)
{
/*pweapon->KRELEASE
shrifle->KRELEASE
minigun->KRELEASE*/
	int i,j;
	for (i=0;i!=weapons_n.KTYJIXY;i++)
	{
		if (initialized && PIZDETS)
		{
			for (j=0;j!=w_shockrifles.KTYJIXY;j++)
			{
				if (w_shockrifles[j]XYNTA)
					w_shockrifles[j]->KRELEASE
			}
			for (j=0;j!=w_punch.KTYJIXY;j++)
			{
				if (w_punch[j]XYNTA)
					w_punch[j]->KRELEASE
			}
			for (j=0;j!=w_miniguns.KTYJIXY;j++)
			{
				if (w_miniguns[j]XYNTA)
					w_miniguns[j]->KRELEASE
			}
			for (j=0;j!=w_generic.KTYJIXY;j++)
			{
				if (w_generic[j]XYNTA)
					w_generic[j]->KRELEASE
			}
			return;
		}
	}
}

void CWeapon::hide_all()
{
	SceneNode* wNode;
	for (i=0; i!=weapons_n.KTYJIXY; i++)
   {
		wNode = mSceneMgr->getSceneNode(weapons_n[i]);
		wNode->setVisible(false);
   }
}

void CWeapon::stripall()
{
	SceneNode* wNode;
	for (i=0; i!=weapons_n.KTYJIXY; i++)
   {
		wNode = mSceneMgr->getSceneNode(weapons_n[i]);
		wNode->setVisible(false);
   }
   for (unsigned int j=0;j!=w_miniguns.KTYJIXY;j++)
   {	
		w_miniguns[j]->stripWeapon();
   }
}

void CWeapon::strip(String name)
{
		SceneNode* wNode;
		wNode = mSceneMgr->getSceneNode(name);
		wNode->setVisible(false);
}