/////////////////////////////////////////////////////////////////////
///////////////Original file by:Fyodor Zagumennov aka Sgw32//////////
///////////////Copyright(c) 2010 Fyodor Zagumennov		   //////////
/////////////////////////////////////////////////////////////////////
#pragma once
#include <Ogre.h>

#include "Run3SoundRuntime.h"
#include "global.h"
#include <OgreNewt.h>
#include "Timeshift.h"
#include <time.h>
//FUZZY LOGIC TOOLBOX

#include "FuzzyRule.h"
#include "FuzzyComposition.h"
#include "Fuzzy.h"
#include "FuzzyRuleConsequent.h"
#include "FuzzyOutput.h"
#include "FuzzyInput.h"
#include "FuzzyIO.h"
#include "FuzzySet.h"
#include "FuzzyRuleAntecedent.h"
#include "NamedPipeServer.h"

//

//FFLL

//#include "FFLLAPI.h"

//PID Controller

#include "PID.h"

//

#define LOWP(x) (x<0.3f)&&(x>=0.0f) 
#define MEDIUMP(x) (x>0.3f)&&(x<0.7f)
#define HIGHP(x) (x>0.7f)&&(x<=1.0f)
#define LOWM(x) (x>-0.3f)&&(x<=0.0f) 
#define MEDIUMM(x) (x<-0.3f)&&(x>-0.7f)
#define HIGHM(x) (x<-0.7f)&&(x>=-1.0f)

using namespace Ogre;

enum FUZZY_OBJECT_EVENT
{
	ENABLE1,
	ENABLE2,
	ENABLE3,
	ENABLE4,
	ENABLE5
};

enum REG_TYPE_OBJECT2
{
	PID_CONTROL,
	FUZZY_CONTROL,
	FUZZY_CONTROL2
};

class langVar
{
public:
      langVar(float data)
      {
       mV=data;
       outputValue1=outputValue2=0;
      }
      langVar()
      {
      }
      ~langVar()
      {
      }
      void setValue(float data)
      {
           mV=data;
      }
      void fuzzyfy()
      {
           float x = mV;
           if (x>=0)
           {
           ltm=0.0f;
           mtm=0.0f;
           htm=0.0f;
                   
           if (x<0.2f)
           {
              lt=1.0f;
              mt=0.0f;
              ht=0.0f;
           }
           
           if ((x>=0.2f)&&(x<0.3f))
           {
              lt=(0.3f-x)/0.1f;
              mt=(x-0.2f)/0.1f;
              ht=0.0f;
           }
           
           if ((x>=0.3f)&&(x<0.6f))
           {
              lt=0.0f;
              mt=1.0f;
              ht=0.0f;
           }
           
           if ((x>=0.6f)&&(x<0.7f))
           {
              lt=0.0f;
              mt=(0.7f-x)/0.1f;
              ht=(x-0.6f)/0.1f;
           }
           
           if (x>=0.7f)
           {
              lt=0.0f;
              mt=0.0f;
              ht=1.0f;        
           }
           }
           else
           {
               lt=0.0f;
           mt=0.0f;
           ht=0.0f;
                   
           if (fabs(x)<0.2f)
           {
              ltm=1.0f;
              mtm=0.0f;
              htm=0.0f;
           }
           
           if ((fabs(x)>=0.2f)&&(fabs(x)<0.3f))
           {
              ltm=(0.3f-fabs(x))/0.1f;
              mtm=(fabs(x)-0.2f)/0.1f;
              htm=0.0f;
           }
           
           if ((fabs(x)>=0.3f)&&(fabs(x)<0.6f))
           {
              ltm=0.0f;
              mtm=1.0f;
              htm=0.0f;
           }
           
           if ((fabs(x)>=0.6f)&&(fabs(x)<0.7f))
           {
              ltm=0.0f;
              mtm=(0.7f-fabs(x))/0.1f;
              htm=(fabs(x)-0.6f)/0.1f;
           }
           
           if (fabs(x)>=0.7f)
           {
              ltm=0.0f;
              mtm=0.0f;
              htm=1.0f;        
           }
           }
          term="HIGHMINUS";
          if (htm<mtm)
          term="MEDIUMMINUS";
          if (mtm<ltm)
          term="LOWMINUS";
          if (lt>ltm)
          term="LOWPLUS";
          if (mt>lt)
          term="MEDIUMPLUS";
          if (ht>mt)
          term="HIGHPLUS";
          
          #ifdef ALL_DEBUG
                 cout<<htm<<" "<<mtm<<" "<<ltm<<" "<<lt<<" "<<mt<<" "<<ht<<endl;
          #endif
          processRules();
          aggregate();
          deffuzuficate();
              
      }
      
