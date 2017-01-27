#include ".\Ragdoll.h"
#include "PhysObject.h"
#include "global.h"

RagDoll::RagBone::RagBone( RagDoll* creator, OgreNewt::World* world, RagDoll::RagBone* parent, Ogre::Bone* ogreBone, Ogre::MeshPtr mesh, 
						  Ogre::Vector3 dir, RagDoll::RagBone::BoneShape shape, Ogre::Vector3 size, Ogre::Real mass )
{
	mDoll = creator;
	mParent = parent;
	mOgreBone = ogreBone;

	OgreNewt::ConvexCollision* col = NULL;

	// in the case of the cylindrical primitives, they need to be rotated to align the main axis with the direction vector.
	Ogre::Quaternion orient = Ogre::Quaternion::IDENTITY;
	Ogre::Vector3 pos = Ogre::Vector3::ZERO;
	Ogre::Matrix3 rot;

	if (dir == Ogre::Vector3::UNIT_Y)
	{
		rot.FromEulerAnglesXYZ(Ogre::Degree(0), Ogre::Degree(0), Ogre::Degree(90));
		orient.FromRotationMatrix( rot );
	}

	if (dir == Ogre::Vector3::UNIT_Z)
	{
		rot.FromEulerAnglesXYZ(Ogre::Degree(0), Ogre::Degree(90), Ogre::Degree(0));
		orient.FromRotationMatrix( rot );
	}


	// make the rigid body.
	switch (shape)
	{
	case RagDoll::RagBone::BS_BOX:
		col = new OgreNewt::CollisionPrimitives::Box( world, size );
		break;

	case RagDoll::RagBone::BS_CAPSULE:
		col = new OgreNewt::CollisionPrimitives::Capsule( world, size.y, size.x, orient, pos );
		break;

	case RagDoll::RagBone::BS_CONE:
		col = new OgreNewt::CollisionPrimitives::Cone( world, size.y, size.x, orient, pos );
		break;

	case RagDoll::RagBone::BS_CYLINDER:
		col = new OgreNewt::CollisionPrimitives::Cylinder( world, size.y, size.x, orient, pos );
		break;

	case RagDoll::RagBone::BS_ELLIPSOID:
		col = new OgreNewt::CollisionPrimitives::Ellipsoid( world, size );
		break;

	case RagDoll::RagBone::BS_CONVEXHULL:
		col = _makeConvexHull( world, mesh, size.x );
		break;

	default:
		col = new OgreNewt::CollisionPrimitives::Box( world, size );
		break;
	}

	mBody = new OgreNewt::Body( world, col );
	mBody->setUserData( this );
	//mBody->setStandardForceCallback();
	mBody->setCustomForceAndTorqueCallback<RagBone>( &RagDoll::RagBone::bone_force_callback ,this);

	Ogre::Vector3 inertia;
	Ogre::Vector3 com;
	col->calculateInertialMatrix( inertia, com );
	
	mBody->setMassMatrix( mass, inertia * mass );
	mBody->setCenterOfMass( com );
	mBody->setAutoFreeze(false);
	mBody->setCustomTransformCallback( RagDoll::_placementCallback );


}

void RagDoll::RagBone::bone_force_callback( OgreNewt::Body* me)
{
   //apply a simple gravity force. 
   Ogre::Real mass; 
   Ogre::Vector3 inertia; 
   
   me->getMassMatrix(mass, inertia); 
   Ogre::Vector3 force(0,GRAVITY*TIME_SHIFT,0); 
   force *= mass; 

   me->addForce( force ); 
   Vector3 f2 = me->getStdAddForce();
   me->addForce( f2*TIME_SHIFT ); 
   me->setStandartAddForce(Ogre::Vector3::ZERO);
}


RagDoll::RagBone::~RagBone()
{
	mOgreBone->setManuallyControlled( false );
	delete mBody;
}


void RagDoll::RagBone::_hingeCallback( OgreNewt::BasicJoints::Hinge* me )
{
	RagDoll::RagBone* bone = (RagDoll::RagBone*)me->getUserData();

	Ogre::Degree angle = me->getJointAngle();
	Ogre::Degree lim1( bone->getLimit1() );
	Ogre::Degree lim2( bone->getLimit2() );

	if (angle < lim1)
	{
		Ogre::Real accel = me->calculateStopAlpha( lim1 );
		me->setCallbackAccel( accel );
	}

	if (angle > lim2)
	{
		Ogre::Real accel = me->calculateStopAlpha( lim2 );
		me->setCallbackAccel( accel );
	}

}


