/*
	Ragdoll.h

	OgreNewt custom ragdoll class
*/
#ifndef _WALABER_RAGDOLL_
#define _WALABER_RAGDOLL_

#include <OgreNewt.h>
#include <Ogre.h>
#include "tinyxml.h"
#include "tinystr.h"
#include "Timeshift.h"

class RagDoll
{
public:
	/*
		Main bone class, represents a single bone (rigid body) in the ragdoll.  is linked to a bone in the model too.
	*/
	class RagBone
	{
	public:
		enum BoneShape
		{
			BS_BOX, BS_ELLIPSOID, BS_CYLINDER, BS_CAPSULE, BS_CONE, BS_CONVEXHULL
		};

		RagBone( RagDoll* creator, OgreNewt::World* world, RagBone* parent, Ogre::Bone* ogreBone, Ogre::MeshPtr mesh, Ogre::Vector3 dir, RagBone::BoneShape shape, Ogre::Vector3 size, Ogre::Real mass );

		~RagBone();

		// newton body.
		OgreNewt::Body* getBody() { return mBody; }

		// set limits.
		void setLimits( Ogre::Real limit1, Ogre::Real limit2 ) { mLimit1 = limit1; mLimit2 = limit2; }

		void bone_force_callback( OgreNewt::Body* me);


		// get limits.
		Ogre::Real getLimit1() { return mLimit1; }
		Ogre::Real getLimit2() { return mLimit2; }

		// get ogre bone
		Ogre::Bone* getOgreBone() { return mOgreBone; }

		// hinge callback.
		static void _hingeCallback( OgreNewt::BasicJoints::Hinge* me );

		// get parent.
		RagBone* getParent() { return mParent; }

		// get doll.
		RagDoll* getDoll() { return mDoll; }
		
		// set offset.
		void setOffset( Ogre::Quaternion orient, Ogre::Vector3 pos ) { mOffsetOrient = orient; mOffsetPos = pos; }

		// get offsets
		Ogre::Quaternion getOffsetOrient() { return mOffsetOrient; }
		Ogre::Vector3 getOffsetPos() { return mOffsetPos; }

	private:

		OgreNewt::ConvexCollision* _makeConvexHull( OgreNewt::World* world, Ogre::MeshPtr mesh, Ogre::Real minWeight );

		// pointer to the doll to which this bone belongs.
		RagDoll* mDoll;

		RagBone* mParent;

		OgreNewt::Body* mBody;

		Ogre::Bone* mOgreBone;

		// limits.
		Ogre::Real mLimit1;
		Ogre::Real mLimit2;

		// offset, for root bone use.
		Ogre::Vector3 mOffsetPos;
		Ogre::Quaternion mOffsetOrient;

	};


	// constructor
	RagDoll( Ogre::String filename, OgreNewt::World* world, Ogre::SceneNode* node );

	~RagDoll();

	// set a different main node
	void setSceneNode( Ogre::SceneNode* node ) { mNode = node; }

	void inheritVelOmega( Ogre::Vector3 vel, Ogre::Vector3 omega );
	
	Ogre::SceneNode* getSceneNode(){return mNode;}

	// newton callbacks go here.
	static void _placementCallback( OgreNewt::Body* me, const Ogre::Quaternion& orient, const Ogre::Vector3& pos );

private:
	
	enum JointType
	{
		JT_BALLSOCKET, JT_HINGE
	};

	// add a bone!
	RagBone* _addBone( OgreNewt::World* world, RagBone* parent, Ogre::Vector3 dir, RagBone::BoneShape shape, Ogre::Vector3 size, Ogre::Real mass, Ogre::Bone* ogrebone );

	// recursive function for creating bones.
	void _addAllBones( RagBone* parent, TiXmlElement* bone );

	// add a joint between 2 bones.
	void _joinBones( RagDoll::JointType type, RagBone* parent, RagBone* child, Ogre::Vector3 pos, Ogre::Vector3 pin, Ogre::Real limit1, Ogre::Real limit2 );

	// list of bones.
	typedef std::list<RagBone*> RagBoneList;
	typedef std::list<RagBone*>::iterator RagBoneListIterator;

	RagBoneList mBones;

	
	// scene node
	Ogre::SceneNode* mNode;

	// mesh for this character.
	Ogre::MeshPtr mMesh;

	// skeleton instance.
	Ogre::SkeletonInstance* mSkeleton;

	// world.
	OgreNewt::World* mWorld;

};



#endif	// _WALABER_RAGDOLL_
	