      void aggregate()
      {
      }
      
      
      void processRules()
      {
           /*На этом этапе мы считаем, 
           что на самом деле используются параллельно 
           2 нечетких контроллера по углу тангажа.
           Соответственно, выход даёт 2 переменные -
           мощность заднего(них) и передного(них) двигателей.
           */
           //Нечеткие правила
           /*if (term=="LOWMINUS")
           {
                                outputterm1="LOWPLUS";
                                outputterm2="LOWMINUS";
           }
           if (term=="MEDIUMMINUS")
           {
                                outputterm1="MEDIUMPLUS";
                                outputterm2="MEDIUMMINUS";
           }
           if (term=="HIGHMINUS")
           {
                                outputterm1="HIGHPLUS";
                                outputterm2="HIGHMINUS";
           }*/
           //PITCH MINUS
           o1_ltm=lt;
           o1_mtm=mt;
           o1_htm=ht;
           o2_ltm=ltm;
           o2_mtm=mtm;
           o2_htm=htm;
           //PITCH PLUS
           o1_lt=ltm;
           o1_mt=mtm;
           o1_ht=htm;
           o2_lt=lt;
           o2_mt=mt;
           o2_ht=ht;
           /*o1_mt=0;
           o1_ht=0;
           o1_ltm=0;
           o1_mtm=0;
           o1_htm=0;
           o2_lt=0;
           o2_mt=0;
           o2_ht=0;
           o2_ltm=0;
           o2_mtm=0;
           o2_htm=0;
           
           if (LOWM(ltm))
           {
                         o1_lt=ltm;
            //             o1_mt=0;
              //           o1_ht=0;
                //         o1_ltm=0;
                  //       o1_mtm=0;
                    //     o1_htm=0;
                      //   o2_lt=0;
                        // o2_mt=0;
                         //o2_ht=0;
                         o2_ltm=ltm;
                        // o2_mtm=0;
                         //o2_htm=0;
           }
           if (MEDIUMM(ltm))
           {
                         //o1_lt=0;
                         o1_mt=mtm;
                         //o1_ht=0;
                         //o1_ltm=0;
                         //o1_mtm=0;
                         //o1_htm=0;
                         //o2_lt=0;
                         //o2_mt=0;
                         //o2_ht=0;
                         o2_mtm=mtm;
                         //o2_mtm=0;
                         //o2_htm=0;
           }
           if (HIGHM(htm))
           {
                         //o1_lt=ltm;
                         //o1_mt=0;
                         o1_ht=htm;
                         //o1_ltm=0;
                         //o1_mtm=0;
                         //o1_htm=0;
                         //o2_lt=0;
                         //o2_mt=0;
                         //o2_ht=0;
                         o2_htm=htm;
                         o2_mtm=0;
                         o2_htm=0;
           }*/
           #ifdef ALL_DEBUG
           cout<<o1_htm<<" "<<o1_mtm<<" "<<o1_ltm<<" "<<o1_lt<<" "<<o1_mt<<" "<<o1_ht<<endl;
           cout<<o2_htm<<" "<<o2_mtm<<" "<<o2_ltm<<" "<<o2_lt<<" "<<o2_mt<<" "<<o2_ht<<endl;
           #endif
      }
      //от 0 до 1
      // с учётом того, что треугольник обрезается сверху.
      float getTriangleLevel(float level,float x1, float x2,float x)
      {
           //x2 - 1 (но на самом деле level)
           //x1 - 0
           if (((x-x1)/(x2-x1))<level)
              return (x-x1)/(x2-x1);
           else
              return level;
      }
      //от 0 до 1
      float calcCenterOfMassTrapezoid(float level,float x1, float x2, float x3,float x4)
      {
            //AREA
           float triange1=(x2-x1)*level/2;
           float triangle2=(x4-x3)*level/2;
           float box=(x3-x2)*level;
           float boxc=(x3-x2)/2;
           float triangle1c=(x2-x1)*2/3;
           float triangle2c=(x4-x3)*1/3;
           float center = (boxc+triangle1c+triangle2c)/3; 
           if (level)
           return center;
           else
           return 0; 
      }

