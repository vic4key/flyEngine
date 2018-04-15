#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include <StaticMeshRenderable.h>
#include <StaticModelRenderable.h>
#include <System.h>
#include <renderer/ProjectionParams.h>
#include <renderer/RenderParams.h>
#include <math/FlyMath.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <map>
#include <Model.h>
#include <memory>
#include <Entity.h>
#include <Camera.h>
#include <Light.h>
#include <iostream>
#include <Quadtree.h>
#include <Settings.h>
#include <functional>

namespace fly
{
  template<class API>
  class AbstractRenderer : public System
  {
  public:
    AbstractRenderer() : _api()
    {
      _pp._near = 0.1f;
      _pp._far = 1000.f;
      _pp._fieldOfViewDegrees = 45.f;
    }
    virtual ~AbstractRenderer() {}
    void setSettings(const Settings& settings)
    {
      _api.createCompositeShaderFile(settings);
      if (_settings._shadows != settings._shadows ||
        _settings._shadowPercentageCloserFiltering != settings._shadowPercentageCloserFiltering ||
        _settings._normalMapping != settings._normalMapping ||
        _settings._parallaxMapping != settings._parallaxMapping ||
        _settings._reliefMapping != settings._reliefMapping ||
        _settings._windAnimations != settings._windAnimations) {
        _api.recreateShadersAndMaterials(settings);
        for (const auto& e : _staticMeshRenderables) {
          e.second->init(&_api);
        }
      }
      _gsp._exposure = settings._exposure;
      _settings = settings;
      _api.setAnisotropy(settings._anisotropy);
      if (settings._postProcessing || settings._depthPrePass) {
        _lightingBuffer = _api.createRenderToTexture(_viewPortSize);
        _depthBuffer = _api.createDepthbuffer(_viewPortSize);
      }
      else {
        _lightingBuffer = nullptr;
        _depthBuffer = nullptr;
      }
      _shadowMap = settings._shadows ? _api.createShadowmap(settings._shadowMapSize, settings) : nullptr;
      if (settings._shadows) {
        _staticMeshRenderFuncSM = [this]() {
          std::vector<Mat4f> light_vps;
          auto vp_shadow_volume = _directionalLight->getViewProjectionMatrices(_viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._fieldOfViewDegrees,
            inverse(_gsp._viewMatrix), _directionalLight->getViewMatrix(), static_cast<float>(_settings._shadowMapSize), _settings._smFrustumSplits, light_vps, _api.isDirectX());
          std::vector<StaticMeshRenderable*> sm_visible_elements = _quadtree->getVisibleElements<API::isDirectX()>(vp_shadow_volume);
          std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<StaticMeshRenderable*>>> sm_display_list;
          for (const auto& e : sm_visible_elements) {
            sm_display_list[e->_shaderDescDepth][e->_materialDesc.get()].push_back(e);
          }
          _api.setDepthClampEnabled<true>();
          _api.setViewport(Vec2u(_settings._shadowMapSize));
          for (unsigned i = 0; i < _settings._smFrustumSplits.size(); i++) {
            _api.setRendertargets({}, _shadowMap.get(), i);
            _api.clearRendertarget<false, true, false>(Vec4f());
            _gsp._VP = &light_vps[i];
            for (const auto& e : sm_display_list) {
              _api.setupShaderDesc(*e.first, _gsp);
              for (const auto& e1 : e.second) {
                e1.first->setupDepth(e.first->getShader().get());
                for (const auto& smr : e1.second) {
                  smr->_meshRenderFuncDepth();
                }
              }
            }
          }
          _gsp._worldToLight = light_vps;
          _gsp._smFrustumSplits = _settings._smFrustumSplits;
          _gsp._smBias = _settings._smBias;
          _api.setDepthClampEnabled<false>();
        };
      }
      else {
        _staticMeshRenderFuncSM = []() {};
      }
      if (settings._dlSortMode == DisplayListSortMode::SHADER_AND_MATERIAL) {
        _staticMeshRenderFunc = [this]() {
          std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<StaticMeshRenderable*>>> display_list;
          for (const auto& e : _visibleMeshes) {
            display_list[e->_shaderDesc][e->_materialDesc.get()].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupShaderDesc(*e.first, _gsp);
            for (const auto& e1 : e.second) {
              e1.first->setup(e.first->getShader().get());
              for (const auto& smr : e1.second) {
                smr->_meshRenderFunc();
              }
            }
          }
        };
      }
      else {
        _staticMeshRenderFunc = [this]() {
          std::map<typename API::ShaderDesc*, std::vector<StaticMeshRenderable*>> display_list;
          for (const auto& e : _visibleMeshes) {
            display_list[e->_shaderDesc].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupShaderDesc(*e.first, _gsp);
            for (const auto& smr : e.second) {
              smr->_materialDesc->setup(e.first->getShader().get());
              smr->_meshRenderFunc();
            }
          }
        };
      }
    }
    const Settings& getSettings() const
    {
      return _settings;
    }
    virtual void onComponentsChanged(Entity* entity) override
    {
      auto camera = entity->getComponent<Camera>();
      auto dl = entity->getComponent<DirectionalLight>();
      auto mr = entity->getComponent<fly::StaticMeshRenderable>();
      if (mr) {
        auto mesh_renderable = std::make_shared<StaticMeshRenderable>();
        mesh_renderable->_materialDesc = _api.createMaterial(mr->getMaterial(), _settings);
        mesh_renderable->_meshData = _meshGeometryStorage.addMesh(mr->getMesh());
        mesh_renderable->_smr = mr;
        mesh_renderable->init(&_api);
        _staticMeshRenderables[entity] = mesh_renderable;
        _sceneMin = minimum(_sceneMin, mr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, mr->getAABBWorld()->getMax());
      }
      else {
        _staticMeshRenderables.erase(entity);
        // TODO: erase the renderable from the quadtree as well
      }
      if (camera) {
        _camera = camera;
      }
      if (dl) {
        _directionalLight = dl;
      }
    }
    virtual void update(float time, float delta_time) override
    {
      if (_quadtree == nullptr) {
        buildQuadtree();
      }
      if (_camera && _directionalLight) {
        _api.beginFrame();
        _gsp._viewMatrix = _camera->getViewMatrix(_camera->_pos, _camera->_eulerAngles);
        _vpScene = _gsp._projectionMatrix * _gsp._viewMatrix;
        _api.setDepthTestEnabled<true>();
        _api.setFaceCullingEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        _api.setDepthWriteEnabled<true>();
        _gsp._lightPosWorld = _directionalLight->_pos;
        _gsp._camPosworld = _camera->_pos;
        _gsp._lightIntensity = _directionalLight->getIntensity();
        _gsp._time = time;
        _meshGeometryStorage.bind();
        _staticMeshRenderFuncSM();
        _api.setViewport(_viewPortSize);
        _gsp._VP = &_vpScene;
        _visibleMeshes = _quadtree->getVisibleElements<API::isDirectX()>(_vpScene);
        if (_settings._depthPrePass) {
          _api.setRendertargets({}, _depthBuffer.get());
          _api.clearRendertarget<false, true, false>(Vec4f());
          std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<StaticMeshRenderable*>>> display_list;
          for (const auto& e : _visibleMeshes) {
            display_list[e->_shaderDescDepth][e->_materialDesc.get()].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupShaderDesc(*e.first, _gsp);
            for (const auto& e1 : e.second) {
              e1.first->setupDepth(e.first->getShader().get());
              for (const auto& smr : e1.second) {
                smr->_meshRenderFuncDepth();
              }
            }
          }
          _api.setDepthWriteEnabled<false>();
          _api.setDepthFunc<API::DepthFunc::EQUAL>();
        }
        (_settings._postProcessing || _settings._depthPrePass) ? _api.setRendertargets({ _lightingBuffer.get() }, _depthBuffer.get()) : _api.bindBackbuffer(_defaultRenderTarget);
        _settings._depthPrePass ? _api.clearRendertarget<true, false, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f)) : _api.clearRendertarget<true, true, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f));
        if (_shadowMap) {
          _api.bindShadowmap(*_shadowMap);
        }
        _staticMeshRenderFunc();
        if (_settings._debugQuadtreeNodeAABBs) {
          renderQuadtreeAABBs();
        }
        if (_settings._debugObjectAABBs) {
          renderObjectAABBs(_visibleMeshes);
        }
        if (_settings._postProcessing || _settings._depthPrePass) {
          _api.setDepthTestEnabled<false>();
          _api.bindBackbuffer(_defaultRenderTarget);
          _api.composite(_lightingBuffer.get(), _gsp);
        }
        _api.endFrame();
      }
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      float aspect_ratio = _viewPortSize[0] / _viewPortSize[1];
      _gsp._projectionMatrix = _api.getZNearMapping() == ZNearMapping::ZERO ?
        glm::perspectiveRH_ZO(glm::radians(_pp._fieldOfViewDegrees), aspect_ratio, _pp._near, _pp._far) :
        glm::perspectiveRH_NO(glm::radians(_pp._fieldOfViewDegrees), aspect_ratio, _pp._near, _pp._far);

      setSettings(_settings);
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    inline void reloadShaders() { _api.reloadShaders(); }
    const Vec3f& getSceneMin() const { return _sceneMin; }
    const Vec3f& getSceneMax() const { return _sceneMax; }
    std::vector<std::shared_ptr<Material>> getAllMaterials() { return _api.getAllMaterials(); }
  private:
    API _api;
    ProjectionParams _pp;
    GlobalShaderParams _gsp;
    Vec2f _viewPortSize;
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    Vec3f _sceneMin = Vec3f(std::numeric_limits<float>::max());
    Vec3f _sceneMax = Vec3f(std::numeric_limits<float>::lowest());
    Settings _settings;
    std::unique_ptr<typename API::RTT> _lightingBuffer;
    std::unique_ptr<typename API::Depthbuffer> _depthBuffer;
    std::unique_ptr<typename API::Shadowmap> _shadowMap;
    unsigned _defaultRenderTarget = 0;
    Mat4f _vpScene;

    // Wrapper for StaticMeshRenderable
    struct StaticMeshRenderable
    {
      std::shared_ptr<typename API::MaterialDesc> _materialDesc;
      typename API::MeshGeometryStorage::MeshData _meshData;
      std::shared_ptr<fly::StaticMeshRenderable> _smr;
      typename API::ShaderDesc* _shaderDesc;
      typename API::ShaderDesc* _shaderDescDepth;
      std::function<void()> _meshRenderFunc;
      std::function<void()> _meshRenderFuncDepth;
      void init(const API* api)
      {
        _shaderDesc = _materialDesc->getMeshShaderDesc(_smr->hasWind()).get();
        _shaderDescDepth = _materialDesc->getMeshShaderDescDepth(_smr->hasWind()).get();
        if (!_smr->hasWind()) {
          _meshRenderFunc = [this, api]() {
            api->renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse());
          };
          _meshRenderFuncDepth = [this, api]() {
            api->renderMesh(_meshData, _smr->getModelMatrix());
          };
        }
        else {
          _meshRenderFunc = [this, api]() {
            api->renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getWindParams(), *getAABBWorld());
          };
          _meshRenderFuncDepth = [this, api]() {
            api->renderMesh(_meshData, _smr->getModelMatrix(), _smr->getWindParams(), *getAABBWorld());
          };
        }
      }
      inline AABB* getAABBWorld() const { return _smr->getAABBWorld(); }
    };
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::map<Entity*, std::shared_ptr<StaticMeshRenderable>> _staticMeshRenderables;
    std::unique_ptr<Quadtree<StaticMeshRenderable>> _quadtree;
    std::vector<StaticMeshRenderable*> _visibleMeshes;
    std::function<void()> _staticMeshRenderFunc;
    std::function<void()> _staticMeshRenderFuncSM;

    void buildQuadtree()
    {
      _quadtree = std::make_unique<Quadtree<StaticMeshRenderable>>(Vec2f({ _sceneMin[0], _sceneMin[2] }), Vec2f({ _sceneMax[0], _sceneMax[2] }));
      for (const auto& e : _staticMeshRenderables) {
        _quadtree->insert(e.second.get());
      }
    }
    void renderQuadtreeAABBs()
    {
      auto visible_nodes = _quadtree->getVisibleNodes(_vpScene);
      if (visible_nodes.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        std::vector<AABB*> aabbs;
        for (const auto& n : visible_nodes) {
          aabbs.push_back(n->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _vpScene, Vec3f(1.f, 0.f, 0.f));
      }
    }
    void renderObjectAABBs(const std::vector<StaticMeshRenderable*>& visible_elements)
    {
      if (visible_elements.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        std::vector<AABB*> aabbs;
        for (const auto& e : visible_elements) {
          aabbs.push_back(e->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _vpScene, Vec3f(0.f, 1.f, 0.f));
      }
    }
  };
}

#endif
