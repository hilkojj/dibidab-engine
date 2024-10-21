#pragma once

#include <entt/entity/registry.hpp>

#include <string>


class EntityEngine;

/**
 * Abstract class.
 *
 * Entity templates are used to construct one (or more) entities with a collection of components.
 */
class EntityTemplate
{
  private:
    friend class EntityEngine;
    int templateHash = -1;

  protected:
    EntityEngine *engine = nullptr;

  public:

    virtual const std::string &getDescription() const;

    entt::entity create(bool persistent=false);

    virtual void createComponents(entt::entity, bool persistent=false) = 0;

  protected:

    virtual ~EntityTemplate() = default;

};