      void deffuzuficate()
      {
           //Выход - 0..1 (мин-макс для двигателей)
           //Малый - 0..0.3 
           //Средний - 0.2..0.7
           //Большой - 0.5..1
           float chm1 = calcCenterOfMassTrapezoid(o1_htm,-1.0f,-1.0f,-0.5f,-0.7f);
           float cmm1 = calcCenterOfMassTrapezoid(o1_mtm,-0.7f,-0.5f,-0.2f,-0.3f);
           float clm1 = calcCenterOfMassTrapezoid(o1_ltm,-0.2f,-0.3f,0,0.1f);
           float cl1 = calcCenterOfMassTrapezoid(o1_lt,-0.1f,0.0f,0.2f,0.3f);
           float cm1 = calcCenterOfMassTrapezoid(o1_mt,0.2f,0.3f,0.5f,0.7f);
           float ch1 = calcCenterOfMassTrapezoid(o1_ht,0.5f,0.7f,1.0f,1.0f);
           float out = (chm1+cmm1+clm1+cl1+cm1+ch1);
           #ifdef ALL_DEBUG
           cout<<chm1<<" "<<cmm1<<" "<<clm1<<" "<<cl1<<" "<<cm1<<" "<<ch1<<endl;
           #endif
           int cnt=0;
           if (chm1)
              cnt++;
           if (cmm1)
              cnt++;
           if (clm1)
              cnt++;
           if (cl1)
              cnt++;
           if (cm1)
              cnt++;
           if (ch1)
              cnt++;
           outputValue1=out/cnt;
           cnt=0;
           float chm2 = calcCenterOfMassTrapezoid(o2_htm,-1.0f,-1.0f,-0.5f,-0.7f);
           float cmm2 = calcCenterOfMassTrapezoid(o2_mtm,-0.7f,-0.5f,-0.2f,-0.3f);
           float clm2 = calcCenterOfMassTrapezoid(o2_ltm,-0.2f,-0.3f,-0.1f,0.1f);
           float cl2 = calcCenterOfMassTrapezoid(o2_lt,-0.1f,0.1f,0.2f,0.3f);
           float cm2 = calcCenterOfMassTrapezoid(o2_mt,0.2f,0.3f,0.5f,0.7f);
           float ch2 = calcCenterOfMassTrapezoid(o2_ht,0.5f,0.7f,1.0f,1.0f);
           out = (chm2+cmm2+clm2+cl2+cm2+ch2);
           if (chm2)
              cnt++;
           if (cmm2)
              cnt++;
           if (clm2)
              cnt++;
           if (cl2)
              cnt++;
           if (cm2)
              cnt++;
           if (ch2)
              cnt++;
           #ifdef ALL_DEBUG
           cout<<chm2<<" "<<cmm2<<" "<<clm2<<" "<<cl2<<" "<<cm2<<" "<<ch2<<endl;
           #endif
           outputValue2=out/cnt;
      }
      float getHighPlus()
      {
            return ht;
      }
      float getMediumPlus()
      {
            return mt;
      }
      float getLowPlus()
      {
            return lt;
      }
      float getHighMinus()
      {
            return htm;
      }
      float getMediumMinus()
      {
            return mtm;
      }
      float getLowMinus()
      {
            return ltm;
      }
      string getCurrentTerm()
      {
             return term;
      }
      
      float getOutputValue1()
      {
            return outputValue1;
      }
      float getOutputValue2()
      {
            return outputValue2;
      }
      string getOutputTerm1()
      {
             return outputterm1;
      }
      string getOutputTerm2()
      {
             return outputterm2;
      }
private:
      float mV; 
      string term; 
      string outputterm1;
      string outputterm2;
      float outputValue1,outputValue2;
      float ltm,o1_ltm,o2_ltm;
      float mtm,o1_mtm,o2_mtm;
      float htm,o1_htm,o2_htm;
      float lt,o1_lt,o2_lt;
      float mt,o1_mt,o2_mt;
      float ht,o1_ht,o2_ht;
};



