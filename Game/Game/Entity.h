#include <vector>
#include "EntityComponent.h"
#include "Platform.h"

enum EntityType {
	ENTITY_PLATFORM
};

struct Entity {
	EntityType type;
	std::vector<EntityComponent *> m_components;

	union {
		Platform platform;
	};
};

Entity *create_entity();

void entity_add_component(EntityComponent *component);
bool entity_has_component(EntityComponentEnum componentType);