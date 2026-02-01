#include "PolygonOcclusionCulling.h"

template<> PolygonOcclusionCulling *Ogre::Singleton<PolygonOcclusionCulling>::ms_Singleton = 0;

PolygonOcclusionCulling::PolygonOcclusionCulling()
{
        cnt = 0.0f;
        mInterval = 0.5f;
        mPixelThreshold = 2;
        enable = true;
}

PolygonOcclusionCulling::~PolygonOcclusionCulling()
{
}

void PolygonOcclusionCulling::init()
{
        mgr = static_cast<CustomSceneManager*>(global::getSingleton().getSceneManager());
        SceneManager::MovableObjectIterator it = mgr->getMovableObjectIterator("Entity");
        while(it.hasMoreElements())
        {
                MovableObject* mo = it.getNext();
                Entity* e = dynamic_cast<Entity*>(mo);
                if(e)
                        passEntity(e);
        }
}

void PolygonOcclusionCulling::passEntity(Entity* ent)
{
        unsigned int numSubs = ent->getNumSubEntities();
        for(unsigned int i=0;i<numSubs;i++)
        {
                SubEntity* se = ent->getSubEntity(i);
                const SubMesh* sm = se->getSubMesh();
                const IndexData* id = sm->indexData;
                PolyDesc pd;
                pd.parent = ent;
                pd.subEnt = se;
                pd.startIndex = id->indexStart;
                pd.indexCount = id->indexCount;
                pd.bounds = se->_getWorldAABB();
                polys.push_back(pd);
        }
}

void PolygonOcclusionCulling::upd(const FrameEvent& evt)
{
        if(!enable)
                return;
        cnt -= evt.timeSinceLastFrame;
        if(cnt <= 0)
        {
                cnt = mInterval;
                vector<PolyDesc>::iterator it;
                for(it=polys.begin();it!=polys.end();++it)
                {
                        int pixels = mgr->getPixelCount(it->subEnt);
                        if((pixels < (int)mPixelThreshold) && (pixels != -1))
                        {
                                it->subEnt->setVisible(false);
                        }
                        else
                        {
                                it->subEnt->setVisible(true);
                        }
                }
        }
}

void PolygonOcclusionCulling::cleanup()
{
        polys.clear();
}