OgreNewt::ConvexCollision* RagDoll::RagBone::_makeConvexHull( OgreNewt::World* world, Ogre::MeshPtr mesh, Ogre::Real minWeight )
{
	std::vector< Ogre::Vector3 > vertexVector;

	// for this bone, gather all of the vertices linked to it, and make an individual convex hull.
	std::string boneName = mOgreBone->getName();
	unsigned int boneIndex = mOgreBone->getHandle();

	Ogre::Matrix4 invMatrix;
	invMatrix.makeInverseTransform( -mOgreBone->_getBindingPoseInversePosition(), Ogre::Vector3::UNIT_SCALE / mOgreBone->_getBindingPoseInverseScale(), mOgreBone->_getBindingPoseInverseOrientation().Inverse());

	unsigned int num_sub = mesh->getNumSubMeshes();

	for (unsigned int i = 0; i < num_sub; i++)
	{
		Ogre::SubMesh* submesh = mesh->getSubMesh(i);
		Ogre::SubMesh::BoneAssignmentIterator bai = submesh->getBoneAssignmentIterator();

		Ogre::VertexDeclaration* v_decl;
		const Ogre::VertexElement* p_elem;
		float* v_Posptr;
		size_t v_count;
		Ogre::VertexData* v_data = NULL;

		if (submesh->useSharedVertices)
		{
			v_data = mesh->sharedVertexData;
			v_count = v_data->vertexCount;
			v_decl = v_data->vertexDeclaration;
			p_elem = v_decl->findElementBySemantic( Ogre::VES_POSITION );
		}
		else
		{
			v_data = submesh->vertexData;
			v_count = v_data->vertexCount;
			v_decl = v_data->vertexDeclaration;
			p_elem = v_decl->findElementBySemantic( Ogre::VES_POSITION );
		}		
			
		size_t start = v_data->vertexStart;
		//pointer
		Ogre::HardwareVertexBufferSharedPtr v_sptr = v_data->vertexBufferBinding->getBuffer( p_elem->getSource() );
		unsigned char* v_ptr = static_cast<unsigned char*>(v_sptr->lock( Ogre::HardwareBuffer::HBL_READ_ONLY ));
		unsigned char* v_offset;
		
		while (bai.hasMoreElements())
		{
			Ogre::VertexBoneAssignment vba = bai.getNext();
			if (vba.boneIndex == boneIndex)
			{
				//found a vertex that is attached to this bone.
				if (vba.weight >= minWeight)
				{
					//get offset to Position data!
					v_offset = v_ptr + (vba.vertexIndex * v_sptr->getVertexSize());
					p_elem->baseVertexPointerToElement( v_offset, &v_Posptr );

					Ogre::Vector3 vert;
					vert.x = *v_Posptr; v_Posptr++;
					vert.y = *v_Posptr; v_Posptr++;
					vert.z = *v_Posptr; 

					// apply transformation in to local space.
					vert = invMatrix * vert;

					vertexVector.push_back( vert );

					Ogre::LogManager::getSingletonPtr()->logMessage("  vertex found! id:"+Ogre::StringConverter::toString(vba.vertexIndex));
				}
			}

		}

		v_sptr->unlock();

	}
		

	// okay, we have gathered all verts for this bone.  make a convex hull!
	unsigned int numVerts = vertexVector.size();
	Ogre::Vector3* verts = new Ogre::Vector3[ numVerts ];
	unsigned int j = 0;
	while (!vertexVector.empty())
	{
		verts[j] = vertexVector.back();
		vertexVector.pop_back();
		j++;
	}

	//////////////////////////////////////////////////////////////////////////////////
	OgreNewt::ConvexCollision* col = new OgreNewt::CollisionPrimitives::ConvexHull( world, verts, numVerts );

	delete []verts;

	return col;	
}




