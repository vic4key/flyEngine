#ifndef RENDERPARAMS_H
#define RENDERPARAMS_H

#include <math/FlyMath.h>
#include <vector>

namespace fly
{
  enum class ZNearMapping
  {
    ZERO, // directx
    MINUS_ONE // opengl
  };

  struct GlobalShaderParams
  {
    Mat4f _projectionMatrix;
    Mat4f _viewMatrix;
    Mat4f _VP;
    Mat4f _Vinverse;
    std::vector<Mat4f> _worldToLight;
    std::vector<float> _smFrustumSplits;
    Vec3f _lightPosWorld;
    Vec3f _camPosworld;
    Vec3f _lightIntensity;
    ZNearMapping _zNearMapping;
  };
}

#endif