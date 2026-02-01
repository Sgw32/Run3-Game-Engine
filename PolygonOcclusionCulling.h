/*A PART OF RUN3 GAME ENGINE
Polygon level Occlusion Culling by Codex 2025
*/
#pragma once

#include "Ogre.h"
#include "global.h"
#include "Manager_Template.h"
#include "CustomSceneManager.h"

using namespace Ogre;
using namespace std;

#define DEBUG_POLY_CULLING

class PolygonOcclusionCulling : public Singleton<PolygonOcclusionCulling>, public managerTemplate
{
public:
        PolygonOcclusionCulling(String manName){LogManager::getSingleton().logMessage(manName+" manager initialized!");}
        PolygonOcclusionCulling();
        virtual ~PolygonOcclusionCulling();
        virtual void init();

        /// Set how often occlusion queries are processed in seconds
        void setUpdateInterval(Real interval){mInterval = interval;}

        /// Set pixel visibility threshold
        void setPixelThreshold(unsigned int thres){mPixelThreshold = thres;}

        void passEntity(Entity* ent);
        void setEnabled(bool en){enable=en;}
        virtual void upd(const FrameEvent& evt);
        virtual void cleanup();
private:
        struct PolyDesc
        {
                Entity* parent;
                SubEntity* subEnt;
                size_t startIndex;
                size_t indexCount;
                AxisAlignedBox bounds;
        };

        vector<PolyDesc> polys;
        Real cnt;
        Real mInterval;
        unsigned int mPixelThreshold;
        bool enable;
        CustomSceneManager* mgr;
};