class copter
{
public:
       copter()
       {
               p=y=r=0;
               p=1.0f;
               
               fuzzyPitch = langVar::langVar(p);
               motorv1=motorv2=0.1f;
               l1=10.0f;
               l2=10.0f;
               wp=0;
               motorin1=motorin2=0;
       }
	   ~copter(){};
       void step(const Ogre::FrameEvent &evt)
       {
           
            
             forceY1=-motorv1*10+cos(p*3.1415926535f/180)*(motorin1);
             forceX1=sin(p*3.1415926535f/180)*(motorin1);
             forceY2=-motorv2*10+cos(p*3.1415926535f/180)*(motorin2);
             forceX2=sin(p*3.1415926535f/180)*(motorin2);
             forceDv1 = sqrt(forceY1*forceY1+forceX1*forceX1);
             forceDv2 = sqrt(forceY2*forceY2+forceX2*forceX2);
             cosNormal = cos((90+p)*3.1415926535/180);
             moment = forceDv1*l1-forceDv2*l2;
			 wp+=moment*0.1*evt.timeSinceLastFrame;         
             if ((p>-90)&&(p<90))
             {
            // p+=wp*0.1*evt.timeSinceLastFrame;
             fuzzyPitch.setValue(p/90);
             fuzzyPitch.fuzzyfy();
             }       
#ifdef ALL_DEBUG
			 cout<<moment<<endl;
             cout<<"MOMENT"<<endl;
             
             cout<<wp<<endl;
             cout<<"Pitch Omega"<<endl;
             
             cout<<forceDv1<<endl;
             cout<<"forceDv1"<<endl;
             cout<<forceDv2<<endl;
             cout<<"forceDv2"<<endl;
             
             cout<<p<<fuzzyPitch.getCurrentTerm()<<endl;

             cout<<"Output motor 1"<<endl;
             cout<<fuzzyPitch.getOutputValue1()<<endl;
             cout<<"Output motor 2"<<endl;
             cout<<fuzzyPitch.getOutputValue2()<<endl;
#endif
             motorin1=fuzzyPitch.getOutputValue1()*50; //В 2 раза больше силы тяжести
             motorin2=fuzzyPitch.getOutputValue2()*50;
       }
       void fuzzyfy(float pitch)
       {
            
       }
       void setPitch(float pitch)
	   {
		   p=pitch;
	   }
	   float getPitch()
	   {
		   return p;
	   }
	   float getPitchOmega()
	   {
		   return wp;
	   }
       void setBackMotorInput(float inp);
       void setRearMotorInput(float inp);
private:
        langVar fuzzyPitch;
        float p,y,r;
        float wp;
        float motorv1,motorv2;     
        float motorin1,motorin2;
        float l1,l2;
		float forceX1,forceX2,forceDv1,forceDv2,cosNormal,moment;
        float forceY1,forceY2;
};

class FileLogger {

        public:


            // If you can?t/dont-want-to use C++11, remove the "class" word after enum
            enum e_logType { LOG_ERROR, LOG_WARNING, LOG_INFO };

			/*FileLogger()
			{

			}*/

            // ctor (remove parameters if you don?t need them)
            FileLogger ()
                  :   numWarnings (0U),
                      numErrors (0U)
            {

                myFile.open ("fuzzy_test.log");

                // Write the first lines
                if (myFile.is_open()) {
                    myFile << "Fuzzy Test, version 1.0"  << std::endl;
                } // if

            }


            // dtor
            ~FileLogger () {

                if (myFile.is_open()) {
                    myFile << std::endl << std::endl;

                    // Report number of errors and warnings
                    myFile << numWarnings << " warnings" << std::endl;
                    myFile << numErrors << " errors" << std::endl;

                    myFile.close();
                } // if

            }


            // Overload << operator using log type
            friend FileLogger &operator << (FileLogger &logger, const e_logType l_type) {

                switch (l_type) {
                    case FileLogger::e_logType::LOG_ERROR:
                        logger.myFile << "[ERROR]: ";
                        ++logger.numErrors;
                        break;

                    case FileLogger::e_logType::LOG_WARNING:
                        logger.myFile << "[WARNING]: ";
                        ++logger.numWarnings;
                        break;

                    default:
                        logger.myFile << "[INFO]: ";
                        break;
                } // sw


                return logger;

            }


