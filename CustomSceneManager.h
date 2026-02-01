#ifndef __CUSTOM_SCENE_MANAGER__H__
#define    __CUSTOM_SCENE_MANAGER__H__
 
//#include <OgreNoMemoryMacros.h>
#include <OgreSceneManagerEnumerator.h>
//#include <OgreMemoryMacros.h>
 
using namespace std;

#define OCCLUSION_CULLING
//#define OCCLUSION_CULLING2

class CustomSceneManagerFactory : public Ogre::SceneManagerFactory
{
protected:
    void initMetaData(void) const;
public:
    CustomSceneManagerFactory() {}
    ~CustomSceneManagerFactory() {}
    /// Factory type name
    static const Ogre::String FACTORY_TYPE_NAME;
    Ogre::SceneManager* createInstance(const Ogre::String& instanceName);
    void destroyInstance(Ogre::SceneManager* instance);
};
 
class CustomSceneManager : public Ogre::SceneManager
{
public:
    CustomSceneManager(const Ogre::String& name);
    ~CustomSceneManager();
    const Ogre::String& getTypeName(void) const;
 
    /** Internal utility method for rendering a single object. 
    @remarks
        Assumes that the pass has already been set up.
    @param rend The renderable to issue to the pipeline
    @param pass The pass which is being used
    @param doLightIteration If true, this method will issue the renderable to
        the pipeline possibly multiple times, if the pass indicates it should be
        done once per light
    @param manualLightList Only applicable if doLightIteration is false, this
        method allows you to pass in a previously determined set of lights
        which will be used for a single render of this object.
    */
	void inspectRenderable(Ogre::Renderable* rend)
	{
		g_InspectRenderable = rend;
	}
	
	unsigned int getPixelCount()
	{
		return g_PixelCount;
	}
	
	int getPixelCount(Ogre::Renderable* rend)
	{
		if (occ.find(rend)!=occ.end())
		{
			int temp = occ[rend];
			occ[rend]=-1; //The most important part. Otherwise, after disappear, objects are not visible. 
			//				Thus, without the affect to the OC code. Thus, they won't appear.
			return temp;
		}
		else
		{
			return -1;
		}
	}


    virtual void renderSingleObject(Ogre::Renderable* rend, const Ogre::Pass* pass, 
			bool lightScissoringClipping, bool doLightIteration, const Ogre::LightList* manualLightList = 0);
private:
	map<Ogre::Renderable*,unsigned int> occ;
	Ogre::Renderable* g_InspectRenderable;
	unsigned int g_PixelCount;

};
 
class RenderableOcclusionPair
{
public:
    RenderableOcclusionPair(Ogre::Renderable* _renderable);
    ~RenderableOcclusionPair();
 
    ///
    /// Returns the pixel count, and changes the state
    /// to continue the next hardware query
    ///
    unsigned int GetPixelCount();
 
    ///
    /// Returns the renderable to query
    ///
    Ogre::Renderable* GetRenderable();
 
    ///
    /// Start the query, if the state
    /// is ready to render.
    ///
    void BeginOcclusionQuery();
 
    ///
    /// Finish the query, if the state
    /// is ready to render. Changes
    /// the state to wait for access
    ///
    void EndOcclusionQuery();
 
protected:
 
    ///
    /// HardwareOcclusionQuery
    ///
    Ogre::HardwareOcclusionQuery*    m_HardwareOcclusionQuery;
 
    ///
    /// Renderable
    ///
    Ogre::Renderable*                m_Renderable;
 
    ///
    /// Pixel Count
    ///
    unsigned int                    m_PixelCount;
 
    ///
    /// Pixel State
    ///
    enum PixelState
    {
        READY_FOR_RENDER,
        QUERY_STARTED,
        READY_FOR_ACCESS,
        __ENUM_MAX__ // must be last
    };
 
    ///
    /// PixelState
    ///
    PixelState                        m_PixelState;
};
 
#endif    // __CUSTOM_SCENE_MANAGER__H__