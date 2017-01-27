/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2015 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#include "FuzzyTest.h"

FuzzyTest2::FuzzyTest2()
{
	reg_type = FUZZY_CONTROL;
	motor_cnt=0;
	pidpitch=0;
	pidroll=0;
	crisp1=0;
	crisp2=0;
	crisp3=0;
	crisp4=0;
	mNamedPipe=0;
}


FuzzyTest2::~FuzzyTest2()
{
}

void FuzzyTest2::setup(Ogre::SceneNode* door_node,bool box)
{
	LogManager::getSingleton().logMessage("AEROXO OBJECT: NOT SUPPORTED");
}

void FuzzyTest2::setup(Ogre::SceneNode* node)
{
	OgreNewt::World* mWorld = global::getSingleton().getWorld();
	door_node = node;
	Entity* ent;
	String name = node->getName();
	if (!(scene->hasEntity(name)))
		ent=scene->createEntity( name, "era51.mesh" );
	else
	{
		String name = node->getName();
		while (scene->hasEntity(name))
			name=name+"1";
		ent=scene->createEntity(name, "era51.mesh" );
	}
	
	Vector3 scale = node->getScale();
	node->attachObject(ent);
	//node->setScale(scale);
	AxisAlignedBox aab=ent->getBoundingBox();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*node->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));
	//Rigid body
	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size);
	door_bod = new OgreNewt::Body( mWorld, col );
	door_bod->attachToNode( node );
	// initial position
	door_bod->setPositionOrientation( node->getPosition()-(center*scale), node->getOrientation() );
	delete col;
	//Make it movable!
	Real mass = 10.0f;
	Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, size);
	door_bod->setMassMatrix( mass, inertia );
	door_bod->setName(name);
	door_bod->setCustomForceAndTorqueCallback<FuzzyTest2>( &FuzzyTest2::force_callback ,this);
	door_bod->setAutoFreeze(0);


	OgreNewt::Joint* joint;
	joint = new OgreNewt::BasicJoints::BallAndSocket( mWorld, door_bod, NULL, node->_getDerivedPosition() );
	//OgreNewt::BasicJoints::UpVector* hor = new OgreNewt::BasicJoints::UpVector(mWorld,door_bod,Vector3::UNIT_X);
	
	setupPID();
	fuzzySetup();
	//buildRules();
	//mCpt = new copter();
	
}

void FuzzyTest2::parseMotor(Entity* ent, Vector3 pos, Vector3 refpos,Vector3 scale, Real pscale, Quaternion rot)
{
	if (motor_cnt==4)
		return;
	OgreNewt::World* mWorld = global::getSingleton().getWorld();

	SceneNode* nod = scene->getRootSceneNode()->createChildSceneNode(ent->getName(),refpos+pos*pscale,rot);
	nod->attachObject(ent);
	//nod->setPosition(pos*pscale);
	nod->setScale(scale*pscale);

	OgreNewt::Body* bod = addMotorGroup(nod,ent,refpos+pos*pscale,scale,pscale,rot);
	
	
	
	
	motor_cnt++;
	if (motor_cnt==1)
	{
		bod->setCustomForceAndTorqueCallback<FuzzyTest2>( &FuzzyTest2::motor1_callback ,this);
	}
	if (motor_cnt==2)
	{
		bod->setCustomForceAndTorqueCallback<FuzzyTest2>( &FuzzyTest2::motor2_callback ,this);
	}
	if (motor_cnt==3)
	{
		bod->setCustomForceAndTorqueCallback<FuzzyTest2>( &FuzzyTest2::motor3_callback ,this);
	}
	if (motor_cnt==4)
	{
		bod->setCustomForceAndTorqueCallback<FuzzyTest2>( &FuzzyTest2::motor4_callback ,this);
	}
	bod->setAutoFreeze(0);
	OgreNewt::Joint* joint = new OgreNewt::BasicJoints::BallAndSocket( mWorld, bod, door_bod, refpos+pos*pscale);
	OgreNewt::BasicJoints::UpVector* hor = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Y);
	hors.push_back(hor);
	hor = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_X);
	hors2.push_back(hor);
	hor = new OgreNewt::BasicJoints::UpVector(mWorld,bod,Vector3::UNIT_Z);
	hors3.push_back(hor);
}


OgreNewt::Body* FuzzyTest2::addMotorGroup(Ogre::SceneNode* node, Entity* ent, Vector3 pos, Vector3 scale, Real pscale, Quaternion rot)
{
	OgreNewt::World* mWorld = global::getSingleton().getWorld();


	AxisAlignedBox aab=ent->getBoundingBox();
	Ogre::Vector3 min = aab.getMinimum()*scale;
	Ogre::Vector3 max = aab.getMaximum()*scale;
	Ogre::Vector3 center = aab.getCenter()*node->getScale();
	Ogre::Vector3 size = Vector3(fabs(max.x-min.x),fabs(max.y-min.y),fabs(max.z-min.z));

	OgreNewt::Collision* col = new OgreNewt::CollisionPrimitives::Box(mWorld, size);
	OgreNewt::Body* bod = new OgreNewt::Body( mWorld, col );
	bod->attachToNode( node );
	// initial position
	bod->setPositionOrientation( node->getPosition(), node->getOrientation() );
	delete col;

	Real mass = 1.0f;
	Vector3 inertia = OgreNewt::MomentOfInertia::CalcBoxSolid( mass, size);
	bod->setMassMatrix( mass, inertia );
	//bod->setName(name);
	return bod;
}

