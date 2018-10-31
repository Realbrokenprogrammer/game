#pragma once

struct HealthComponent {
	float currentHealth;
	float maxHealth;
	bool alive;
};

struct InputComponent {
};

enum EntityComponentEnum {
	HEALTH_COMPONENT,
	INPUT_COMPONENT
};

struct EntityComponent {
	EntityComponentEnum type;

	union {
		HealthComponent health;
		InputComponent input;
	};
};