RagDoll::RagDoll( Ogre::String filename, OgreNewt::World* world, Ogre::SceneNode* node )
{
	mNode = node;
	mWorld = world;

	// get the skeleton.
	mSkeleton = ((Ogre::Entity*)(mNode->getAttachedObject(0)))->getSkeleton();

	// get the mesh.
	mMesh = ((Ogre::Entity*)(mNode->getAttachedObject(0)))->getMesh();

	Ogre::String basename, path;
	Ogre::StringUtil::splitFilename(filename, basename, path);
	Ogre::DataStreamPtr pStream;
	if (Ogre::ResourceGroupManager::getSingleton().resourceExists("General",basename))
	{
			pStream = Ogre::ResourceGroupManager::getSingleton().
				openResource( basename, "General" );
			
	}
	else
	{
			path = "[DotSceneLoader] File "+basename;
			Ogre::LogManager::getSingleton().logMessage(path+" doesn't exist" );
			return;
	}

	Ogre::String data = pStream->getAsString();

	TiXmlDocument* doc = new TiXmlDocument();
	doc->Parse(data.c_str());
	pStream->close();
	pStream.setNull();

	if( doc->Error() )
	{
			//We'll just log, and continue on gracefully
			Ogre::LogManager::getSingleton().logMessage("[RagDoll module] The TiXmlDocument reported an error");
			delete doc;
			return;
	}
	TiXmlElement* root = doc->RootElement();

	if (!root)
	{
		// error!
		Ogre::LogManager::getSingleton().logMessage("[RagDoll module] cannot find 'root' in xml file: "+filename );
		return;
	}

	// found the root ragdoll.  find the root bone, and go!
	RagBone* parent = NULL;

	TiXmlElement* bone = root->FirstChildElement("Bone");

	if (bone)
	{
		_addAllBones( NULL, bone );

	}

}


void RagDoll::_addAllBones(RagDoll::RagBone* parent, TiXmlElement* bone)
{
	// get the information for the bone represented by this element.
	Ogre::Vector3 dir = Ogre::StringConverter::parseVector3( bone->Attribute("dir") );
	Ogre::Real length = Ogre::StringConverter::parseReal( bone->Attribute("length") );
		
	Ogre::Vector3 size = Ogre::StringConverter::parseVector3( bone->Attribute("size") )*mNode->getScale();
	
	
	Ogre::String skeleton_bone = bone->Attribute("skeleton_bone");
	Ogre::Bone* ogrebone = mSkeleton->getBone( skeleton_bone );


	Ogre::String shapestr = bone->Attribute("shape");
	RagDoll::RagBone::BoneShape shape = RagDoll::RagBone::BS_BOX;

	if (shapestr=="box")
		shape = RagDoll::RagBone::BS_BOX;
	
	if (shapestr=="capsule")
		shape = RagDoll::RagBone::BS_CAPSULE;
	
	if (shapestr=="cylinder")
		shape = RagDoll::RagBone::BS_CYLINDER;
	
	if (shapestr=="cone")
		shape = RagDoll::RagBone::BS_CONE;
	
	if (shapestr=="ellipsoid")
		shape = RagDoll::RagBone::BS_ELLIPSOID;

	if (shapestr=="hull")
		shape = RagDoll::RagBone::BS_CONVEXHULL;

	Ogre::Real mass = Ogre::StringConverter::parseReal( bone->Attribute("mass") );
	
	///////////////////////////////////////////////////////////////////////////////
	RagBone* me = _addBone( mWorld, parent, dir, shape, size, mass, ogrebone );
	///////////////////////////////////////////////////////////////////////////////

	// position the bone.
	Ogre::Quaternion boneorient = mNode->getOrientation() * ogrebone->_getDerivedOrientation();
	Ogre::Vector3 bonepos;
	if (shape != RagDoll::RagBone::BS_CONVEXHULL)
		bonepos = mNode->_getFullTransform() * ogrebone->_getDerivedPosition() + (boneorient * (dir * (length*0.5f)));
	else
		bonepos = mNode->_getFullTransform() * ogrebone->_getDerivedPosition();

	me->getBody()->setPositionOrientation( bonepos, boneorient );

	// set offsets
	if (!parent)
	{
		Ogre::Quaternion offsetorient = (boneorient.Inverse()) * mNode->getOrientation();
		Ogre::Vector3 offsetpos = boneorient.Inverse() * (mNode->getPosition() - bonepos);
		me->setOffset( offsetorient, offsetpos );
	}


	// get the joint to connect this bone with it's parent.
	if (parent)
	{
		TiXmlElement* joint = bone->FirstChildElement("Joint");
		if (!joint)
		{
			// error!
			Ogre::LogManager::getSingleton().logMessage(" ERROR! cannot find 'Joint' in xml file!");
			return;
		}

		Ogre::Vector3 jointpin = Ogre::StringConverter::parseVector3( joint->Attribute("pin") );
		Ogre::String jointtypestr = joint->Attribute("type");
		RagDoll::JointType jointtype = RagDoll::JT_BALLSOCKET;

		if (jointtypestr == "ballsocket")
			jointtype = RagDoll::JT_BALLSOCKET;

		if (jointtypestr == "hinge")
			jointtype = RagDoll::JT_HINGE;


		Ogre::Real limit1 = Ogre::StringConverter::parseReal( joint->Attribute("limit1") );
		Ogre::Real limit2 = Ogre::StringConverter::parseReal( joint->Attribute("limit2") );

		Ogre::Vector3 jpos = mNode->_getFullTransform() * ogrebone->_getDerivedPosition();
		Ogre::Vector3 jpin = (mNode->getOrientation() * parent->getOgreBone()->_getDerivedOrientation()) * jointpin;

		_joinBones( jointtype, parent, me, jpos, jpin, limit1, limit2);
	}
	
#ifdef _DEBUG
	Ogre::LogManager::getSingleton().logMessage(" added bone from "+ogrebone->getName()+".");
#endif // _DEBUG

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	// add all children of this bone.
	TiXmlElement* child = bone->FirstChildElement("Bone");

	while (child)
	{
		_addAllBones( me, child );

		child = child->NextSiblingElement("Bone");
	}
}