void FuzzyTest2::force_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   //Ogre::Vector3 force(0,gravity,0); 
   Ogre::Vector3 force(0,0,0); 
   force *= mass; 
	Quaternion quat = get_orientation();
	//Quaternion mPitchRot = Quaternion(Degree(mCpt->getPitch()),Vector3::UNIT_X);
	//set_orientation(mPitchRot);
	Real pitch = quat.getPitch().valueDegrees();
	Real roll = quat.getRoll().valueDegrees();
	while(roll>90.0f)
		roll-=180.0f;
	while(pitch>90.0f)
		pitch-=180.0f;
	//mCpt->setPitch(pitch);
   me->addForce(force);

   vector<OgreNewt::BasicJoints::UpVector*>::iterator i;
	pidpitch=getPIDPitch();
	pidroll=getPIDRoll();
   for (i=hors.begin();i!=hors.end();i++)
   {
	   (*i)->setPin(quat*Vector3::UNIT_Y);
   }
   for (i=hors2.begin();i!=hors2.end();i++)
   {
	   (*i)->setPin(quat*Vector3::UNIT_X);
   }
   for (i=hors3.begin();i!=hors3.end();i++)
   {
	   (*i)->setPin(quat*Vector3::UNIT_Z);
   }

	if (reg_type==FUZZY_CONTROL)
   {
   fuzzy->setInput(1, 1-pitch/45);
   fuzzy->setInput(2, 1-roll/45);
   fuzzy->fuzzify();
   fuzzy2->setInput(1, 1-pitch/45);
   fuzzy2->setInput(2, 1-roll/45);
   fuzzy2->fuzzify();
   fuzzy3->setInput(1, 1-pitch/45);
   fuzzy3->setInput(2, 1-roll/45);
   fuzzy3->fuzzify();
   fuzzy4->setInput(1, 1-pitch/45);
   fuzzy4->setInput(2, 1-roll/45);
   fuzzy4->fuzzify();

   crisp1 = fuzzy->defuzzify(1);
   crisp2 = fuzzy2->defuzzify(1);
   crisp3 = fuzzy3->defuzzify(1);
   crisp4 = fuzzy4->defuzzify(1);

	   //LogManager::getSingleton().logMessage("Pitch:"+StringConverter::toString(pitch));
		//LogManager::getSingleton().logMessage("Roll:"+StringConverter::toString(roll));
		//LogManager::getSingleton().logMessage("Crisp1:"+StringConverter::toString(crisp1));
		//LogManager::getSingleton().logMessage("Crisp2:"+StringConverter::toString(crisp2));
		//LogManager::getSingleton().logMessage("Crisp3:"+StringConverter::toString(crisp3));
		//LogManager::getSingleton().logMessage("Crisp4:"+StringConverter::toString(crisp4));
   }

   if (reg_type==FUZZY_CONTROL2)
   {
	   if (mNamedPipe)
	   {

			mNamedPipe->setCurrentPitchAndRoll(-pitch,-roll);
			mNamedPipe->upd(0.1);
	   }		
			//ffll_set_value(model, child, 0, pitch/90*1000); 
           // ffll_set_value(model, child, 1, roll/90*1000);
			//crisp1=
   }

   if (mNamedPipe)
   {
   myLog << currentDateTime().c_str() <<" ANGLES "<<StringConverter::toString(pitch).c_str()<<" "<<StringConverter::toString(roll).c_str()
	   <<"	OUTF	"
	   <<StringConverter::toString(mNamedPipe->getMotor1()).c_str()<<"	"
	   <<StringConverter::toString(mNamedPipe->getMotor2()).c_str()<<"	"
	   <<StringConverter::toString(mNamedPipe->getMotor3()).c_str()<<"	"
	   <<StringConverter::toString(mNamedPipe->getMotor4()).c_str()
	   <<"	PID	"
	   <<StringConverter::toString(pidpitch).c_str()<<"	"
	   <<StringConverter::toString(pidroll).c_str()<<"	" << "\n";
   }
   else
   {
		myLog << currentDateTime().c_str() <<" ANGLES "<<StringConverter::toString(pitch).c_str()<<" "<<StringConverter::toString(roll).c_str()
	   <<"	OUTF	"
	   <<"	PID	"
	   <<StringConverter::toString(pidpitch).c_str()<<"	"
	   <<StringConverter::toString(pidroll).c_str()<<"	" << "\n";
   }
	//me->setOmega(Vector3(mCpt->getPitchOmega()/100));
}

void FuzzyTest2::motor1_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Real timesh  = Timeshift::getSingleton().getTimeK();
   me->getMassMatrix(mass, inertia); 
   //Ogre::Vector3 force(0,gravity,0); 
   Ogre::Vector3 force = Vector3::ZERO;
   if (reg_type==PID_CONTROL)
   {
   force = Ogre::Vector3(0,100000-pidpitch*900000-pidroll*900000,0); 
   }
   if (reg_type==FUZZY_CONTROL)
   {
   force = Ogre::Vector3(0,100000+crisp1*90000,0); 
   }
	if (reg_type==FUZZY_CONTROL2)
   {
	   if (mNamedPipe)
   force = Ogre::Vector3(0,100000.0f+mNamedPipe->getMotor1()*900000.0f,0); 
   }
   force *= mass; 
    LogManager::getSingleton().logMessage("Motor1 up:"+StringConverter::toString(force));
	Quaternion quat = get_orientation();
	
   me->addForce(force*timesh);
}

void FuzzyTest2::motor2_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Real timesh  = Timeshift::getSingleton().getTimeK();
   me->getMassMatrix(mass, inertia); 
   //Ogre::Vector3 force(0,gravity,0); 
    Ogre::Vector3 force = Vector3::ZERO;
   if (reg_type==PID_CONTROL)
   {
   force = Ogre::Vector3(0,100000-pidpitch*900000+pidroll*900000,0); 
   }
   if (reg_type==FUZZY_CONTROL)
   {
   force = Ogre::Vector3(0,100000+crisp2*90000,0); 
   }
   if (reg_type==FUZZY_CONTROL2)
   {
	    if (mNamedPipe)
   force = Ogre::Vector3(0,100000.0f+mNamedPipe->getMotor2()*900000.0f,0); 
   }
   force *= mass; 
    LogManager::getSingleton().logMessage("Motor2 up:"+StringConverter::toString(force));
	Quaternion quat = get_orientation();
	
   me->addForce(force*timesh);
}

