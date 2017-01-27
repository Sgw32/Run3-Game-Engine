#include "MusicPlayer.h"

template<> MusicPlayer *Singleton<MusicPlayer>::ms_Singleton=0;

MusicPlayer::MusicPlayer()
{
	smooth_transition=true;
	fadesound=false;
	fade_level=0;
	modVol=0;
	preg=false;
}

MusicPlayer::~MusicPlayer()
{
}

void MusicPlayer::init()
{
	LogManager::getSingleton().logMessage("MusicPlayer: loading config");
	ConfigFile cf;
	cf.load("run3/core/music.cfg");
	smooth_transition=StringConverter::parseBool(cf.getSetting("smooth","","false"));
	fade_att=StringConverter::parseReal(cf.getSetting("fade_att","","0.1"));
	device = audiere::AudioDevicePtr(audiere::OpenDevice());
	if (!device) 
  {
		
	 /* std::cout << "ERROR!";
	  std::cin >> i;*/
	  LogManager::getSingleton().logMessage("Audiere: Couldn't open device");
  }
	else
		 LogManager::getSingleton().logMessage("Audiere: device opened OK!");
  stream=0;
}

void MusicPlayer::toggleMusic(String filename, bool loop)
{


	if (device)
	{
		LogManager::getSingleton().logMessage("Music player: Playing "+filename);
		if (stream) 
		{
			stream->stop();
			stream->unref();
			stream=0;
			return;
		}

		stream=audiere::OutputStreamPtr(OpenSound(device, filename.c_str(), true));
		if (!stream) 
		{
			LogManager::getSingleton().logMessage("Audiere: Couldn't alloc audio source "+filename);
				// failure
				return;
		}
		else
		{
			//Great, we have some opened streams!  What do we do with them?

			// let's start the background music first
			stream->setRepeat(loop);
			stream->setVolume(1.5f); // 50% volume
			//stream->
			stream->play();
		}

	}
}

void MusicPlayer::setMusicPitch(Real pitch)
{
	if (stream)
	stream->setPitchShift(pitch);
}

void MusicPlayer::playMusicOverride(String filename,bool loop)
{
	if (filename.empty())
	{
		if (stream) 
		{
			stream->stop();
			stream=0;
		}
		return;
	}
	if (device)
	{
		LogManager::getSingleton().logMessage("Music player: Playing "+filename);
		if (stream) 
		{
			stream->stop();
			stream=0;
			//stream->unref();
			//stream=0;
		}

		stream=audiere::OutputStreamPtr(OpenSound(device, filename.c_str(), true));
		if (!stream) 
		{
			LogManager::getSingleton().logMessage("Audiere: Couldn't alloc audio source "+filename);
				// failure
				return;
		}
		else
		{
			//Great, we have some opened streams!  What do we do with them?

			// let's start the background music first
			stream->setRepeat(loop);
			stream->setVolume(1.5f); // 50% volume
			//stream->
			stream->play();
		}


	}
}

void MusicPlayer::playMusic(String filename,bool loop)
{
	if (device)
	{
		LogManager::getSingleton().logMessage("Music player: Playing "+filename);
		if (stream) 
		{
			if (!smooth_transition)
			{
			stream->stop();
			stream->unref();
			stream=0;
			}
			else
			{
				cloop=loop;
				cfn=filename;
				fadesound=true;
				return;
			}
		}

		stream=audiere::OutputStreamPtr(OpenSound(device, filename.c_str(), true));
		if (!stream) 
		{
			LogManager::getSingleton().logMessage("Audiere: Couldn't alloc audio source "+filename);
				// failure
				return;
		}
		else
		{
			//Great, we have some opened streams!  What do we do with them?

			// let's start the background music first
			stream->setRepeat(loop);
			stream->setVolume(1.5f); // 50% volume
			//stream->
			stream->play();
		}


	}
}

void MusicPlayer::upd()
{
	if (device&&stream)
	{
		//LogManager::getSingleton().logMessage("UPD");
		if (stream->isPlaying())
		{
			if (fadesound)
			{
				if ((stream->getVolume()-fade_att)<fade_level)
				{
					playMusicOverride(cfn,cloop);
					fadesound=false;
					return;
				}
				stream->setVolume(stream->getVolume()-fade_att);
				
			}
		//LogManager::getSingleton().logMessage("UPD");
		device->update();
		}
	}
}

void MusicPlayer::upd(const FrameEvent &evt)
{
	if (device&&stream)
	{
		//LogManager::getSingleton().logMessage("UPD");
		if (stream->isPlaying())
		{
			if (fadesound)
			{
				if ((stream->getVolume()-fade_att*evt.timeSinceLastFrame)<fade_level)
				{
					playMusicOverride(cfn,cloop);
					fadesound=false;
					return;
				}
				stream->setVolume(stream->getVolume()-fade_att*evt.timeSinceLastFrame);
				
			}
			if (preg)
			{
				stream->setVolume(stream->getVolume()-(stream->getVolume()-modVol)*evt.timeSinceLastFrame);
			}
		//LogManager::getSingleton().logMessage("UPD");
		device->update();
		}
	}
}

void MusicPlayer::cleanup()
{

	if (stream) 
	{
		modVol=1.0f;
		preg=false;
		fadesound=false;
		LogManager::getSingleton().logMessage("Music player: Unref");
		stream->stop();
		stream=0;
		LogManager::getSingleton().logMessage("Music player: Unref Ready");
	}
}