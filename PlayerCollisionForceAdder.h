#include <OgreNewt.h>
#include <Ogre.h>
/*#include "Player.h"
#include "global.h"*/

class PlayerCollisionForceAdder: public OgreNewt::ContactCallback
{
public:
   PlayerCollisionForceAdder();
   ~PlayerCollisionForceAdder();

   int userBegin();

   int userProcess();

   void userEnd();

private:
	OgreNewt::Body* mBody01;
	OgreNewt::Body* mBody02;
};