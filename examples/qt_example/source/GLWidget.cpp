#include <opengl/OpenGLAPI.h>
#include <GLWidget.h>
#include <AssimpImporter.h>
#include <btBulletDynamicsCommon.h>
#include <Leakcheck.h>
#include <renderer/AbstractRenderer.h>
#include <iostream>
#include <Engine.h>
#include <Transform.h>
#include <GameTimer.h>
#include <Camera.h>
#include <CameraController.h>
#include <AntTweakBar.h>
#include <LevelOfDetail.h>
#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>
#include <AntWrapper.h>
#include <physics/Bullet3PhysicsSystem.h>
#include <physics/RigidBody.h>

GLWidget::GLWidget()
{
  _engine = std::make_unique<fly::Engine>();
  _gameTimer = std::make_unique<fly::GameTimer>();
  setMouseTracking(true);
}

GLWidget::~GLWidget()
{
  TwTerminate();
}

void GLWidget::initializeGL()
{
  _renderer = std::make_shared<fly::AbstractRenderer<fly::OpenGLAPI>>(&_graphicsSettings);
  _graphicsSettings.addListener(_renderer);
  _engine->addSystem(_renderer);
  _physicsSystem = std::make_shared<fly::Bullet3PhysicsSystem>();
  _engine->addSystem(_physicsSystem);
  initGame();
  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
  TwInit(TwGraphAPI::TW_OPENGL_CORE, nullptr);
  _bar = TwNewBar("Stats");
  TwAddButton(_bar, _fpsButtonName, nullptr, nullptr, nullptr);
#if RENDERER_STATS
  TwAddButton(_bar, _renderedMeshesName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _renderedMeshesShadowName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _renderedTrianglesName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _renderedTrianglesShadowName, nullptr, nullptr, nullptr);
#endif
  auto settings_bar = TwNewBar("Settings");
  AntWrapper(settings_bar, &_graphicsSettings, _renderer->getApi());
  TwSetTopBar(_bar);
}

void GLWidget::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2i( width, height ));
  TwWindowSize(width, height);
}

