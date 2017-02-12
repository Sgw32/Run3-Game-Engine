#include "SoftwareOcclusionCulling.h"

template<> SoftwareOcclusionCulling *Ogre::Singleton<SoftwareOcclusionCulling>::ms_Singleton=0;

SoftwareOcclusionCulling::SoftwareOcclusionCulling()
{
	cnt=1.0f;
}

SoftwareOcclusionCulling::~SoftwareOcclusionCulling()
{
}

void SoftwareOcclusionCulling::init()
{
	mgr = (CustomSceneManager*)global::getSingleton().getSceneManager(); //TODO Replace with your mSceneMgr SceneManager
}

void SoftwareOcclusionCulling::upd(const FrameEvent& evt) // Feed it from some registered framelistener
{
	
	if (!enable)
		return;
	cnt-=evt.timeSinceLastFrame;
	if (cnt<0)
	{
		cnt=1.0f;
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
					if ((pixelc<100)&&(pixelc!=-1))
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
