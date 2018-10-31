#include <vector>
#include "EntityComponent.h"

struct Entity {
	std::vector<EntityComponent *> m_components;
};

Entity *create_entity();

void entity_add_component(EntityComponent *component);
bool entity_has_component(EntityComponentEnum componentType);