void GLWidget::paintGL()
{
  _gameTimer->tick();
  _renderer->setDefaultRendertarget(defaultFramebufferObject());
  _engine->update(_gameTimer->getTimeSeconds(), _gameTimer->getDeltaTimeSeconds());
  if (contains<int>(_keysPressed, 'W')) {
    _camController->stepForward(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'A')) {
    _camController->stepLeft(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'S')) {
    _camController->stepBackward(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'D')) {
    _camController->stepRight(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'C')) {
    _camController->stepDown(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, Qt::Key::Key_Space)) {
    _camController->stepUp(_gameTimer->getDeltaTimeSeconds());
  }
  _fps++;
  if (_gameTimer->getTotalTimeSeconds() >= _measure) {
    _measure = _gameTimer->getTotalTimeSeconds() + 1.f;
    std::string fps_label_str = std::to_string(_fps) + " FPS";
    TwSetParam(_bar, _fpsButtonName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, fps_label_str.c_str());
#if RENDERER_STATS
    const auto& stats = _renderer->getStats();
    TwSetParam(_bar, _renderedMeshesName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Meshes:" + formatNumber(stats._renderedMeshes)).c_str());
    TwSetParam(_bar, _renderedMeshesShadowName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Meshes SM:" + formatNumber(stats._renderedMeshesShadow)).c_str());
    TwSetParam(_bar, _renderedTrianglesName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Triangles:" + formatNumber(stats._renderedTriangles)).c_str());
    TwSetParam(_bar, _renderedTrianglesShadowName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Triangles SM:" + formatNumber(stats._renderedTrianglesShadow)).c_str());
#endif
    _fps = 0;
  }
  TwDraw();
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
  _keysPressed.insert(e->key());
  if (e->key() == Qt::Key::Key_Shift) {
    _camController->acceleratePressed();
  }
  if (e->key() == Qt::Key::Key_Control) {
    _camController->deceleratePressed();
  }
}

void GLWidget::keyReleaseEvent(QKeyEvent * e)
{
  _keysPressed.erase(e->key());
  if (e->key() == Qt::Key::Key_Shift) {
    _camController->accelerateReleased();
  }
  if (e->key() == Qt::Key::Key_Control) {
    _camController->decelerateReleased();
  }
}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    if (!TwMouseButton(TwMouseAction::TW_MOUSE_PRESSED, TwMouseButtonID::TW_MOUSE_LEFT)) {
      _camController->mousePress(fly::Vec3f({ static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f }));
    }
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (_camController->isPressed()) {
    _camController->mouseMove(fly::Vec3f({ static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f }));
  }
  else {
    TwMouseMotion(e->localPos().x(), e->localPos().y());
  }
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    if (_camController->isPressed()) {
      _camController->mouseRelease();
    }
    else {
      TwMouseButton(TwMouseAction::TW_MOUSE_RELEASED, TwMouseButtonID::TW_MOUSE_LEFT);
    }
  }
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
}

void GLWidget::initGame()
{
  auto importer = std::make_shared<fly::AssimpImporter>();
#if TREE_SCENE
  auto tree_model = importer->loadModel("assets/Tree1/Tree1.obj");
  for (const auto& m : tree_model->getMeshes()) {
    auto entity = _engine->getEntityManager()->createEntity();
    entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m, tree_model->getMaterials()[m->getMaterialIndex()], fly::Transform().getModelMatrix(), false));
  }
#else
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  for (const auto& m : sponza_model->getMaterials()) {
    m->setSpecularExponent(32.f);
    if (m->getDiffusePath() == "assets/sponza/textures\\spnza_bricks_a_diff.tga") {
      m->setHeightPath("assets/DisplacementMap.png");
    }
    else if (m->getDiffusePath() == "assets/sponza/textures\\sponza_floor_a_diff.tga") {
      m->setHeightPath("assets/height.png");
    }
  }

  fly::Vec3f sponza_scale (0.01f);
  std::vector<std::shared_ptr<btCollisionShape>> _sponzaShapes;
  for (const auto& mesh : sponza_model->getMeshes()) {
    _triangleMeshes.push_back(std::make_shared<btTriangleMesh>());
    for (unsigned i = 0; i < mesh->getIndices().size(); i += 3) {
      auto v0 = mesh->getVertices()[mesh->getIndices()[i]]._position;
      auto v1 = mesh->getVertices()[mesh->getIndices()[i + 1]]._position;
      auto v2 = mesh->getVertices()[mesh->getIndices()[i + 2]]._position;
      _triangleMeshes.back()->addTriangle(btVector3(v0[0], v0[1], v0[2]), btVector3(v1[0], v1[1], v1[2]), btVector3(v2[0], v2[1], v2[2]));
    }
    _sponzaShapes.push_back(std::make_shared<btBvhTriangleMeshShape>(_triangleMeshes.back().get(), true, true));
    _sponzaShapes.back()->setLocalScaling(btVector3(sponza_scale[0], sponza_scale[1], sponza_scale[2]));
  }

#if SPONZA_MANY
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
#endif
      unsigned index = 0;
      for (const auto& mesh : sponza_model->getMeshes()) {
        auto entity = _engine->getEntityManager()->createEntity();
        bool has_wind = index >= 44 && index <= 62;
        fly::Vec3f aabb_offset = has_wind ? fly::Vec3f(0.f, 0.f, 0.25f) : fly::Vec3f(0.f);
        if (index == sponza_model->getMeshes().size() - 28) {
          has_wind = true;
          aabb_offset = fly::Vec3f(0.f, 0.f, 0.25f);
        }
        entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(mesh,
#if SPONZA_MANY
          sponza_model->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(x * 60.f, 0.f, y * 60.f), fly::Vec3f(sponza_scale)).getModelMatrix(), has_wind, aabb_offset));