            // Overload << operator using C style strings
            // No need for std::string objects here
            friend FileLogger &operator << (FileLogger &logger, const char *text) {

                logger.myFile << text;
                return logger;

            }

			/*inline FileLogger& operator = ( FileLogger& logger )
			{
				myFile=logger.myFile;

				return *this;
			}*/
            // Make it Non Copyable (or you can inherit from sf::NonCopyable if you want)
            //FileLogger (const FileLogger &) = delete;
            //FileLogger &operator= (const FileLogger &) = delete;


			std::ofstream           myFile;
        private:

            

            unsigned int            numWarnings;
            unsigned int            numErrors;

}; // class end

class FuzzyObject
{
public:
	FuzzyObject(String manName){LogManager::getSingleton().logMessage(manName+" fuzzy object initialized!");}
	FuzzyObject(){};
	virtual ~FuzzyObject(){}
	virtual void init(Ogre::SceneManager* mSceneMgr)
	{
	}

	virtual void setup(Ogre::SceneNode* do_node){}
	virtual void setup(Ogre::SceneNode* do_node,bool box){}
	virtual void force_callback( OgreNewt::Body* me ){}
	virtual void setname(String name){}
	virtual void set_position(Vector3 pos){}
	virtual void set_orientation(Quaternion quat){}
	virtual Quaternion get_orientation()
	{
		return Quaternion::IDENTITY;
	}
	virtual void rotateBody(Quaternion quat){}
	virtual Vector3 get_position(){return Vector3::ZERO;}
	virtual String getname(){return "";}
	virtual void unload(){}
	virtual void Fire(String event){}
	virtual void setUseInteract(bool interact)
	{
	}
	virtual bool frameStarted(const Ogre::FrameEvent &evt){return true;}
	FileLogger myLog;
};

class FuzzyTest : public FuzzyObject
{
public:
	FuzzyTest();
	~FuzzyTest();
	void init(Ogre::SceneManager* mSceneMgr)
	{
		scene = mSceneMgr;
	}

	
	void setup(Ogre::SceneNode* do_node);
	void setup(Ogre::SceneNode* do_node,bool box);
	void FuzzyTest::force_callback( OgreNewt::Body* me );
	void setname(String name);
	void set_position(Vector3 pos);
	void set_orientation(Quaternion quat);
	Quaternion get_orientation()
	{
		Quaternion quat;
		Vector3 pos;
		door_bod->getPositionOrientation(pos,quat);
		return quat;
	}
	void rotateBody(Quaternion quat);
	Vector3 get_position();
	String getname();
	void unload();
	void Fire(String event);
	void setUseInteract(bool interact)
	{
		mInteract=interact;
	}
	void enablePIDs()
	{

	}
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	copter* mCpt;
	SceneNode* door_node;
	SceneManager* scene;
	OgreNewt::Body* door_bod;
	bool mInteract;
};

class FuzzyTest2 : public FuzzyObject
{
public:
	FuzzyTest2();
	~FuzzyTest2();
	void init(Ogre::SceneManager* mSceneMgr)
	{
		scene = mSceneMgr;
	}

	void fuzzySetup();
	void buildRules();
	void setupNamedPipeControl()
	{
		if (reg_type==FUZZY_CONTROL2)
		{
			mNamedPipe = new NamedPipeServer("Named Pipe initialized!");
			mNamedPipe->init();
			
		}
	}
	const std::string currentDateTime() 
	{
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
		// for more information about date/time format
		strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

		return buf;
	}
	void setupPID()
	{
		pid_pitch.kP(1);
		pid_pitch.kI(0);
		pid_pitch.kD(0);
		pid_roll.kP(1);
		pid_roll.kI(0);
		pid_roll.kD(0);
		pid_yaw.kP(1);
		pid_yaw.kI(0);
		pid_yaw.kD(0);
	}

	float getPitch()
	{
		return get_orientation().getPitch().valueDegrees();
	}

