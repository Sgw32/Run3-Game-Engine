/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "Run3SoundRuntime.h"

template<> Run3SoundRuntime *Singleton<Run3SoundRuntime>::ms_Singleton=0;

Run3SoundRuntime::Run3SoundRuntime()
{
	
}

Run3SoundRuntime::~Run3SoundRuntime()
{
}

void Run3SoundRuntime::init()
{
	//global::getSingleton().getRoot()->addFrameListener(this);
	sMgr=global::getSingleton().getSoundManager();
	ply=global::getSingleton().getPlayer();
}

void Run3SoundRuntime::emitSound(String sound, Real duration, bool loop)
{
	/*if (sound=="none")
		return;
	durations.push_back(duration);
	unsigned int s;
	sMgr->loadAudio(sound,&s,loop);
	sMgr->setSound(s,Vector3::ZERO,Vector3::ZERO,Vector3::ZERO,100000,false,false,100000,100000,100000);
	sMgr->playAudio(s,false);
	String msg = sound;
	msg+=StringConverter::toString(duration);
	LogManager::getSingleton().logMessage("Playing sound: file "+msg);
	sounds.push_back(s);
	dists.push_back(0);
	poss.push_back(Vector3(0,0,0));
	optim.push_back(false);
	fileNames.push_back(sound);*/
	if (durations.size()>MAX_AUDIO_SOURCES-1)
		return;
	if (sound=="none")
		return;
	durations.push_back(duration);
	unsigned int s;
	sMgr->loadAudio(sound,&s,loop);
	sMgr->setSound(s,global::getSingleton().getPlayer()->get_location(),Vector3(1,1,1),Vector3(1,1,1),60000.0f,false,false,60000.0f,1,30000.0f);
	sMgr->playAudio(s,false);
	String msg = sound;
	msg+=StringConverter::toString(duration);
	LogManager::getSingleton().logMessage("Playing sound: file "+msg);
	sounds.push_back(s);
	dists.push_back(0);
	poss.push_back(Vector3(0,0,0));
	optim.push_back(false);
	fileNames.push_back(sound);
}

void Run3SoundRuntime::emitSound(String sound, Real duration, bool loop,Vector3 pos,Real dist,Real fallofDist)
{
	if (durations.size()>MAX_AUDIO_SOURCES-1)
		return;
	if (sound=="none")
		return;
	durations.push_back(duration);
	unsigned int s;
	sMgr->loadAudio(sound,&s,loop);
	sMgr->setSound(s,pos,Vector3::ZERO,Vector3::ZERO,dist,false,false,dist,dist,dist);
	sMgr->playAudio(s,false);
	String msg = sound;
	msg+=StringConverter::toString(duration);
	LogManager::getSingleton().logMessage("Playing sound: file "+msg);
	sounds.push_back(s);
	dists.push_back(0);
	poss.push_back(Vector3(0,0,0));
	optim.push_back(false);
	fileNames.push_back(sound);
}

void Run3SoundRuntime::emitSound(String sound, Real duration, bool loop,Vector3 pos,Real minGain,Real rolloff,Real fallofDist)
{
	if (durations.size()>MAX_AUDIO_SOURCES-1)
		return;
	if (sound=="none")
		return;
	durations.push_back(duration);
	unsigned int s;
	sMgr->loadAudio(sound,&s,loop);
	sMgr->setSound(s,pos,Vector3::ZERO,Vector3::ZERO,1000,false,false,minGain,rolloff,fallofDist);
	sMgr->playAudio(s,false);
	String msg = sound;
	msg+=StringConverter::toString(duration);
	LogManager::getSingleton().logMessage("Playing sound: file "+msg);
	sounds.push_back(s);
	dists.push_back(0);
	poss.push_back(Vector3(0,0,0));
	optim.push_back(false);
	fileNames.push_back(sound);
}

void Run3SoundRuntime::emitSoundOp(String sound, Real duration, bool loop,Vector3 pos,Real dist,Real fallofDist)
{
	if (durations.size()>MAX_AUDIO_SOURCES-1)
		return;
	if (sound=="none")
		return;
	durations.push_back(duration);
	/*unsigned int s;
	sMgr->loadAudio(sound,&s,loop);
	sMgr->setSound(s,pos,Vector3::ZERO,Vector3::ZERO,dist,false,false,dist,dist,dist);
	sMgr->playAudio(s,false);*/
	//String msg = sound;
	//msg+=StringConverter::toString(duration);
	//LogManager::getSingleton().logMessage("Playing sound: file "+msg);
	sounds.push_back(0);
	dists.push_back(dist);
	poss.push_back(pos);
	optim.push_back(true);
	fileNames.push_back(sound);
}	

void Run3SoundRuntime::emitCollideSound(int type, Vector3 pos)
{

}

void Run3SoundRuntime::addAmbientSound(String sound, Vector3 pos, Real dist, Real fallofDist)
{
	LogManager::getSingleton().logMessage("adding AmbientSound1 to environment...");
	AmbientSound s = AmbientSound(sMgr,sound,pos,dist,fallofDist);
	asounds.push_back(s);
}

void Run3SoundRuntime::addAmbientSound(String name,String sound, Vector3 pos, Real dist, Real fallofDist)
{
	LogManager::getSingleton().logMessage("adding AmbientSound2 to environment...");
	AmbientSound s = AmbientSound(sMgr,sound,pos,dist,fallofDist);
	s.setName(name);
	asounds.push_back(s);
}

