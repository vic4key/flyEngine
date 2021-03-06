#include <AssimpImporter.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <Engine.h>
#include <opengl/OpenGLAPI.h>
#include <renderer/AbstractRenderer.h>
#include <Camera.h>
#include <Entity.h>
#include <Light.h>
#include <Transform.h>
#include <StaticMeshRenderable.h>
#include <Model.h>
#include <GameTimer.h>
#include <set>
#include <CameraController.h>
#include <GraphicsSettings.h>

bool handleEvents(sf::RenderWindow& window, const std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>& rs, 
  std::set<sf::Keyboard::Key>& keys_pressed, fly::CameraController* cc, float delta_time)
{
  sf::Event event;
  while (window.pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      return false;
    }
    else if (event.type == sf::Event::Resized) {
      auto size = window.getSize();
      rs->onResize(fly::Vec2u(size.x, size.y));
    }
    else if (event.type == sf::Event::KeyPressed) {
      keys_pressed.insert(event.key.code);
    }
    else if (event.type == sf::Event::KeyReleased) {
      keys_pressed.erase(event.key.code);
    }
  }
  if (keys_pressed.find(sf::Keyboard::W) != keys_pressed.end()) {
    cc->stepForward(delta_time);
  }
  if (keys_pressed.find(sf::Keyboard::A) != keys_pressed.end()) {
    cc->stepLeft(delta_time);
  }
  if (keys_pressed.find(sf::Keyboard::S) != keys_pressed.end()) {
    cc->stepBackward(delta_time);
  }
  if (keys_pressed.find(sf::Keyboard::D) != keys_pressed.end()) {
    cc->stepRight(delta_time);
  }
  if (keys_pressed.find(sf::Keyboard::Space) != keys_pressed.end()) {
    cc->stepUp(delta_time);
  }
  if (keys_pressed.find(sf::Keyboard::C) != keys_pressed.end()) {
    cc->stepDown(delta_time);
  }
  return true;
}

int main()
{
  fly::Vec2u winsize(1024u, 768u);
  sf::RenderWindow window(sf::VideoMode(winsize[0], winsize[1]), "My window");
  auto engine = std::make_unique<fly::Engine>();
  fly::GraphicsSettings gs;
  auto rs = std::make_shared<fly::AbstractRenderer<fly::OpenGLAPI>>(&gs);
  rs->onResize(winsize);
  engine->addSystem(rs);
  {auto cam_entity = engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 3.f, 0.f), glm::vec3(glm::radians(270.f), 0.f, 0.f)));
  auto light = engine->getEntityManager()->createEntity();
  light->addComponent(std::make_shared<fly::DirectionalLight>(fly::Vec3f(1.f), fly::Vec3f( 50.f, 50.f, 0.f ), fly::Vec3f(0.f)));
  light->addComponent(std::shared_ptr<fly::Light>(light->getComponent<fly::DirectionalLight>()));
  auto importer = std::make_unique<fly::AssimpImporter>();
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  for (const auto& m : sponza_model->getMeshes()) {
    auto sponza_entity = engine->getEntityManager()->createEntity();
    sponza_entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m, sponza_model->getMaterials()[m->getMaterialIndex()], fly::Transform(0.f, fly::Vec3f(0.01f)).getModelMatrix(), false));
  }
  fly::GameTimer timer;
  std::set<sf::Keyboard::Key> keys_pressed;
  fly::CameraController cc(cam_entity->getComponent<fly::Camera>(), 20.f);
  while (true) {
    bool active = handleEvents(window, rs, keys_pressed, &cc, timer.getDeltaTimeSeconds());
    if (active) {
      timer.tick();
      engine->update(timer.getTimeSeconds(), timer.getDeltaTimeSeconds());
      window.display();
    }
    else {
      break;
    }
  }
  }
  rs = nullptr;
  engine = nullptr;
  window.close();
  return 0;
}