#else
          sponza_model->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(0.f), sponza_scale).getModelMatrix(), has_wind, aabb_offset));
#endif
#if PHYSICS
        const auto& model_matrix = entity->getComponent<fly::StaticMeshRenderable>()->getModelMatrix();
        entity->addComponent(std::make_shared<fly::RigidBody>(model_matrix[3].xyz(), 0.f, _sponzaShapes[index], 0.f));
#endif
        index++;
      }
#if SPONZA_MANY
    }
  }
#endif
#endif
#if PHYSICS
  _graphicsSettings.setDebugObjectAABBs(true);
  auto sphere_model = importer->loadModel("assets/sphere.obj");
  for (const auto& m : sphere_model->getMeshes()) {
    auto ent = _engine->getEntityManager()->createEntity();
    auto vec = m->getAABB()->getMax() - m->getAABB()->getMin();
    float radius = std::max(vec[0], std::max(vec[1], vec[2])) * 0.5f;
    auto col_shape = std::make_shared<btSphereShape>(radius);
    float scale = 0.5f;
    col_shape->setLocalScaling(btVector3(scale, scale, scale));
    float mass = 0.1f;
    float restitution = 1.f;
    ent->addComponent(std::make_shared<fly::RigidBody>(fly::Vec3f(0.f, 30.f, 0.f), mass, col_shape, restitution));
    ent->addComponent(std::make_shared<fly::DynamicMeshRenderable>(m, sphere_model->getMaterials()[m->getMaterialIndex()], ent->getComponent<fly::RigidBody>()));
  }
#endif

#if SPONZA_MANY || TREE_SCENE
  auto plane_model = importer->loadModel("assets/plane.obj");
  for (const auto& m : plane_model->getMaterials()) {
    m->setNormalPath("assets/ground_normals.png");
    m->setDiffuseColor(fly::Vec3f(0.870f, 0.768f, 0.329f) * 1.5f);
  }
  for (const auto& m : plane_model->getMeshes()) {
    std::vector<fly::Vertex> vertices_new;
    for (const auto& v : m->getVertices()) {
      fly::Vertex v_new = v;
      v_new._uv *= 120.f;
      vertices_new.push_back(v_new);
    }
    m->setVertices(vertices_new);
  }
  for (const auto& m : plane_model->getMeshes()) {
    auto entity = _engine->getEntityManager()->createEntity();
    auto scale = _renderer->getSceneMax() - _renderer->getSceneMin();
    scale[1] = 1.f;
    auto translation = _renderer->getSceneMin();
    entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m,
      plane_model->getMaterials()[m->getMaterialIndex()], fly::Transform(translation, scale).getModelMatrix(), false));
  }
#endif

  auto cam_entity = _engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(4.f, 2.f, 0.f), glm::vec3(glm::radians(270.f), 0.f, 0.f)));
  auto dl_entity = _engine->getEntityManager()->createEntity();
  _dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(-1000.f, 2000.f, -1000.f), glm::vec3(-500.f, 0.f, -500.f));
  dl_entity->addComponent(_dl);
  
  _camController = std::make_unique<fly::CameraController>(cam_entity->getComponent<fly::Camera>(), 20.f);
#if SPONZA_MANY
  _camController->setSpeed(100.f);
#endif
}

std::string GLWidget::formatNumber(unsigned number)
{
  unsigned num = number;
  std::string ret;
  while (true) {
    unsigned remainder = num % 1000;
    num /= 1000;
    char rem_str[4];
    if (num == 0) {
      if (remainder >= 100) {
        sprintf_s(rem_str, "%3u", remainder);
      }
      else if (remainder >= 10) {
        sprintf_s(rem_str, "%2u", remainder);
      }
      else {
        sprintf_s(rem_str, "%1u", remainder);
      }
    }
    else {
      sprintf_s(rem_str, "%03u", remainder);
    }
    ret = rem_str + ret;
    if (num == 0) {
      break;
    }
    else {
      ret = "," + ret;
    }
  }
  return ret;
}
