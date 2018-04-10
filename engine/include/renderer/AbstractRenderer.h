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
      if (_settings._shadows != settings._shadows ||
        _settings._shadowPercentageCloserFiltering != settings._shadowPercentageCloserFiltering ||
        _settings._smBias != settings._smBias ||
        _settings._normalMapping != settings._normalMapping) {
        _api.recreateShadersAndMaterials(settings);
      }
      _settings = settings;
      if (settings._postProcessing) {
        _lightingBuffer = _api.createRenderToTexture(_viewPortSize);
        _depthBuffer = _api.createDepthbuffer(_viewPortSize);
      }
      else {
        _lightingBuffer = nullptr;
        _depthBuffer = nullptr;
      }
      _shadowMap = settings._shadows ? _api.createShadowmap(settings._shadowMapSize, settings) : nullptr;
      if (settings._shadows) {
        _shaderSetupFunc = [this](typename API::MaterialDesc::ShaderProgram* shader) {
          _api.setupShader(shader, _rp, _shadowMap.get());
        };
        _materialSetupFunc = [this](const typename API::MaterialDesc& mat_desc) {
          _api.setupMaterial(mat_desc, _rp, _shadowMap.get());
        };
        _staticMeshRenderFuncSM = [this]() {
          std::vector<Mat4f> light_vps;
          auto vp_shadow_volume = _directionalLight->getViewProjectionMatrices(_viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._fieldOfViewDegrees,
            _rp._Vinverse, _directionalLight->getViewMatrix(), static_cast<float>(_settings._shadowMapSize), _settings._smFrustumSplits, light_vps, _api.isDirectX());
          std::vector<StaticMeshRenderable*> sm_visible_elements = _quadtree->getVisibleElements<API::isDirectX()>(vp_shadow_volume);
          std::map<typename API::MaterialDesc::ShaderProgram*, std::vector<StaticMeshRenderable*>> sm_display_list;
          for (const auto& e : sm_visible_elements) {
            sm_display_list[e->_materialDesc->getSMShader().get()].push_back(e);
          }
          _api.setDepthClampEnabled<true>();
          _api.setViewport(Vec2u(_settings._shadowMapSize));
          for (unsigned i = 0; i < _settings._smFrustumSplits.size(); i++) {
            _api.setRendertargets({}, _shadowMap.get(), i);
            _api.clearRendertarget<false, true, false>(Vec4f());
            for (const auto& e : sm_display_list) {
              _api.setupShader(e.first);
              for (const auto& smr : e.second) {
                _api.renderMeshMVP(smr->_meshData, light_vps[i] * smr->_smr->getModelMatrix());
              }
            }
          }
          _rp._viewToLight.resize(light_vps.size());
          for (unsigned i = 0; i < _settings._smFrustumSplits.size(); i++) {
            _rp._viewToLight[i] = light_vps[i] * _rp._Vinverse;
          }
          _rp._smFrustumSplits = _settings._smFrustumSplits;
          _api.setDepthClampEnabled<false>();
        };
      }
      else {
        _shaderSetupFunc = [this](typename API::MaterialDesc::ShaderProgram* shader) {
          _api.setupShader(shader, _rp);
        };
        _materialSetupFunc = [this](const typename API::MaterialDesc& mat_desc) {
          _api.setupMaterial(mat_desc, _rp);
        };
        _staticMeshRenderFuncSM = []() {};
      }
      if (settings._dlSortMode == DisplayListSortMode::SHADER_AND_MATERIAL) {
        _staticMeshRenderFunc = [this]() {
          std::map<typename API::MaterialDesc::ShaderProgram*, std::map<typename API::MaterialDesc*, std::vector<StaticMeshRenderable*>>> display_list;
          for (const auto& e : _visibleMeshes) {
            display_list[e->_materialDesc->getShader().get()][e->_materialDesc.get()].push_back(e);
          }
          for (const auto& e : display_list) {
            _shaderSetupFunc(e.first);
            for (const auto& e1 : e.second) {
              e1.first->setup();
              for (const auto& smr : e1.second) {
                _api.renderMesh(smr->_meshData, _rp._viewMatrix * smr->_smr->getModelMatrix());
              }
            }
          }
        };
      }
      else {
        _staticMeshRenderFunc = [this]() {
          std::map<typename API::MaterialDesc*, std::vector<StaticMeshRenderable*>> display_list;
          for (const auto& e : _visibleMeshes) {
            display_list[e->_materialDesc.get()].push_back(e);
          }
          for (const auto& e : display_list) {
            _materialSetupFunc(*e.first);
            for (const auto& smr : e.second) {
              _api.renderMesh(smr->_meshData, _rp._viewMatrix * smr->_smr->getModelMatrix());
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
        _rp._viewMatrix = _camera->getViewMatrix(_camera->_pos, _camera->_eulerAngles);
        _rp._VP = _rp._projectionMatrix * _rp._viewMatrix;
        if (_settings._shadows) {
          _rp._Vinverse = inverse(_rp._viewMatrix);
        }
        _api.setDepthTestEnabled<true>();
        _api.setFaceCullingEnabled<true>();
        _rp._lightPosView = (_rp._viewMatrix * Vec4f(_directionalLight->_pos, 1.f)).xyz();
        _rp._lightIntensity = _directionalLight->getIntensity();
        _meshGeometryStorage.bind();
        _staticMeshRenderFuncSM();
        _settings._postProcessing ? _api.setRendertargets({ _lightingBuffer.get() }, _depthBuffer.get()) : _api.bindBackbuffer(_defaultRenderTarget);
        _api.setViewport(_viewPortSize);
        _api.clearRendertarget<true, true, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f));
        _visibleMeshes = _quadtree->getVisibleElements<API::isDirectX()>(_rp._VP);
        _staticMeshRenderFunc();
        if (_settings._debugQuadtreeNodeAABBs) {
          renderQuadtreeAABBs();
        }
        if (_settings._debugObjectAABBs) {
          renderObjectAABBs(_visibleMeshes);
        }
        if (_settings._postProcessing) {
          _api.setDepthTestEnabled<false>();
          _api.bindBackbuffer(_defaultRenderTarget);
          _api.composite(_lightingBuffer.get());
        }
      }
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      float aspect_ratio = _viewPortSize[0] / _viewPortSize[1];
      _rp._projectionMatrix = _api.getZNearMapping() == ZNearMapping::ZERO ?
        glm::perspectiveRH_ZO(glm::radians(_pp._fieldOfViewDegrees), aspect_ratio, _pp._near, _pp._far) :
        glm::perspectiveRH_NO(glm::radians(_pp._fieldOfViewDegrees), aspect_ratio, _pp._near, _pp._far);

      setSettings(_settings);
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    inline void reloadShaders() { _api.reloadShaders(); }
    const Vec3f& getSceneMin() const { return _sceneMin; }
    const Vec3f& getSceneMax() const { return _sceneMax; }
  private:
    API _api;
    ProjectionParams _pp;
    GlobalShaderParams _rp;
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

    // Wrapper for StaticMeshRenderable
    struct StaticMeshRenderable
    {
      std::shared_ptr<typename API::MaterialDesc> _materialDesc;
      typename API::MeshGeometryStorage::MeshData _meshData;
      std::shared_ptr<fly::StaticMeshRenderable> _smr;
      inline AABB* getAABBWorld() const { return _smr->getAABBWorld(); }
    };
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::map<Entity*, std::shared_ptr<StaticMeshRenderable>> _staticMeshRenderables;
    std::unique_ptr<Quadtree<StaticMeshRenderable>> _quadtree;
    std::vector<StaticMeshRenderable*> _visibleMeshes;

    std::function<void(typename API::MaterialDesc::ShaderProgram*)> _shaderSetupFunc;
    std::function<void(const typename API::MaterialDesc&)> _materialSetupFunc;
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
      auto visible_nodes = _quadtree->getVisibleNodes(_rp._VP);
      if (visible_nodes.size()) {
        std::vector<AABB*> aabbs;
        for (const auto& n : visible_nodes) {
          aabbs.push_back(n->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _rp._VP, Vec3f(1.f, 0.f, 0.f));
      }
    }
    void renderObjectAABBs(const std::vector<StaticMeshRenderable*>& visible_elements)
    {
      if (visible_elements.size()) {
        std::vector<AABB*> aabbs;
        for (const auto& e : visible_elements) {
          aabbs.push_back(e->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _rp._VP, Vec3f(0.f, 1.f, 0.f));
      }
    }
  };
}

#endif