	float getRoll()
	{
		return get_orientation().getRoll().valueDegrees();
		//return 0;
	}
	
	void fuzzyfyFFCL();

	float getPIDPitch()
	{
		Ogre::FrameEvent evt;
		evt.timeSinceLastFrame=0.1;
		return getPIDPitch(getPitch(),evt);
	}

	float getPIDRoll()
	{
		Ogre::FrameEvent evt;
		evt.timeSinceLastFrame=0.1;
		//return 0;
		return getPIDRoll(getRoll(),evt);
	}

	inline float getPIDPitch(float pitch,const Ogre::FrameEvent &evt)
	{
		return pid_pitch.get_pid(-pitch,1,evt)/90.0f;
	}

	inline float getPIDRoll(float pitch,const Ogre::FrameEvent &evt)
	{
		return pid_roll.get_pid(-pitch,1,evt)/90.0f;
	}

	OgreNewt::Body* addMotorGroup(Ogre::SceneNode* node, Entity* ent, Vector3 pos, Vector3 scale, Real pscale, Quaternion rot);

	void parseMotor(Entity* ent, Vector3 pos,Vector3 refpos, Vector3 scale, Real pscale, Quaternion rot);
	void setup(Ogre::SceneNode* do_node);
	void setup(Ogre::SceneNode* do_node,bool box);
	void FuzzyTest2::force_callback( OgreNewt::Body* me );

	void FuzzyTest2::motor1_callback( OgreNewt::Body* me );
	void FuzzyTest2::motor2_callback( OgreNewt::Body* me );
	void FuzzyTest2::motor3_callback( OgreNewt::Body* me );
	void FuzzyTest2::motor4_callback( OgreNewt::Body* me );

	void setname(String name);
	void set_position(Vector3 pos);
	void set_orientation(Quaternion quat);
	Quaternion get_orientation()
	{
		Quaternion quat;
		Vector3 pos;
		door_bod->getPositionOrientation(pos,quat);
		return quat;
	}
	void rotateBody(Quaternion quat);
	Vector3 get_position();
	String getname();//{return mName;}
	void unload();
	void Fire(String event);
	void setRegType(int type)
	{
		if (reg_type==FUZZY_CONTROL2)
		{
		myLog << "Fuzzy logic control.";
		}
		if (reg_type==PID_CONTROL)
		{
		myLog << "PID logic control.";
		}
		reg_type=type;
	}
	void setUseInteract(bool interact)
	{
		mInteract=interact;
	}
	void processEvent(int event);
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
private:
	vector<OgreNewt::BasicJoints::UpVector*> hors;
	vector<OgreNewt::BasicJoints::UpVector*> hors2;
	vector<OgreNewt::BasicJoints::UpVector*> hors3;

	Fuzzy* fuzzy;
	Fuzzy* fuzzy2;
	Fuzzy* fuzzy3;
	Fuzzy* fuzzy4;

	FuzzyInput* pitch;// 
	FuzzyInput* roll;
	FuzzyOutput* fMotor1;
	FuzzyOutput* fMotor2;
	FuzzyOutput* fMotor3;
	FuzzyOutput* fMotor4;

	float pidpitch;
	float pidroll;

	float crisp1;
	float crisp2;
	float crisp3;
	float crisp4;
	//PITCH
	FuzzySet* highminus;
	FuzzySet* mediumminus;
	FuzzySet* zero;
	FuzzySet* mediumplus;
	FuzzySet* highplus;
	String mName;
	//ROLL
	FuzzySet* highminus2;
	FuzzySet* mediumminus2;
	FuzzySet* zero2;
	FuzzySet* mediumplus2;
	FuzzySet* highplus2;

	//Motor1
	FuzzySet* highminus3;
	FuzzySet* mediumminus3;
	FuzzySet* zero3;
	FuzzySet* mediumplus3;
	FuzzySet* highplus3;

	PID pid_pitch;
	PID pid_yaw;
	PID pid_roll;


	NamedPipeServer* mNamedPipe;
	//BEGIN FFCL SECTION

	///int model;
	///int child;
	int motor_cnt;
	int reg_type;
	SceneNode* door_node;
	SceneManager* scene;
	OgreNewt::Body* door_bod;
	bool mInteract;
};