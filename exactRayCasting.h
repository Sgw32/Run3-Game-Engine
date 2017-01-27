using namespace Ogre;
using namespace MyGUI;

String rayName;
RaySceneQuery *mRaySceneQuery = 0;

void GetMeshInformation(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3 *&vertices, size_t &index_count, unsigned long *&indices, const Ogre::Vector3 &position, const Ogre::Quaternion &orient, const Ogre::Vector3 &scale) {
	bool added_shared = false;
	size_t current_offset = 0;
	size_t shared_offset = 0;
	size_t next_offset = 0;
	size_t index_offset = 0;
	vertex_count = index_count = 0;
	for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i) {
		Ogre::SubMesh* submesh = mesh->getSubMesh( i );
		if ( !submesh ) continue;
		if(submesh->useSharedVertices) {
			if( !added_shared ) {
				vertex_count += mesh->sharedVertexData->vertexCount;
				added_shared = true;
			}
		} else {
			vertex_count += submesh->vertexData->vertexCount;
		}
		index_count += submesh->indexData->indexCount;
	}
	if ( vertices ) delete vertices;
	if ( indices ) delete indices;
	vertices = new Ogre::Vector3[vertex_count];
	indices = new unsigned long[index_count];
	added_shared = false;
	for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i) {
		Ogre::SubMesh* submesh = mesh->getSubMesh(i);
		if ( !submesh ) continue;
		Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
		if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared)) {
			if(submesh->useSharedVertices) {
				added_shared = true;
				shared_offset = current_offset;
			}
			const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
			Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
			unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
			float* pReal;
			for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize()) {
				posElem->baseVertexPointerToElement(vertex, &pReal);
				Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
				vertices[current_offset + j] = (orient * (pt * scale)) + position;
			}
			vbuf->unlock();
			next_offset += vertex_data->vertexCount;
		}
		Ogre::IndexData* index_data = submesh->indexData;
		size_t numTris = index_data->indexCount / 3;
		Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
		bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);
		unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
		unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);
		size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;
		if ( use32bitindexes ) {
			for ( size_t k = 0; k < numTris*3; ++k) {
				indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
			}
		} else {
			for ( size_t k = 0; k < numTris*3; ++k) {
				indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
					static_cast<unsigned long>(offset);
			}
		}
		ibuf->unlock();
		current_offset = next_offset;
	}
}

int getFirstName(Ogre::Real _x, Ogre::Real _y) {
	Ogre::Real x,y;
	x = Real( Real( _x ) / Real( window->getWidth( ) ) );
	y = Real( Real( _y ) / Real( window->getHeight( ) ) );
	Ogre::Ray _ray = mCamera->getCameraToViewportRay(x, y); /// стреляем лучом из курсора
	Ogre::Vector3 inters = Ogre::Vector3::ZERO;
	Entity* ent, *ent_fin = 0;
    Vector3 singleRes;
    if ( mRaySceneQuery != NULL ) {
        mRaySceneQuery->clearResults( );
        mRaySceneQuery->setQueryTypeMask( SceneManager::ENTITY_TYPE_MASK);
        mRaySceneQuery->setRay( _ray );
        mRaySceneQuery->setSortByDistance(true);
        if ( mRaySceneQuery->execute().size() <= 0 ) {
            return 0;
        }
    } else {
        Ogre::LogManager::getSingleton().logMessage("...Cannot raycast without RaySceneQuery instance...");
        return 0;
    }
    Ogre::Real closest_distance = -1.0f;
    Ogre::Vector3 closest_result;
    Ogre::RaySceneQueryResult &query_result = mRaySceneQuery->getLastResults();
    Ogre::RaySceneQueryResult::iterator itr = query_result.begin( );
/// перебор всех объектов, в которые попал луч
    for (size_t qr_idx = 0; qr_idx < query_result.size( ); qr_idx++, itr++ ) {
        if ((closest_distance >= 0.0f) && (closest_distance < query_result[qr_idx].distance)) {
            break;
        }
        if (query_result[qr_idx].movable != NULL) {
            ent = static_cast< Ogre::Entity * >(query_result[ qr_idx ].movable);
            if ( ent ) {
                size_t vertex_count = 0;
                size_t index_count = 0;
                Ogre::Vector3 *vertices = new Ogre::Vector3(Ogre::Vector3::ZERO);
                unsigned long *indices = new unsigned long(0);
                if ( ! ent->getMesh( ).isNull( ) ) {
                    if ( ent->getMesh( )->getNumSubMeshes( ) > 10000 ) continue;
                    GetMeshInformation(ent->getMesh(), vertex_count, vertices, index_count, indices,
                            ent->getParentNode()->_getDerivedPosition(),
                            ent->getParentNode()->_getDerivedOrientation(),
                            ent->getParentNode()->_getDerivedScale());
                    bool new_closest_found = false;
                    for (int i = 0; i < static_cast<int>(index_count); i += 3) {
						std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(_ray, vertices[indices[i]],
						vertices[indices[i+1]], vertices[indices[i+2]],Ogre::Math::calculateBasicFaceNormal(vertices[indices[i]],vertices[indices[i+1]], vertices[indices[i+2]]), true,true);
						if (hit.first) {
							if ((closest_distance < 0.5f) || (hit.second < closest_distance)) {
								closest_distance = hit.second;
								new_closest_found = true;
							}
						}
                    }
                    delete[] vertices;
                    delete[] indices;
                    if (new_closest_found) {
                        closest_result = _ray.getPoint(closest_distance);
                        ent_fin = ent;
                    }
                }
            }
        }
    }
    if (closest_distance >= 0.0f && ent_fin)
    {
        rayName = ent_fin->getName();   /// вот и имя объекта
    }
}