void FuzzyTest2::motor3_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Real timesh  = Timeshift::getSingleton().getTimeK();
   me->getMassMatrix(mass, inertia); 
   //Ogre::Vector3 force(0,gravity,0); 
    Ogre::Vector3 force = Vector3::ZERO;
   if (reg_type==PID_CONTROL)
   {
	force =  Ogre::Vector3 (0,100000+pidpitch*900000+pidroll*900000,0); 
   }
    if (reg_type==FUZZY_CONTROL)
   {
   force = Ogre::Vector3(0,100000+crisp3*90000,0); 
   }
   if (reg_type==FUZZY_CONTROL2)
   {
	    if (mNamedPipe)
   force = Ogre::Vector3(0,100000.0f+mNamedPipe->getMotor3()*900000.0f,0); 
   }
   force *= mass; 
    LogManager::getSingleton().logMessage("Motor3 up:"+StringConverter::toString(force));
	Quaternion quat = get_orientation();
	
   me->addForce(force*timesh);
}

void FuzzyTest2::motor4_callback( OgreNewt::Body* me ) 
{ 
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   Real timesh  = Timeshift::getSingleton().getTimeK();
   me->getMassMatrix(mass, inertia); 
   //Ogre::Vector3 force(0,gravity,0); 
   Ogre::Vector3 force = Vector3::ZERO;
   if (reg_type==PID_CONTROL)
   {
	   force=Ogre::Vector3(0,100000.0f+pidpitch*900000-pidroll*900000,0); 
   }
    if (reg_type==FUZZY_CONTROL)
   {
   force = Ogre::Vector3(0,100000.0f+crisp4*90000,0); 
   }
   if (reg_type==FUZZY_CONTROL2)
   {
	    if (mNamedPipe)
   force = Ogre::Vector3(0,100000.0f+mNamedPipe->getMotor4()*900000.0f,0); 
   }
   force *= mass; 
    LogManager::getSingleton().logMessage("Motor4 up:"+StringConverter::toString(force));
	Quaternion quat = get_orientation();
	
   me->addForce(force*timesh);
}

void FuzzyTest2::Fire(String event)
{
	
}

Vector3 FuzzyTest2::get_position()
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
return oldpos;
}

void FuzzyTest2::set_orientation(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat);
}

void FuzzyTest2::rotateBody(Quaternion quat)
{
	Vector3 oldpos;
	Quaternion quat2;
	door_bod->getPositionOrientation(oldpos,quat2);
	door_bod->setPositionOrientation(oldpos,quat*quat2);
}

void FuzzyTest2::setname(String name)
{
	mName=name;
}

String FuzzyTest2::getname()
{
	return mName;
}


void FuzzyTest2::unload()
{
LogManager::getSingleton().logMessage("AEROXO OBJECT: Disposing");
delete door_bod;
	   door_bod = 0;
	Entity* ent = (Entity*)door_node->getAttachedObject(0);
	door_node->detachAllObjects();
	LogManager::getSingleton().logMessage("AEROXO OBJECT: Disposing child");

	door_node->removeAndDestroyAllChildren();

	if (ent)
	global::getSingleton().getSceneManager()->destroyEntity(ent);
	   global::getSingleton().getSceneManager()->destroySceneNode(door_node);
	   LogManager::getSingleton().logMessage("AEROXO OBJECT: Ok.");
	  
}

void FuzzyTest2::set_position(Vector3 pos)
{
Vector3 oldpos;
Quaternion quat;
door_bod->getPositionOrientation(oldpos,quat);
door_bod->setPositionOrientation(pos,quat);
}

void FuzzyTest2::processEvent(int type)
{

}

void FuzzyTest2::fuzzySetup()
{
	   // myLog = FileLogger("1.0","fuzzy_test.txt");
		myLog << "Fuzzy object initialization.";
		fuzzy = new Fuzzy();
		fuzzy2 = new Fuzzy();
		fuzzy3 = new Fuzzy();
		fuzzy4 = new Fuzzy();

		pitch = new FuzzyInput(1);
		roll = new FuzzyInput(2);
		fMotor1 = new FuzzyOutput(1);
		fMotor2 = new FuzzyOutput(1);
		fMotor3 = new FuzzyOutput(1);
		fMotor4 = new FuzzyOutput(1);

		highminus = new FuzzySet(0, 0, 1.0f-0.7f, 1.0f-0.5f);
		mediumminus = new FuzzySet(1.0f-0.7f, 1.0f-0.5f, 1.0f-0.5f, 1.0f-0.05f);
		zero = new FuzzySet(1.0f-0.07f, 1.0f, 1.0f, 1.07f);
		mediumplus = new FuzzySet(1.05f, 1.5f, 1.5f, 1.7f);
		highplus = new FuzzySet(1.5, 1.7f, 2.0f, 2.0f);

		highminus2 = new FuzzySet(0, 0, 1.0f-0.7f, 1.0f-0.5f);
		mediumminus2 = new FuzzySet(1.0f-0.7f, 1.0f-0.5f, 1.0f-0.5f, 1.0f-0.05f);
		zero2 = new FuzzySet(1.0f-0.07f, 1.0f, 1.0f, 1.07f);
		mediumplus2 = new FuzzySet(1.05f, 1.5f, 1.5f, 1.7f);
		highplus2 = new FuzzySet(1.5, 1.7f, 2.0f, 2.0f);

		highminus3 = new FuzzySet(0, 0, 1.0f-0.7f, 1.0f-0.5f);
		mediumminus3 = new FuzzySet(1.0f-0.7f, 1.0f-0.5f, 1.0f-0.5f, 1.0f-0.05f);
		zero3 = new FuzzySet(1.0f-0.07f, 1.0f, 1.0f, 1.07f);
		mediumplus3 = new FuzzySet(1.05f, 1.5f, 1.5f, 1.7f);
		highplus3 = new FuzzySet(1.5, 1.7f, 2.0f, 2.0f);

		pitch->addFuzzySet(highminus);
		pitch->addFuzzySet(mediumminus);
		pitch->addFuzzySet(zero);
		pitch->addFuzzySet(mediumplus);
		pitch->addFuzzySet(highplus);

		roll->addFuzzySet(highminus2);
		roll->addFuzzySet(mediumminus2);
		roll->addFuzzySet(zero2);
		roll->addFuzzySet(mediumplus2);
		roll->addFuzzySet(highplus2);

		fMotor1->addFuzzySet(highminus3);
		fMotor1->addFuzzySet(mediumminus3);
		fMotor1->addFuzzySet(zero3);
		fMotor1->addFuzzySet(mediumplus3);
		fMotor1->addFuzzySet(highplus3);

		fMotor2->addFuzzySet(highminus3);
		fMotor2->addFuzzySet(mediumminus3);
		fMotor2->addFuzzySet(zero3);
		fMotor2->addFuzzySet(mediumplus3);
		fMotor2->addFuzzySet(highplus3);

		fMotor3->addFuzzySet(highminus3);
		fMotor3->addFuzzySet(mediumminus3);
		fMotor3->addFuzzySet(zero3);
		fMotor3->addFuzzySet(mediumplus3);
		fMotor3->addFuzzySet(highplus3);

		fMotor4->addFuzzySet(highminus3);
		fMotor4->addFuzzySet(mediumminus3);
		fMotor4->addFuzzySet(zero3);
		fMotor4->addFuzzySet(mediumplus3);
		fMotor4->addFuzzySet(highplus3);

		fuzzy->addFuzzyInput(pitch);
		fuzzy->addFuzzyInput(roll);
		fuzzy->addFuzzyOutput(fMotor1);
		fuzzy2->addFuzzyInput(pitch);
		fuzzy2->addFuzzyInput(roll);
		fuzzy2->addFuzzyOutput(fMotor2);
		fuzzy3->addFuzzyInput(pitch);
		fuzzy3->addFuzzyInput(roll);
		fuzzy3->addFuzzyOutput(fMotor3);
		fuzzy4->addFuzzyInput(pitch);
		fuzzy4->addFuzzyInput(roll);
		fuzzy4->addFuzzyOutput(fMotor4);
		buildRules();

		/*model = ffll_new_model();  

		ret_val = (int)ffll_load_fcl_file(model, "run3/game/tiltrotor.fcl");  

		if (ret_val < 0)
			{
				LogManager::getSingleton().logMessage("Error Opening tiltrotor.fcl");
			// make sure the "working directory" in "Project | Settings..."
			// is set to the executable's directory if running from the MSVC IDE
			return 0;
			}

		// create a child for the model...
		child = ffll_new_child(model);
		ffll_set_value(model, child, 0, 0); 
        ffll_set_value(model, child, 0, 0);*/

		
		 
}