RagDoll::~RagDoll()
{
	while (mBones.size() > 0)
	{
		RagBone* bone = mBones.back();

		delete bone;

		mBones.pop_back();
	}

}


RagDoll::RagBone* RagDoll::_addBone( OgreNewt::World* world, RagDoll::RagBone* parent, Ogre::Vector3 dir, RagBone::BoneShape shape, Ogre::Vector3 size, Ogre::Real mass, Ogre::Bone* ogrebone )
{
	RagDoll::RagBone* bone = new RagDoll::RagBone( this, world, parent, ogrebone, mMesh, dir, shape, size, mass );
	ogrebone->setManuallyControlled( true );

	mBones.push_back( bone );

	return bone;
}

void RagDoll::_joinBones( RagDoll::JointType type, RagBone* parent, RagBone* child, Ogre::Vector3 pos, Ogre::Vector3 pin, Ogre::Real limit1, Ogre::Real limit2 )
{
	pin.normalise();
	OgreNewt::Joint* joint = NULL;

	switch (type)
	{
	case RagDoll::JT_BALLSOCKET:
		joint = new OgreNewt::BasicJoints::BallAndSocket( child->getBody()->getWorld(), child->getBody(), parent->getBody(), pos );
		((OgreNewt::BasicJoints::BallAndSocket*)joint)->setLimits(pin, Ogre::Degree(limit1), Ogre::Degree(limit2));
		break;

	case RagDoll::JT_HINGE:
		joint = new OgreNewt::BasicJoints::Hinge( child->getBody()->getWorld(), child->getBody(), parent->getBody(), pos, pin );
		((OgreNewt::BasicJoints::Hinge*)joint)->setCallback( RagBone::_hingeCallback );
		joint->setUserData( child );
		child->setLimits( limit1, limit2 );
		break;
	}

}


void RagDoll::_placementCallback( OgreNewt::Body* me, const Ogre::Quaternion& orient, const Ogre::Vector3& pos )
{
	RagDoll::RagBone* bone = (RagDoll::RagBone*)me->getUserData();
	RagDoll* doll = bone->getDoll();

	// is this the root bone?
	if (!bone->getParent())
	{
		Ogre::Quaternion finalorient = (orient * bone->getOffsetOrient());
		Ogre::Vector3 finalpos = pos + (orient * bone->getOffsetPos());

		doll->mNode->setPosition( finalpos );
		doll->mNode->setOrientation( finalorient );
	}
	else
	{
		// standard bone, calculate the local orientation between it and it's parent.
		Ogre::Quaternion parentorient;
		Ogre::Vector3 parentpos;

		bone->getParent()->getBody()->getPositionOrientation( parentpos, parentorient );

		Ogre::Quaternion localorient = parentorient.Inverse() * orient;

		bone->getOgreBone()->setOrientation( localorient );
		
	}
}


void RagDoll::inheritVelOmega( Ogre::Vector3 vel, Ogre::Vector3 omega )
{
	// find main position.
	Ogre::Vector3 mainpos = mNode->getPosition();

	for (RagBoneListIterator it = mBones.begin(); it != mBones.end(); it++)
	{
		Ogre::Vector3 pos;
		Ogre::Quaternion orient;

		(*it)->getBody()->getPositionOrientation( pos, orient );
		(*it)->getBody()->setVelocity( vel + omega.crossProduct( pos - mainpos ) );
	}
}