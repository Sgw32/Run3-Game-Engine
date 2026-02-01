/*A PART OF RUN3 GAME ENGINE
Software Occlusion Culling  by Sgw32 2011
*/
#pragma once

#include "Ogre.h"
#include "global.h"
#include "Manager_Template.h"
#include "CustomSceneManager.h"

using namespace Ogre;
using namespace std;
 
#define DEBUG_CULLING

class SoftwareOcclusionCulling : public Singleton<SoftwareOcclusionCulling>, public managerTemplate // Singleton initialize, another template for Run3 Game Engine compliance
{
public:
	SoftwareOcclusionCulling(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
        SoftwareOcclusionCulling();
        virtual ~SoftwareOcclusionCulling();
        virtual void init();

        /// Set how often occlusion queries are processed in seconds
        void setUpdateInterval(Real interval) { mInterval = interval; }

        /// Set pixel visibility threshold
        void setPixelThreshold(unsigned int thres) { mPixelThreshold = thres; }
	
	void passEntity(Entity* ent) //Call it every time a new entity appears
	{
		LogManager::getSingleton().logMessage("Passing entity to culling:"+ent->getName());
		zps.push_back(ent);
	}
	void setEnabled(bool en)
	{
		enable=en;
	}
	virtual void upd(const FrameEvent& evt);
	virtual void cleanup(){zps.clear();};
private:
        Real cnt;
        Real mInterval;
        unsigned int mPixelThreshold;
        bool enable;
        vector<Entity*> zps;
        CustomSceneManager* mgr;
};