void FuzzyTest2::buildRules()
{
		//Левые части нечетких правил.
		FuzzyRuleAntecedent* pitchHighPlusRollHighPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumPlusRollHighPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchZeroRollHighPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumMinusRollHighPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchHighMinusRollHighPlus = new FuzzyRuleAntecedent();

		FuzzyRuleAntecedent* pitchHighPlusRollMediumPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumPlusRollMediumPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchZeroRollMediumPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumMinusRollMediumPlus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchHighMinusRollMediumPlus = new FuzzyRuleAntecedent();

		FuzzyRuleAntecedent* pitchHighPlusRollZero = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumPlusRollZero = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchZeroRollZero = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumMinusRollZero = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchHighMinusRollZero = new FuzzyRuleAntecedent();

		FuzzyRuleAntecedent* pitchHighPlusRollMediumMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumPlusRollMediumMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchZeroRollMediumMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumMinusRollMediumMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchHighMinusRollMediumMinus = new FuzzyRuleAntecedent();

		FuzzyRuleAntecedent* pitchHighPlusRollHighMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumPlusRollHighMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchZeroRollHighMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchMediumMinusRollHighMinus = new FuzzyRuleAntecedent();
		FuzzyRuleAntecedent* pitchHighMinusRollHighMinus = new FuzzyRuleAntecedent();

		//Правые части нечетких правил.
		FuzzyRuleConsequent* thenMotor1HighPlus = new FuzzyRuleConsequent();
		FuzzyRuleConsequent* thenMotor1MediumPlus = new FuzzyRuleConsequent();
		FuzzyRuleConsequent* thenMotor1Zero = new FuzzyRuleConsequent();
		FuzzyRuleConsequent* thenMotor1MediumMinus = new FuzzyRuleConsequent();
		FuzzyRuleConsequent* thenMotor1HighMinus = new FuzzyRuleConsequent();

		//Нечеткие правила
		//Первый двигатель. То есть передний правый.
		//Большой+ крен
		//Если тангаж большой+ и крен большой+ то двигатель 1 - большой-
		//Если тангаж средний+ и крен большой+ то двигатель 1 - большой-
		//Если тангаж нулевой и крен большой+ то двигатель 1 - средний-
		//Если тангаж средний- и крен большой+ то двигатель 1 - нулевой
		//Если тангаж большой- и крен большой+ то двигатель 1 - нулевой

		//Cредний+ крен
		//Если тангаж большой+ и крен средний+ то двигатель 1 - большой-
		//Если тангаж средний+ и крен средний+ то двигатель 1 - средний-
		//Если тангаж нулевой и крен средний+ то двигатель 1 - средний-
		//Если тангаж средний- и крен средний+ то двигатель 1 - нулевой
		//Если тангаж большой- и крен средний+ то двигатель 1 - нулевой

		//Нулевой крен
		//Если тангаж большой+ и крен нулевой то двигатель 1 - большой-
		//Если тангаж средний+ и крен нулевой то двигатель 1 - средний-
		//Если тангаж нулевой и крен нулевой то двигатель 1 - нулевой
		//Если тангаж средний- и крен нулевой то двигатель 1 - средний+
		//Если тангаж большой- и крен нулевой то двигатель 1 - большой+

		//Средний- крен
		//Если тангаж большой+ и крен средний- то двигатель 1 - средний-
		//Если тангаж средний+ и крен средний- то двигатель 1 - средний-
		//Если тангаж нулевой и крен средний- то двигатель 1 - средний+
		//Если тангаж средний- и крен средний- то двигатель 1 - средний+
		//Если тангаж большой- и крен средний- то двигатель 1 - большой+

		//Большой- крен
		//Если тангаж большой+ и крен большой- то двигатель 1 - средний+
		//Если тангаж средний+ и крен большой- то двигатель 1 - средний+
		//Если тангаж нулевой и крен большой- то двигатель 1 - большой+
		//Если тангаж средний- и крен большой- то двигатель 1 - большой+
		//Если тангаж большой- и крен большой- то двигатель 1 - большой+

		//Пропишем в левой части нечеткие правила с лог. "И"

		pitchHighPlusRollHighPlus->joinWithAND(highplus, highplus2);
		pitchMediumPlusRollHighPlus->joinWithAND(mediumplus, highplus2);
		pitchZeroRollHighPlus->joinWithAND(zero, highplus2);
		pitchMediumMinusRollHighPlus->joinWithAND(mediumminus, highplus2);
		pitchHighMinusRollHighPlus->joinWithAND(highminus, highplus2);

		pitchHighPlusRollMediumPlus->joinWithAND(highplus, mediumplus2);
		pitchMediumPlusRollMediumPlus->joinWithAND(mediumplus, mediumplus2);
		pitchZeroRollMediumPlus->joinWithAND(zero, mediumplus2);
		pitchMediumMinusRollMediumPlus->joinWithAND(mediumminus, mediumplus2);
		pitchHighMinusRollMediumPlus->joinWithAND(highminus, mediumplus2);

		pitchHighPlusRollZero->joinWithAND(highplus, zero2);
		pitchMediumPlusRollZero->joinWithAND(mediumplus, zero2);
		pitchZeroRollZero->joinWithAND(zero, zero2);
		pitchMediumMinusRollZero->joinWithAND(mediumminus, zero2);
		pitchHighMinusRollZero->joinWithAND(highminus, zero2);

		pitchHighPlusRollMediumMinus->joinWithAND(highplus, mediumminus2);
		pitchMediumPlusRollMediumMinus->joinWithAND(mediumplus, mediumminus2);
		pitchZeroRollMediumMinus->joinWithAND(zero, mediumminus2);
		pitchMediumMinusRollMediumMinus->joinWithAND(mediumminus, mediumminus2);
		pitchHighMinusRollMediumMinus->joinWithAND(highplus, mediumminus2);

		pitchHighPlusRollHighMinus->joinWithAND(highplus, highminus2);
		pitchMediumPlusRollHighMinus->joinWithAND(mediumplus, highminus2);
		pitchZeroRollHighMinus->joinWithAND(zero, highminus2);
		pitchMediumMinusRollHighMinus->joinWithAND(mediumminus, highminus2);
		pitchHighMinusRollHighMinus->joinWithAND(highminus, highminus2);

		//В левой части:

		thenMotor1HighPlus->addOutput(highplus3);
		thenMotor1MediumPlus->addOutput(mediumplus3);
		thenMotor1Zero->addOutput(zero3);
		thenMotor1MediumMinus->addOutput(mediumminus3);
		thenMotor1HighMinus->addOutput(highminus3);

		//Motor1
		FuzzyRule* rpitchHighPlusRollHighPlus = new FuzzyRule(1, pitchHighPlusRollHighPlus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumPlusRollHighPlus = new FuzzyRule(2, pitchMediumPlusRollHighPlus, thenMotor1HighMinus);
		FuzzyRule* rpitchZeroRollHighPlus = new FuzzyRule(3, pitchZeroRollHighPlus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumMinusRollHighPlus = new FuzzyRule(4, pitchMediumPlusRollHighPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollHighPlus = new FuzzyRule(5, pitchHighMinusRollHighPlus, thenMotor1MediumPlus);

		FuzzyRule* rpitchHighPlusRollMediumPlus = new FuzzyRule(6, pitchHighPlusRollMediumPlus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumPlusRollMediumPlus = new FuzzyRule(7, pitchMediumPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollMediumPlus = new FuzzyRule(8, pitchZeroRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchMediumMinusRollMediumPlus = new FuzzyRule(9, pitchMediumPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollMediumPlus = new FuzzyRule(10, pitchHighMinusRollMediumPlus, thenMotor1MediumPlus);

		FuzzyRule* rpitchHighPlusRollZero = new FuzzyRule(11, pitchHighPlusRollZero, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumPlusRollZero = new FuzzyRule(12, pitchMediumPlusRollZero, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollZero = new FuzzyRule(13, pitchZeroRollZero, thenMotor1Zero);
		FuzzyRule* rpitchMediumMinusRollZero = new FuzzyRule(14, pitchMediumPlusRollZero, thenMotor1MediumPlus);
		FuzzyRule* rpitchHighMinusRollZero = new FuzzyRule(15, pitchHighMinusRollZero, thenMotor1MediumPlus);

		FuzzyRule* rpitchHighPlusRollMediumMinus = new FuzzyRule(16, pitchHighPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchMediumPlusRollMediumMinus = new FuzzyRule(17, pitchMediumPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollMediumMinus = new FuzzyRule(18, pitchZeroRollMediumMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollMediumMinus = new FuzzyRule(19, pitchMediumPlusRollMediumMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchHighMinusRollMediumMinus = new FuzzyRule(20, pitchHighMinusRollMediumMinus, thenMotor1HighPlus);

		FuzzyRule* rpitchHighPlusRollHighMinus = new FuzzyRule(21, pitchHighPlusRollHighMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollHighMinus = new FuzzyRule(22, pitchMediumPlusRollHighMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchZeroRollHighMinus = new FuzzyRule(23, pitchZeroRollHighMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollHighMinus = new FuzzyRule(24, pitchMediumPlusRollHighMinus, thenMotor1HighPlus);
		FuzzyRule* rpitchHighMinusRollHighMinus = new FuzzyRule(25, pitchHighMinusRollHighMinus, thenMotor1HighPlus);
		//Motor2
		FuzzyRule* rpitchHighPlusRollHighPlus2 = new FuzzyRule(1, pitchHighPlusRollHighPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollHighPlus2 = new FuzzyRule(2, pitchMediumPlusRollHighPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchZeroRollHighPlus2 = new FuzzyRule(3, pitchZeroRollHighPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollHighPlus2 = new FuzzyRule(4, pitchMediumPlusRollHighPlus, thenMotor1HighPlus);
		FuzzyRule* rpitchHighMinusRollHighPlus2 = new FuzzyRule(5, pitchHighMinusRollHighPlus, thenMotor1HighPlus);

		FuzzyRule* rpitchHighPlusRollMediumPlus2 = new FuzzyRule(6, pitchHighPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchMediumPlusRollMediumPlus2 = new FuzzyRule(7, pitchMediumPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollMediumPlus2 = new FuzzyRule(8, pitchZeroRollMediumPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollMediumPlus2 = new FuzzyRule(9, pitchMediumPlusRollMediumPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchHighMinusRollMediumPlus2 = new FuzzyRule(10, pitchHighMinusRollMediumPlus, thenMotor1HighPlus);

		FuzzyRule* rpitchHighPlusRollZero2 = new FuzzyRule(11, pitchHighPlusRollZero, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumPlusRollZero2 = new FuzzyRule(12, pitchMediumPlusRollZero, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollZero2 = new FuzzyRule(13, pitchZeroRollZero, thenMotor1Zero);
		FuzzyRule* rpitchMediumMinusRollZero2 = new FuzzyRule(14, pitchMediumPlusRollZero, thenMotor1MediumPlus);
		FuzzyRule* rpitchHighMinusRollZero2 = new FuzzyRule(15, pitchHighMinusRollZero, thenMotor1MediumPlus);

		FuzzyRule* rpitchHighPlusRollMediumMinus2 = new FuzzyRule(16, pitchHighPlusRollMediumMinus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumPlusRollMediumMinus2 = new FuzzyRule(17, pitchMediumPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollMediumMinus2 = new FuzzyRule(18, pitchZeroRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchMediumMinusRollMediumMinus2 = new FuzzyRule(19, pitchMediumPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollMediumMinus2 = new FuzzyRule(20, pitchHighMinusRollMediumMinus, thenMotor1MediumPlus);

		FuzzyRule* rpitchHighPlusRollHighMinus2 = new FuzzyRule(21, pitchHighPlusRollHighMinus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumPlusRollHighMinus2 = new FuzzyRule(22, pitchMediumPlusRollHighMinus, thenMotor1HighMinus);
		FuzzyRule* rpitchZeroRollHighMinus2 = new FuzzyRule(23, pitchZeroRollHighMinus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumMinusRollHighMinus2 = new FuzzyRule(24, pitchMediumPlusRollHighMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollHighMinus2 = new FuzzyRule(25, pitchHighMinusRollHighMinus, thenMotor1MediumPlus);
		//Motor3
		FuzzyRule* rpitchHighPlusRollHighPlus3 = new FuzzyRule(1, pitchHighPlusRollHighPlus, thenMotor1HighPlus);
		FuzzyRule* rpitchMediumPlusRollHighPlus3 = new FuzzyRule(2, pitchMediumPlusRollHighPlus, thenMotor1HighPlus);
		FuzzyRule* rpitchZeroRollHighPlus3 = new FuzzyRule(3, pitchZeroRollHighPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollHighPlus3 = new FuzzyRule(4, pitchMediumPlusRollHighPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchHighMinusRollHighPlus3 = new FuzzyRule(5, pitchHighMinusRollHighPlus, thenMotor1MediumPlus);

		FuzzyRule* rpitchHighPlusRollMediumPlus3 = new FuzzyRule(6, pitchHighPlusRollMediumPlus, thenMotor1HighPlus);
		FuzzyRule* rpitchMediumPlusRollMediumPlus3 = new FuzzyRule(7, pitchMediumPlusRollMediumPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchZeroRollMediumPlus3 = new FuzzyRule(8, pitchZeroRollMediumPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollMediumPlus3 = new FuzzyRule(9, pitchMediumPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollMediumPlus3 = new FuzzyRule(10, pitchHighMinusRollMediumPlus, thenMotor1HighPlus);

		FuzzyRule* rpitchHighPlusRollZero3 = new FuzzyRule(11, pitchHighPlusRollZero, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollZero3 = new FuzzyRule(12, pitchMediumPlusRollZero, thenMotor1MediumPlus);
		FuzzyRule* rpitchZeroRollZero3 = new FuzzyRule(13, pitchZeroRollZero, thenMotor1Zero);
		FuzzyRule* rpitchMediumMinusRollZero3 = new FuzzyRule(14, pitchMediumPlusRollZero, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollZero3 = new FuzzyRule(15, pitchHighMinusRollZero, thenMotor1HighMinus);

		FuzzyRule* rpitchHighPlusRollMediumMinus3 = new FuzzyRule(16, pitchHighPlusRollMediumMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollMediumMinus3 = new FuzzyRule(17, pitchMediumPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollMediumMinus3 = new FuzzyRule(18, pitchZeroRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchMediumMinusRollMediumMinus3 = new FuzzyRule(19, pitchMediumPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollMediumMinus3 = new FuzzyRule(20, pitchHighMinusRollMediumMinus, thenMotor1HighMinus);

		FuzzyRule* rpitchHighPlusRollHighMinus3 = new FuzzyRule(21, pitchHighPlusRollHighMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollHighMinus3 = new FuzzyRule(22, pitchMediumPlusRollHighMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollHighMinus3 = new FuzzyRule(23, pitchZeroRollHighMinus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumMinusRollHighMinus3 = new FuzzyRule(24, pitchMediumPlusRollHighMinus, thenMotor1HighMinus);
		FuzzyRule* rpitchHighMinusRollHighMinus3 = new FuzzyRule(25, pitchHighMinusRollHighMinus, thenMotor1HighMinus);

		//Motor4
		FuzzyRule* rpitchHighPlusRollHighPlus4 = new FuzzyRule(1, pitchHighPlusRollHighPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollHighPlus4 = new FuzzyRule(2, pitchMediumPlusRollHighPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollHighPlus4 = new FuzzyRule(3, pitchZeroRollHighPlus, thenMotor1HighMinus);
		FuzzyRule* rpitchMediumMinusRollHighPlus4 = new FuzzyRule(4, pitchMediumPlusRollHighPlus, thenMotor1HighMinus);
		FuzzyRule* rpitchHighMinusRollHighPlus4 = new FuzzyRule(5, pitchHighMinusRollHighPlus, thenMotor1HighMinus);

		FuzzyRule* rpitchHighPlusRollMediumPlus4 = new FuzzyRule(6, pitchHighPlusRollMediumPlus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollMediumPlus4 = new FuzzyRule(7, pitchMediumPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchZeroRollMediumPlus4 = new FuzzyRule(8, pitchZeroRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchMediumMinusRollMediumPlus4 = new FuzzyRule(9, pitchMediumPlusRollMediumPlus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollMediumPlus4 = new FuzzyRule(10, pitchHighMinusRollMediumPlus, thenMotor1HighMinus);

		FuzzyRule* rpitchHighPlusRollZero4 = new FuzzyRule(11, pitchHighPlusRollZero, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumPlusRollZero4 = new FuzzyRule(12, pitchMediumPlusRollZero, thenMotor1MediumPlus);
		FuzzyRule* rpitchZeroRollZero4 = new FuzzyRule(13, pitchZeroRollZero, thenMotor1Zero);
		FuzzyRule* rpitchMediumMinusRollZero4 = new FuzzyRule(14, pitchMediumPlusRollZero, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollZero4 = new FuzzyRule(15, pitchHighMinusRollZero, thenMotor1HighMinus);

		FuzzyRule* rpitchHighPlusRollMediumMinus4 = new FuzzyRule(16, pitchHighPlusRollMediumMinus, thenMotor1HighPlus);
		FuzzyRule* rpitchMediumPlusRollMediumMinus4 = new FuzzyRule(17, pitchMediumPlusRollMediumMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchZeroRollMediumMinus4 = new FuzzyRule(18, pitchZeroRollMediumMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollMediumMinus4 = new FuzzyRule(19, pitchMediumPlusRollMediumMinus, thenMotor1MediumMinus);
		FuzzyRule* rpitchHighMinusRollMediumMinus4 = new FuzzyRule(20, pitchHighMinusRollMediumMinus, thenMotor1MediumMinus);

		FuzzyRule* rpitchHighPlusRollHighMinus4 = new FuzzyRule(21, pitchHighPlusRollHighMinus, thenMotor1HighPlus);
		FuzzyRule* rpitchMediumPlusRollHighMinus4 = new FuzzyRule(22, pitchMediumPlusRollHighMinus, thenMotor1HighPlus);
		FuzzyRule* rpitchZeroRollHighMinus4 = new FuzzyRule(23, pitchZeroRollHighMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchMediumMinusRollHighMinus4 = new FuzzyRule(24, pitchMediumPlusRollHighMinus, thenMotor1MediumPlus);
		FuzzyRule* rpitchHighMinusRollHighMinus4 = new FuzzyRule(25, pitchHighMinusRollHighMinus, thenMotor1MediumPlus);
		
		//Motor1
		fuzzy->addFuzzyRule(rpitchHighPlusRollHighPlus);
		fuzzy->addFuzzyRule(rpitchMediumPlusRollHighPlus);
		fuzzy->addFuzzyRule(rpitchZeroRollHighPlus);
		fuzzy->addFuzzyRule(rpitchMediumMinusRollHighPlus);
		fuzzy->addFuzzyRule(rpitchHighMinusRollHighPlus);

		fuzzy->addFuzzyRule(rpitchHighPlusRollMediumPlus);
		fuzzy->addFuzzyRule(rpitchMediumPlusRollMediumPlus);
		fuzzy->addFuzzyRule(rpitchZeroRollMediumPlus);
		fuzzy->addFuzzyRule(rpitchMediumMinusRollMediumPlus);
		fuzzy->addFuzzyRule(rpitchHighMinusRollMediumPlus);

		fuzzy->addFuzzyRule(rpitchHighPlusRollZero);
		fuzzy->addFuzzyRule(rpitchMediumPlusRollZero);
		fuzzy->addFuzzyRule(rpitchZeroRollZero);
		fuzzy->addFuzzyRule(rpitchMediumMinusRollZero);
		fuzzy->addFuzzyRule(rpitchHighMinusRollZero);

		fuzzy->addFuzzyRule(rpitchHighPlusRollMediumMinus);
		fuzzy->addFuzzyRule(rpitchMediumPlusRollMediumMinus);
		fuzzy->addFuzzyRule(rpitchZeroRollMediumMinus);
		fuzzy->addFuzzyRule(rpitchMediumMinusRollMediumMinus);
		fuzzy->addFuzzyRule(rpitchHighMinusRollMediumMinus);

		fuzzy->addFuzzyRule(rpitchHighPlusRollHighMinus);
		fuzzy->addFuzzyRule(rpitchMediumPlusRollHighMinus);
		fuzzy->addFuzzyRule(rpitchZeroRollHighMinus);
		fuzzy->addFuzzyRule(rpitchMediumMinusRollHighMinus);
		fuzzy->addFuzzyRule(rpitchHighMinusRollHighMinus);

		//Motor2
		fuzzy2->addFuzzyRule(rpitchHighPlusRollHighPlus2);
		fuzzy2->addFuzzyRule(rpitchMediumPlusRollHighPlus2);
		fuzzy2->addFuzzyRule(rpitchZeroRollHighPlus2);
		fuzzy2->addFuzzyRule(rpitchMediumMinusRollHighPlus2);
		fuzzy2->addFuzzyRule(rpitchHighMinusRollHighPlus2);

		fuzzy2->addFuzzyRule(rpitchHighPlusRollMediumPlus2);
		fuzzy2->addFuzzyRule(rpitchMediumPlusRollMediumPlus2);
		fuzzy2->addFuzzyRule(rpitchZeroRollMediumPlus2);
		fuzzy2->addFuzzyRule(rpitchMediumMinusRollMediumPlus2);
		fuzzy2->addFuzzyRule(rpitchHighMinusRollMediumPlus2);

		fuzzy2->addFuzzyRule(rpitchHighPlusRollZero2);
		fuzzy2->addFuzzyRule(rpitchMediumPlusRollZero2);
		fuzzy2->addFuzzyRule(rpitchZeroRollZero2);
		fuzzy2->addFuzzyRule(rpitchMediumMinusRollZero2);
		fuzzy2->addFuzzyRule(rpitchHighMinusRollZero2);

		fuzzy2->addFuzzyRule(rpitchHighPlusRollMediumMinus2);
		fuzzy2->addFuzzyRule(rpitchMediumPlusRollMediumMinus2);
		fuzzy2->addFuzzyRule(rpitchZeroRollMediumMinus2);
		fuzzy2->addFuzzyRule(rpitchMediumMinusRollMediumMinus2);
		fuzzy2->addFuzzyRule(rpitchHighMinusRollMediumMinus2);

		fuzzy2->addFuzzyRule(rpitchHighPlusRollHighMinus2);
		fuzzy2->addFuzzyRule(rpitchMediumPlusRollHighMinus2);
		fuzzy2->addFuzzyRule(rpitchZeroRollHighMinus2);
		fuzzy2->addFuzzyRule(rpitchMediumMinusRollHighMinus2);
		fuzzy2->addFuzzyRule(rpitchHighMinusRollHighMinus2);

		//Motor3

		fuzzy3->addFuzzyRule(rpitchHighPlusRollHighPlus3);
		fuzzy3->addFuzzyRule(rpitchMediumPlusRollHighPlus3);
		fuzzy3->addFuzzyRule(rpitchZeroRollHighPlus3);
		fuzzy3->addFuzzyRule(rpitchMediumMinusRollHighPlus3);
		fuzzy3->addFuzzyRule(rpitchHighMinusRollHighPlus3);

		fuzzy3->addFuzzyRule(rpitchHighPlusRollMediumPlus3);
		fuzzy3->addFuzzyRule(rpitchMediumPlusRollMediumPlus3);
		fuzzy3->addFuzzyRule(rpitchZeroRollMediumPlus3);
		fuzzy3->addFuzzyRule(rpitchMediumMinusRollMediumPlus3);
		fuzzy3->addFuzzyRule(rpitchHighMinusRollMediumPlus3);

		fuzzy3->addFuzzyRule(rpitchHighPlusRollZero3);
		fuzzy3->addFuzzyRule(rpitchMediumPlusRollZero3);
		fuzzy3->addFuzzyRule(rpitchZeroRollZero3);
		fuzzy3->addFuzzyRule(rpitchMediumMinusRollZero3);
		fuzzy3->addFuzzyRule(rpitchHighMinusRollZero3);

		fuzzy3->addFuzzyRule(rpitchHighPlusRollMediumMinus3);
		fuzzy3->addFuzzyRule(rpitchMediumPlusRollMediumMinus3);
		fuzzy3->addFuzzyRule(rpitchZeroRollMediumMinus3);
		fuzzy3->addFuzzyRule(rpitchMediumMinusRollMediumMinus3);
		fuzzy3->addFuzzyRule(rpitchHighMinusRollMediumMinus3);

		fuzzy3->addFuzzyRule(rpitchHighPlusRollHighMinus3);
		fuzzy3->addFuzzyRule(rpitchMediumPlusRollHighMinus3);
		fuzzy3->addFuzzyRule(rpitchZeroRollHighMinus3);
		fuzzy3->addFuzzyRule(rpitchMediumMinusRollHighMinus3);
		fuzzy3->addFuzzyRule(rpitchHighMinusRollHighMinus3);

		//Motor4

		fuzzy4->addFuzzyRule(rpitchHighPlusRollHighPlus4);
		fuzzy4->addFuzzyRule(rpitchMediumPlusRollHighPlus4);
		fuzzy4->addFuzzyRule(rpitchZeroRollHighPlus4);
		fuzzy4->addFuzzyRule(rpitchMediumMinusRollHighPlus4);
		fuzzy4->addFuzzyRule(rpitchHighMinusRollHighPlus4);

		fuzzy4->addFuzzyRule(rpitchHighPlusRollMediumPlus4);
		fuzzy4->addFuzzyRule(rpitchMediumPlusRollMediumPlus4);
		fuzzy4->addFuzzyRule(rpitchZeroRollMediumPlus4);
		fuzzy4->addFuzzyRule(rpitchMediumMinusRollMediumPlus4);
		fuzzy4->addFuzzyRule(rpitchHighMinusRollMediumPlus4);

		fuzzy4->addFuzzyRule(rpitchHighPlusRollZero4);
		fuzzy4->addFuzzyRule(rpitchMediumPlusRollZero4);
		fuzzy4->addFuzzyRule(rpitchZeroRollZero4);
		fuzzy4->addFuzzyRule(rpitchMediumMinusRollZero4);
		fuzzy4->addFuzzyRule(rpitchHighMinusRollZero4);

		fuzzy4->addFuzzyRule(rpitchHighPlusRollMediumMinus4);
		fuzzy4->addFuzzyRule(rpitchMediumPlusRollMediumMinus4);
		fuzzy4->addFuzzyRule(rpitchZeroRollMediumMinus4);
		fuzzy4->addFuzzyRule(rpitchMediumMinusRollMediumMinus4);
		fuzzy4->addFuzzyRule(rpitchHighMinusRollMediumMinus4);

		fuzzy4->addFuzzyRule(rpitchHighPlusRollHighMinus4);
		fuzzy4->addFuzzyRule(rpitchMediumPlusRollHighMinus4);
		fuzzy4->addFuzzyRule(rpitchZeroRollHighMinus4);
		fuzzy4->addFuzzyRule(rpitchMediumMinusRollHighMinus4);
		fuzzy4->addFuzzyRule(rpitchHighMinusRollHighMinus4);
}

bool FuzzyTest2::frameStarted(const Ogre::FrameEvent &evt)
{

	if (evt.timeSinceLastFrame>3.0f)
		return true;
	//mCpt->step(evt);

	
	return true;
}
