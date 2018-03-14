#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include <set>
#include <EntityManager.h>

namespace fly
{
  class System;

  class Engine
  {
  public:
    void addSystem(const std::shared_ptr<System>& system);
    void update(float time, float delta_time);
    EntityManager* getEntityManager() const;
  private:
    std::unique_ptr<EntityManager> _em = std::unique_ptr<EntityManager>(new EntityManager());
    std::set<std::shared_ptr<System>> _systems;
  };
}

#endif
