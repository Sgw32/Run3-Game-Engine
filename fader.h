#pragma once


namespace Ogre {
	class TextureUnitState;
	class Overlay;
}

class FaderCallback
{
public:
	virtual void fadeInCallback(void) {}
	virtual void fadeOutCallback(void) {}
};

class Fader
{
public:
	Fader(const char *OverlayName, const char *MaterialName, FaderCallback *instance = 0);
	~Fader(void);

	void startFadeIn(double duration = 1.0f);
	void startFadeOut(double duration = 1.0f);
	void fade(double timeSinceLastFrame);

protected:
	double _alpha;
	double _current_dur;
	double _total_dur;
	FaderCallback *_inst;
	Ogre::TextureUnitState *_tex_unit;
	Ogre::Overlay *_overlay;

	enum _fadeop {
		FADE_NONE,
		FADE_IN,
		FADE_OUT,
	} _fadeop;
};