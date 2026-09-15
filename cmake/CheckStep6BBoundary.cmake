set(_headers
    "${RUN3_SOURCE_ROOT}/include/run3/gameplay/LegacyMaterialCatalog.hpp"
    "${RUN3_SOURCE_ROOT}/include/run3/gameplay/PlayerController.hpp"
    "${RUN3_SOURCE_ROOT}/include/run3/gameplay/StaticMap.hpp")
foreach(_header IN LISTS _headers)
    file(READ "${_header}" _text)
    if(_text MATCHES "OgreNewt|Newton\\.h|btBullet|BulletDynamics|BulletCollision")
        message(FATAL_ERROR "Physics backend leaked into public header: ${_header}")
    endif()
endforeach()
