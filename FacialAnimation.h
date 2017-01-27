#pragma once
#include <Ogre.h>
#include "tinyxml.h"

using namespace Ogre;
using namespace std;

#define SI_COUNT 9

#define PROCESS_ANIM 0.2f

class MorphState
{
public:
	MorphState(){mPatch=0;};
	~MorphState(){};
	void init(Real second, String key)
	{
		mKey=key;
		mSecond=second;
	}
	void init(Real second,String key,bool patch) 
	{
		mKey=key;
		mSecond=second;
		mPatch=patch;
		if (mPatch)
		{
			LogManager::getSingleton().logMessage("Experimental facial patch");
		}
	}

	inline Real getSecond(){return mSecond;}
	inline String getKey(){return mKey;}
	int getIndex()
	{
		//int pc = (mPatch+1);
		int pc=1;
		if (mKey=="A")
			return 2*pc;
		if (mKey=="E")
			return 3*pc;
		if (mKey=="O")
			return 4*pc;
		if (mKey=="U")
			return 5*pc;
		if (mKey=="I")
			return 6*pc;

		if (mKey=="B")
			return 7*pc;
		if (mKey=="C")
			return 8*pc;
		if (mKey=="D")
			return 7*pc;
		if (mKey=="F")
			return 8*pc;
		if (mKey=="G")
			return 7*pc;
		if (mKey=="H")
			return 8*pc;
		if (mKey=="J")
			return 7*pc;
		if (mKey=="K")
			return 8*pc;
		if (mKey=="L")
			return -1;
		if (mKey=="M")
			return -1;
		if (mKey=="N")
			return 7*pc;
		if (mKey=="P")
			return -1;
		if (mKey=="Q")
			return 8*pc;
		if (mKey=="R")
			return 7*pc;
		if (mKey=="S")
			return 7*pc;
		if (mKey=="T")
			return 7*pc;
		if (mKey=="V")
			return 8*pc;
		if (mKey=="W")
			return 7*pc;
		if (mKey=="X")
			return 8*pc;
		if (mKey=="Y")
			return 7*pc;
		if (mKey=="Z")
			return 8*pc;
		return 8*pc;
	}
	bool isPatched(){return mPatch>0;}
private:
	int mPatch;
	Real mSecond;
	String mKey;
};

class FacialAnimation
{
public:
	FacialAnimation();
	~FacialAnimation();
	Entity* preInit(String meshFile,String name);
	void init(String xmlFile);
	void animate(Vector3 pos);
	void upd(Real tm);
	void setSubtitle(String subt)
	{
		subtitle=subt;
	}
	void destroy()
	{
		
	}
	void updatePose(int index, float weight){   
       //get the pose list
       PoseList poseList = mesh->getPoseList();
       int curPoseIndex = index;
       //loop through each submesh
       int numSubMeshes = mesh->getNumSubMeshes();
       //skip submesh 0 since it is the shared geometry, and we have no poses on that
       for(int curSubMesh = 1; curSubMesh <= numSubMeshes; curSubMesh++)
	   {
		   try
		   {
				if (mPatched)
			   {
				   //LogManager::getSingleton().logMessage("INDEXes"+StringConverter::toString(curPoseIndex*numSubMeshes+curSubMesh-1));
                vertexPoseKeyFrameMap[curSubMesh]->updatePoseReference(curPoseIndex*numSubMeshes+curSubMesh-1, weight);
                // Dirty animation state since we're fudging this manually
                //If we don't notify dirty, Ogre doesn't know to update this state
                //and nothing happens
                head->getAnimationState("manl"+head->getName())->getParent()->_notifyDirty();    
			   }
			   else
			   {
				   //LogManager::getSingleton().logMessage("INDEX"+StringConverter::toString(index));
				vertexPoseKeyFrameMap[curSubMesh]->updatePoseReference(curPoseIndex, weight);
                // Dirty animation state since we're fudging this manually
                //If we don't notify dirty, Ogre doesn't know to update this state
                //and nothing happens
                head->getAnimationState("manl"+head->getName())->getParent()->_notifyDirty();    
			   } 
		   }
		   catch(...)
		   {
		   }
             
       }   
    }
	void updatePose(int index, float weight,bool patch){   
       //get the pose list
       PoseList poseList = mesh->getPoseList();
       int curPoseIndex = index;
       //loop through each submesh
       int numSubMeshes = mesh->getNumSubMeshes();
       //skip submesh 0 since it is the shared geometry, and we have no poses on that
       for(int curSubMesh = 1; curSubMesh <= numSubMeshes; curSubMesh++)
	   {
		   try
		   {
			   

			   if (patch)
			   {
				   //LogManager::getSingleton().logMessage("INDEXes"+StringConverter::toString(curPoseIndex+curSubMesh-1));
                vertexPoseKeyFrameMap[curSubMesh]->updatePoseReference(curPoseIndex+curSubMesh-1, weight);
                // Dirty animation state since we're fudging this manually
                //If we don't notify dirty, Ogre doesn't know to update this state
                //and nothing happens
                head->getAnimationState("manl"+head->getName())->getParent()->_notifyDirty();    
			   }
			   else
			   {
				   //LogManager::getSingleton().logMessage("INDEX"+StringConverter::toString(index));
				vertexPoseKeyFrameMap[curSubMesh]->updatePoseReference(curPoseIndex, weight);
                // Dirty animation state since we're fudging this manually
                //If we don't notify dirty, Ogre doesn't know to update this state
                //and nothing happens
                head->getAnimationState("manl"+head->getName())->getParent()->_notifyDirty();    
			   }
		   }
		   catch(...)
		   {
		   }
             
       }   
    }
	String getAttrib(TiXmlElement *XMLNode, const String &parameter, const String &defaultValue = "");
	Real getAttribReal(TiXmlElement *XMLNode, const String &parameter, Real defaultValue = 0);
	bool getAttribBool(TiXmlElement *XMLNode, const String &parameter, bool defaultValue = false);
private:
	bool mPatched;
	String subtitle;
	bool mAnimate;
	long curstate;
	Real duration;
	map<int, Ogre::VertexPoseKeyFrame*> vertexPoseKeyFrameMap; 
	MorphState curMState;
	AnimationState* manualAnimState;
	VertexPoseKeyFrame* manualKeyFrame;
	String soundFile;
	Vector3 mPos;
	Ogre::Entity* head;
	MeshPtr mesh;
	Real endAnim,timeAnim;
	vector<MorphState> letters;
	TiXmlDocument   *SequenceDoc;
	TiXmlElement   *SequenceRoot;
};