/*  Copyright 2010-2012 Matthew Paul Reid
    This file is subject to the terms and conditions defined in
    file 'License.txt', which is part of this source code package.
*/
#include "CSMGpuConstants.h"

namespace Ogre {

const Matrix4 PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE(
    0.5,    0,    0,  0.5, 
    0,   -0.5,    0,  0.5, 
    0,      0,    1,    0,
    0,      0,    0,    1);


CSMGpuConstants::CSMGpuConstants(size_t cascadeCount)
{
	mParamsScaleBias = GpuProgramManager::getSingletonPtr()->createSharedParameters("params_shadowMatrixScaleBias");
	for (size_t i=1; i<cascadeCount; i++)
	{
		mParamsScaleBias->addConstantDefinition("texMatrixScaleBias" + StringConverter::toString(i), GCT_FLOAT4);
	}
	
	mParamsShadowMatrix = GpuProgramManager::getSingletonPtr()->createSharedParameters("params_shadowMatrix");
	mParamsShadowMatrix->addConstantDefinition("texMatrix0", GCT_MATRIX_4X4);
}

void CSMGpuConstants::updateCascade(const Ogre::Camera &texCam, size_t index)
{
	if (index == 0)
	{
		mFirstCascadeViewMatrix = texCam.getViewMatrix();
		mFirstCascadeCamWidth = texCam.getOrthoWindowWidth();
		mViewRange = texCam.getFarClipDistance() - texCam.getNearClipDistance();

		Matrix4 texMatrix0 = PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE * texCam.getProjectionMatrixWithRSDepth() * mFirstCascadeViewMatrix;
		mParamsShadowMatrix->setNamedConstant("texMatrix0", texMatrix0);
	}
	else
	{
		hack = PROJECTIONCLIPSPACE2DTOIMAGESPACE_PERSPECTIVE * texCam.getProjectionMatrixWithRSDepth() * texCam.getViewMatrix();

		Matrix4 mat0 = mFirstCascadeViewMatrix;
		Matrix4 mat1 = texCam.getViewMatrix();

		Real offsetX = mat1[0][3] - mat0[0][3];
		Real offsetY = mat1[1][3] - mat0[1][3];
		Real offsetZ = mat1[2][3] - mat0[2][3];

		Real width0 = mFirstCascadeCamWidth;
		Real width1 = texCam.getOrthoWindowWidth();
		
		Real oneOnWidth = 1.0f / width0;
		Real offCenter = width1 / (2.0f * width0) - 0.5;

		RenderSystem* rs = Ogre::Root::getSingletonPtr()->getRenderSystem();
		float depthRange = Math::Abs(rs->getMinimumDepthInputValue() - rs->getMaximumDepthInputValue());

		Vector4 result;
		result.x = offsetX * oneOnWidth + offCenter;
		result.y = -offsetY * oneOnWidth + offCenter;
		result.z = -depthRange * offsetZ / mViewRange;
		result.w = width0 / width1;

		mParamsScaleBias->setNamedConstant("texMatrixScaleBias" + StringConverter::toString(index), result);
	}
}

} // namespace Ogre
