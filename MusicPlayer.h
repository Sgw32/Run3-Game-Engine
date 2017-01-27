#pragma once
#include <audiere.h>
#include <Ogre.h>

using namespace Ogre;
using namespace std;
//using namespace audiere;

class MusicPlayer: public Singleton<MusicPlayer>
{
public:
	MusicPlayer();
	~MusicPlayer();
	void init();
	void playMusic(String filename, bool loop);
	void playMusicOverride(String filename,bool loop);
	void playSound(String filename, bool loop);
	void setMusicPitch(Real pitch);
	void setFadeOutLevel(Real fadel)
	{
		fade_level = fadel;
	}
	void toggleMusic(String filename, bool loop);
	void setVolume(Real volume)
	{
		stream->setVolume(volume);
	}

	Real getPosition() // 0 to 1
	{
		if (stream->getLength())
		{
			//LogManager::getSingleton().logMessage(StringConverter::toString(((float)stream->getPosition())/((float)stream->getLength())));
			return ((float)stream->getPosition())/((float)stream->getLength());
		}
		else
		{
			//LogManager::getSingleton().logMessage("ZERO!!!!");
			return 0;
		}
	}
	void upd();
	void upd(const FrameEvent &evt);
	void activatePReg()
	{
		preg=true;
	}
	void deactivatePReg()
	{
		preg=false;
	}
	void modulatePRegToValue(Real volume)
	{
		modVol=volume;
	}
	void cleanup();
private:
	bool smooth_transition;
	bool preg;
	String cfn;
	bool cloop;
	bool fadesound;
	Real fade_att;
	Real fade_level;
	Real modVol;
	audiere::AudioDevicePtr device;
	audiere::OutputStreamPtr stream;
	map<audiere::OutputStreamPtr,String> asounds;
};



