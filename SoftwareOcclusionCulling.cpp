#include "SoftwareOcclusionCulling.h"

template<> SoftwareOcclusionCulling *Ogre::Singleton<SoftwareOcclusionCulling>::ms_Singleton=0;

SoftwareOcclusionCulling::SoftwareOcclusionCulling()
{
        cnt = 0.0f;
        mInterval = 0.1f;
        mPixelThreshold = 5;
        enable = true;
}

SoftwareOcclusionCulling::~SoftwareOcclusionCulling()
{
}

void SoftwareOcclusionCulling::init()
{
        mgr = static_cast<CustomSceneManager*>(global::getSingleton().getSceneManager());

        // Automatically gather all existing entities
        SceneManager::MovableObjectIterator it = mgr->getMovableObjectIterator("Entity");
        while (it.hasMoreElements())
        {
                MovableObject* mo = it.getNext();
                Entity* e = dynamic_cast<Entity*>(mo);
                if (e)
                        zps.push_back(e);
        }
}

void SoftwareOcclusionCulling::upd(const FrameEvent& evt) // Feed it from some registered framelistener
{
	
	if (!enable)
		return;
        cnt -= evt.timeSinceLastFrame;
        if (cnt <= 0)
        {
                cnt = mInterval;
		#ifdef DEBUG_CULLING
		LogManager::getSingleton().logMessage("Occlusion culling processing");
	#endif

		vector<Entity*>::iterator i;
		for (i=zps.begin();i!=zps.end();i++)
		{
			Entity* cur = (*i);
			#ifdef DEBUG_CULLING
			LogManager::getSingleton().logMessage("First entity name:"+cur->getName());
			#endif

			unsigned int cnt = cur->getNumSubEntities();
			for (unsigned int j=0;j!=cnt;j++)
			{
				SubEntity* ent2 = cur->getSubEntity(j); 
				if (ent2)
				{
					ent2->setVisible(true);
					int pixelc = mgr->getPixelCount(ent2);
					#ifdef DEBUG_CULLING
					LogManager::getSingleton().logMessage("Processing subentity.");
					LogManager::getSingleton().logMessage("Pixel count is:"+StringConverter::toString(pixelc));
					#endif
                                        if ((pixelc < (int)mPixelThreshold) && (pixelc != -1))
					{
						#ifdef DEBUG_CULLING
						LogManager::getSingleton().logMessage("Setting off "+cur->getName()+" num:"+StringConverter::toString(j));
		#endif
						ent2->setVisible(false);

					}
				}
			}
		}
	}
}
