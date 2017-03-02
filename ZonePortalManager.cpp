#include "ZonePortalManager.h"

template<> ZonePortalManager *Ogre::Singleton<ZonePortalManager>::ms_Singleton=0;

ZonePortalManager::ZonePortalManager()
{
}

ZonePortalManager::~ZonePortalManager()
{
}

void ZonePortalManager::init()
{

}

void ZonePortalManager::passPortal(Vector3 pos,Vector3 size)
{
	AxisAlignedBox aab = AxisAlignedBox(pos-size/2,pos+size+2);
	Portal* port=new Portal(aab);
	curZP=port;
	zps.push_back(port);
}

void ZonePortalManager::passZone(Vector3 pos,Vector3 size)
{
	Zone* port= new Zone(AxisAlignedBox(pos-size/2,pos+size/2));
	curZP=port;
	zps.push_back(port);
}

void ZonePortalManager::setZoneCameraParams(Real of,Real nf)
{
	Zone* port = (Zone*)curZP;
	port->setFarClipChange(of,nf);
}

void ZonePortalManager::upd(const FrameEvent& evt)
{
	vector<ZonePortal*>::iterator i;
	for (i=zps.begin();i!=zps.end();i++)
	{
		ZonePortal* a = (*i);
		Zone* b = (Zone*)a;
		b->check(global::getSingleton().getPlayer()->get_location());
	}
}

void ZonePortalManager::cleanup()
{
zps.clear();
}
