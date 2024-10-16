#pragma once
#include <string>

class EntityEngine;

/**
 * Base class for all entity systems.
 *
 * An entity system should be used for updating components of entities.
 */
class EntitySystem
{
  public:
    const std::string name;

    bool bUpdatesEnabled = true;

    EntitySystem(std::string name) : name(std::move(name)) {}

  protected:
    friend EntityEngine;

    int updateFrequency = 0; // update this system n times per second. if n = 0 then update(deltaTime) is called, else update(1/n)
    float updateAccumulator = 0;

    virtual void init(EntityEngine *) {};

    virtual void update(double deltaTime, EntityEngine *) = 0;

    virtual ~EntitySystem() = default;

};
