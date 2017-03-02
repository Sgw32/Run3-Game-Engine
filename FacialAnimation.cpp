#include "FacialAnimation.h"
#include "global.h"
#include "Timeshift.h"
#include "Run3SoundRuntime.h"

FacialAnimation::FacialAnimation()
{
	mAnimate=false;
	endAnim=timeAnim=0;
	curstate=0;
	mPatched=false;
}

FacialAnimation::~FacialAnimation()
{

}


String FacialAnimation::getAttrib(TiXmlElement *XMLNode, const String &attrib, const String &defaultValue)
{
	if(XMLNode->Attribute(attrib.c_str()))
		return XMLNode->Attribute(attrib.c_str());
	else
		return defaultValue;
}

Real FacialAnimation::getAttribReal(TiXmlElement *XMLNode, const String &attrib, Real defaultValue)
{
	if(XMLNode->Attribute(attrib.c_str()))
		return StringConverter::parseReal(XMLNode->Attribute(attrib.c_str()));
	else
		return defaultValue;
}

bool FacialAnimation::getAttribBool(TiXmlElement *XMLNode, const String &attrib, bool defaultValue)
{
	if(!XMLNode->Attribute(attrib.c_str()))
		return defaultValue;

	if(String(XMLNode->Attribute(attrib.c_str())) == "true")
		return true;

	return false;
}