void Run3SoundRuntime::addAmbientSound(String name,String sound, Vector3 pos, Real dist, Real fallofDist,Real gain)
{
	LogManager::getSingleton().logMessage("adding AmbientSound3 to environment...");
	AmbientSound s = AmbientSound(sMgr,sound,pos,dist,fallofDist,gain);
	s.setName(name);
	asounds.push_back(s);
}

void Run3SoundRuntime::disableAmbientSound(String name)
{
	LogManager::getSingleton().logMessage("Disabling AmbientSound ...");
	
	vector<AmbientSound>::iterator it;
	for (it=asounds.begin();it!=asounds.end();it++)
	{
			if ((*it).getName()==name)
			{
				LogManager::getSingleton().logMessage("Found.");
				(*it).disableAmbientSound();
			}
	}
}

void Run3SoundRuntime::enableAmbientSound(String name)
{
	LogManager::getSingleton().logMessage("Enabling AmbientSound ...");
	
	vector<AmbientSound>::iterator it;
	for (it=asounds.begin();it!=asounds.end();it++)
	{
			if ((*it).getName()==name)
			{
				LogManager::getSingleton().logMessage("Found.");
				(*it).enableAmbientSound();
			}
	}
}


void Run3SoundRuntime::clearAmbientSounds()
{
	vector<AmbientSound>::iterator it;
	for (it=asounds.begin();it!=asounds.end();it++)
	{
			(*it).destroy();
	}
	asounds.clear();

	durations.clear(); 
	sounds.clear();
	dists.clear();
	poss.clear();
	optim.clear();
	fileNames.clear();
}

//Kostil
void Run3SoundRuntime::recheckAmbientSounds()
{
	vector<AmbientSound>::iterator it;
	for (it=asounds.begin();it!=asounds.end();it++)
	{
		(*it).destroy();
	}
}

void Run3SoundRuntime::setCheckAmbientSounds()
{
	check=3.0f;
}

bool Run3SoundRuntime::frameStarted(const Ogre::FrameEvent &evt)
{
	if (check)
	{
		check-=evt.timeSinceLastFrame;
		if (check<0)
		{
			recheckAmbientSounds();
			check=0;
		}
	}
	vector<AmbientSound>::iterator it;
	for (it=asounds.begin();it!=asounds.end();it++)
	{
		if ((*it).getAvailability())
			(*it).spawn();
		else
			(*it).destroy();
	}

	for(unsigned int i2=0;i2!=durations.size();i2++)
	{
		if (durations[i2]>0)
		{
			if (optim[i2])
			{
				Real l = (ply->get_location()-poss[i2]).length();
				if (l>dists[i2]&&sounds[i2]!=0)
				{
					sMgr->releaseAudio(sounds[i2]);
					sounds[i2]=0;
				}
				if (l<dists[i2]&&sounds[i2]==0)
				{
					unsigned int s;
					sMgr->loadAudio(fileNames[i2],&s,true);
					sMgr->setSound(s,poss[i2],Vector3::ZERO,Vector3::ZERO,dists[i2],false,false,dists[i2],dists[i2],dists[i2]);
					sMgr->playAudio(s,false);
					sounds[i2]=s;
				}
			}
			durations[i2]-=evt.timeSinceLastFrame;
		}
		if (durations[i2]<=0)
		{
			durations[i2]=0;
			if (sounds[i2]!=256)
			{
				sMgr->releaseAudio(sounds[i2]);
				sounds[i2]=256;
			}
		}
	}
	//vector<unsigned int>::iterator i;
	unsigned int i;
	vector<Real>::iterator j;
	vector<Real>::iterator l;
	vector<Vector3>::iterator k;
	vector<String>::iterator m;
	vector<bool>::iterator o;
	vector<unsigned int>::iterator p;
	unsigned int s=0;
	
	//part to just-in-time delete sounds from list - "Sgw32 Deleter"
	bool stop=true;
//#define DELETE_ITER_SOUND(it,v) it=v.begin();advance(it,i);v.erase(it);	

				/*while (stop)
				{
					stop=false;
					i=0;
					for (j=durations.begin();j!=durations.end();j++)
					{
						if (!stop)
						{
						if ((*j)==0.0f)
						{
							durations.erase(j);

							//DELETE_ITER_SOUND(l,durations)
							DELETE_ITER_SOUND(l,dists)
							DELETE_ITER_SOUND(p,sounds)
							DELETE_ITER_SOUND(k,poss)
							DELETE_ITER_SOUND(o,optim)
							DELETE_ITER_SOUND(m,fileNames)

							stop=true;
							
						}	
						i++;
						if (stop)
							break;
						}
					
					
					}
				}*/
#define DELETE_ITER_SOUND(it,v) v.erase(it);	

	l = dists.begin();
	p = sounds.begin();
	k = poss.begin();
	o = optim.begin();
	m = fileNames.begin();

	for (j=durations.begin();j!=durations.end();)
	{
		if ((*j)==0.0f)
		{
			durations.erase(j);
			DELETE_ITER_SOUND(l,dists)
			DELETE_ITER_SOUND(p,sounds)
			DELETE_ITER_SOUND(k,poss)
			DELETE_ITER_SOUND(o,optim)
			DELETE_ITER_SOUND(m,fileNames)
			continue;
		}	
		j++;
		l++;
		p++;
		k++;
		o++;
		m++;
	}

//#undef DELETE_ITER_SOUND

	for(unsigned int m=0;m!=durations.size();m++)
	{
		Real d =durations[m];
		if (d>0)
			return true;
	}
	durations.clear(); 
	sounds.clear();
	dists.clear();
	poss.clear();
	optim.clear();
	fileNames.clear();
	return true;
}