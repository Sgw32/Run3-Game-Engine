/*
Sound in Ogre3d head by sgw32
*/

#define	TEST_FILE		"2.wav"
#include"Framework.h"

class Run3SoundManager
{
  	virtual void PlaySoundStatic(void)
	{
    ALuint      uiBuffer;
	ALuint      uiSource;  
	ALint       iState;

	// Initialize Framework
	ALFWInit();

	if (!ALFWInitOpenAL())
	{
		ALFWShutdown();

	}

	// Generate an AL Buffer
	alGenBuffers( 1, &uiBuffer );

    ALFWLoadWaveToBuffer((char*)TEST_FILE, uiBuffer);

	// Load Wave file into OpenAL Buffer
	// Generate a Source to playback the Buffer
    alGenSources( 1, &uiSource );

	// Attach Source to Buffer
	alSourcei( uiSource, AL_BUFFER, uiBuffer );

	// Play Source
    alSourcePlay( uiSource );
	do
	{
		Sleep(100);

		// Get Source State
		alGetSourcei( uiSource, AL_SOURCE_STATE, &iState);
	} while (iState == AL_PLAYING);

    alSourceStop(uiSource);
    alDeleteSources(1, &uiSource);
	alDeleteBuffers(1, &uiBuffer);

	ALFWShutdownOpenAL();

	ALFWShutdown();


	} 
};