Entity* FacialAnimation::preInit(String meshFile,String name)
{
	/*LogManager::getSingleton().logMessage( "[FacialAnimation] 1" );     
		mesh = MeshManager::getSingleton().load(meshFile, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 2" ); 
		Animation* anim = mesh->createAnimation("manl", 0);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 3" ); 
		VertexAnimationTrack* track = anim->createVertexTrack(0, VAT_POSE);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 4" ); 
		manualKeyFrame = track->createVertexPoseKeyFrame(0);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 5" ); 
		// create pose references, initially zero
		for (int i = 0; i < SI_COUNT; ++i)
		{
			manualKeyFrame->addPoseReference(i, 0.0f);
		}
		LogManager::getSingleton().logMessage( "[FacialAnimation] 6" ); 

		head = global::getSingleton().getSceneManager()->createEntity(name, meshFile);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 7" ); 
		manualAnimState = head->getAnimationState("manl");
		LogManager::getSingleton().logMessage( "[FacialAnimation] 8" ); 
		manualAnimState->setTimePosition(0);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 9" ); 
		//manualAnimState->setEnabled(true);
		LogManager::getSingleton().logMessage( "[FacialAnimation] 10" ); 
		return head;*/


		try{
			LogManager::getSingleton().logMessage( "[FacialAnimation] 1" );     
          // Pre-load the mesh so that we can tweak it with a manual animation
          mesh = MeshManager::getSingleton().load(meshFile, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
          //we'll read through this list while loading
          PoseList poseList = mesh->getPoseList();
          //used to loop through each submesh
          int numSubMeshes = mesh->getNumSubMeshes();
          int curPoseIndex = 0;
          //create the animation on the mesh
		  if (mesh->hasAnimation("manl"+name))
		  {
			  head = global::getSingleton().getSceneManager()->createEntity(name, meshFile);
			 // LogManager::getSingleton().logMessage( "[FacialAnimation] Getting animation: manl"+name );     
			  manualAnimState =  head->getAnimationState("manl"+name);
		  manualAnimState->setTimePosition(0);
		  //LogManager::getSingleton().logMessage( "[FacialAnimation] 2" );     
           manualAnimState->setEnabled(true);
		  }
		  else
		  {
			//LogManager::getSingleton().logMessage( "[FacialAnimation] New animation: manl"+name ); 
          Animation* anim = mesh->createAnimation("manl"+name, 0);
          //skip submesh 0 since it is the shared geometry, and we have no poses on that
          for(int curSubMesh = 1; curSubMesh <= numSubMeshes; curSubMesh++){      
             //create the VertexTrack on this animation
             Ogre::VertexAnimationTrack *vt = anim->createVertexTrack(curSubMesh, VAT_POSE);
             //create the keyframe we will use to update this vertex track
             //keep all our keyframes in a map for later updating
             vertexPoseKeyFrameMap[curSubMesh] = vt->createVertexPoseKeyFrame(0);
             //add the references to each pose that applies to this subMesh
             while(poseList[curPoseIndex]->getTarget() == curSubMesh-1){
                //create a pose reference for each pose
                vertexPoseKeyFrameMap[curSubMesh]->addPoseReference(curPoseIndex, 0.0f);
                curPoseIndex++;
             }
          }
          //create the head
          head = global::getSingleton().getSceneManager()->createEntity(name, meshFile);
		 // LogManager::getSingleton().logMessage( "[FacialAnimation] Getting animation: manl"+name );
         manualAnimState =  head->getAnimationState("manl"+name);
		  manualAnimState->setTimePosition(0);
		  //LogManager::getSingleton().logMessage( "[FacialAnimation] 2" );     
           manualAnimState->setEnabled(true);
		  //LogManager::getSingleton().logMessage( "[FacialAnimation] 3" );     
          //put entity in scene
		  }
          return head;
       }
       catch(runtime_error err){
          std::string error = "ERROR: ";
          error.append(__FILE__);
          error.append(err.what());
          LogManager::getSingleton().logMessage(error);
		  return 0;
       }
}

void FacialAnimation::init(String xmlData)
{
	LogManager::getSingleton().logMessage("[FacialAnimation] Loading facial animation.");
	letters.clear();
	try
	{
		// Strip the path
		Ogre::String basename, path;
		Ogre::StringUtil::splitFilename(xmlData, basename, path);

		DataStreamPtr pStream = ResourceGroupManager::getSingleton().
			openResource( basename, "General" );

		String data = pStream->getAsString();
		// Open the .xml File
		SequenceDoc = new TiXmlDocument();
		SequenceDoc->Parse( data.c_str() );
		pStream->close();
		pStream.setNull();
		
		if( SequenceDoc->Error() )
		{
			//We'll just log, and continue on gracefully
			LogManager::getSingleton().logMessage("[FacialAnimation] TiXml reporated an error.");
			return;
		}
	}
	catch(...)
	{
		//rest=true;
		//We'll just log, and continue on gracefully
		LogManager::getSingleton().logMessage("[FacialAnimation] SaveFile does not present,or TiXml reporated an error.");
		return;
	}

	SequenceRoot = SequenceDoc->RootElement();
	if( String( SequenceRoot->Value()) != "facialanimation"  ) {
		LogManager::getSingleton().logMessage( "[FacialAnimation] Error: Invalid .xml File. Missing <facialanimation>" );     
		return;
	}
	subtitle = getAttrib(SequenceRoot,"subtitle_text","");
	if (!(global::getSingleton().getPlayer()->getAllowSubtitles()))
		subtitle="";
	mPatched=getAttribBool(SequenceRoot,"patched",false);
	LogManager::getSingleton().logMessage("[FacialAnimation] processing nodes...");
	TiXmlElement* pElement;
	pElement = SequenceRoot->FirstChildElement("node");
	
			while(pElement)
			{
				String key = getAttrib(pElement,"letter","A");
				Real time = getAttribReal(pElement,"time",0.0f);
				bool patch = getAttribBool(pElement,"patch",false);
				MorphState s;
				s.init(time,key,patch);
				letters.push_back(s);
				endAnim=time;
				duration=time+1.0f;
				pElement = pElement->NextSiblingElement("node");
				LogManager::getSingleton().logMessage("[FacialAnimation] processing node...");
			}
			
			soundFile = getAttrib(SequenceRoot->FirstChildElement("file"),"name","run3/sounds/pogran/document01.wav");
			//duration = getAttribReal(SequenceRoot->FirstChildElement("file"),"duration",3.0f);
			LogManager::getSingleton().logMessage("[FacialAnimation] Loading ok.");
	delete SequenceDoc;
}

void FacialAnimation::animate(Vector3 pos)
{
	mAnimate=true;
	mPos=pos;
	curstate=0;
	LogManager::getSingleton().logMessage("[FacialAnimation] Starting animation.");

	
		if (!(subtitle.empty()))
		{
			HUD::getSingleton().flashText(subtitle,PROCESS_ANIM,endAnim);

		}
	//manualAnimState = head->getAnimationState("manl"+head->getName());
	if (manualAnimState)
	{
	manualAnimState->setEnabled(true);
	LogManager::getSingleton().logMessage("[FacialAnimation] Ok.");
	}
	timeAnim=0;
	//curMState=letters[curstate];
}

void FacialAnimation::upd(Real tm)
{
	try
	{
	//just starting
	//if (((mAnimate)&&(timeAnim<PROCESS_ANIM))&&(curstate==0))
	if (((mAnimate))&&(curstate==0))
	{
		if (timeAnim>PROCESS_ANIM)
			timeAnim=PROCESS_ANIM;
		LogManager::getSingleton().logMessage("[FacialAnimation] Pre-ready sequence.");
		if (letters[curstate].getIndex()!=-1)
		{
			//manualKeyFrame->updatePoseReference(letters[curstate].getIndex(), timeAnim/PROCESS_ANIM);
			//manualAnimState->getParent()->_notifyDirty();
			updatePose(letters[curstate].getIndex(), timeAnim/PROCESS_ANIM);
		}
		timeAnim+=tm;
		if (timeAnim>PROCESS_ANIM)
		{
			if (letters[curstate].getIndex()!=-1)
			{
				updatePose(letters[curstate].getIndex(), 0);
			}
			curstate++; 
			Run3SoundRuntime::getSingleton().emitSound(soundFile,duration,false,mPos,200,50,800);
			LogManager::getSingleton().logMessage("[FacialAnimation] Pre-ready ended.");
		}
		LogManager::getSingleton().logMessage("[FacialAnimation] Ok pre.");
	}

	//if (((mAnimate)&&(timeAnim>PROCESS_ANIM)&&(timeAnim<endAnim+PROCESS_ANIM))&&(curstate<letters.size()))
	if (((mAnimate)&&(timeAnim>PROCESS_ANIM))&&(curstate<letters.size()))
	{
		// timeAnim-PROCESS_ANIM is time for active animation
		// Triangle functions are there
		if (timeAnim>endAnim+PROCESS_ANIM)
			timeAnim=endAnim+PROCESS_ANIM;
		Real down=(letters[curstate].getSecond()-(timeAnim-PROCESS_ANIM))/(letters[curstate].getSecond());
		Real up=1-down;
		if (letters[curstate-1].getIndex()!=-1)
		{
			updatePose(letters[curstate-1].getIndex(), down);
		}
		if (letters[curstate].getIndex()!=-1)
		{
			updatePose(letters[curstate].getIndex(), up);
		}
		//manualKeyFrame->updatePoseReference(letters[curstate-1].getIndex(), down);
		//manualKeyFrame->updatePoseReference(letters[curstate].getIndex(), up);
		
		//manualAnimState->getParent()->_notifyDirty();
		timeAnim+=tm;

		if (timeAnim>letters[curstate].getSecond()+PROCESS_ANIM)
		{
			if (letters[curstate-1].getIndex()!=-1)
			{
				updatePose(letters[curstate-1].getIndex(), 0);
			}
			if (letters[curstate].getIndex()!=-1)
			{
				updatePose(letters[curstate].getIndex(), 1);
			}
			curstate++;
		}
	}
	
	//just ending
	if ((mAnimate)&&(timeAnim>endAnim+PROCESS_ANIM))
	{
		//manualKeyFrame->updatePoseReference(letters[curstate].getIndex(), (endAnim+timeAnim)/(endAnim+PROCESS_ANIM));
		//manualAnimState->getParent()->_notifyDirty();
		if (letters[curstate-1].getIndex()!=-1)
		{
		updatePose(letters[curstate-1].getIndex(), 1.0f-(timeAnim-(endAnim+PROCESS_ANIM))/(PROCESS_ANIM));
		}

		/*if (letters[curstate].getIndex()!=-1)
		{
		updatePose(letters[curstate].getIndex(), (timeAnim-(endAnim+PROCESS_ANIM))/(PROCESS_ANIM));
		}*/
		timeAnim+=tm;

		if (timeAnim>endAnim+2*PROCESS_ANIM)
		{
			if (letters[curstate-1].getIndex()!=-1)
			{
			updatePose(letters[curstate-1].getIndex(), 0);
			}
			mAnimate=false;
			timeAnim=0;
			curstate=0;
		}
	}
	}
	catch(...)
	{

	}
	//LogManager::getSingleton().logMessage("[FacialAnimation] Ok step.");
}