#pragma once
#include "test_support.h"
#include "Epidemic/GameFramework/Integration/components.h"
#include <cmath>

namespace composition_tests {
namespace comp = epidemic::gameplay::composition;

struct Health {
    std::int64_t points = 100;
};

struct Movement {
    double speed = 1.0;
    std::string label = "enemy";
};

inline bool ValidHealth(const std::int64_t &value) {
    return value >= 0 && value <= 10000;
}

inline bool ValidSpeed(const double &value) {
    return std::isfinite(value) && value >= 0;
}

struct ComponentsFixture {
    comp::ComponentRegistry registry;
    fw::TypeId health{}, movement{};

    comp::PropertyId points{}, speed{}, label{};

    ComponentsFixture() {
        health = Take(registry.Register<Health>("game.health"));
        movement = Take(registry.Register<Movement>("game.movement"));
        points = Take(registry.RegisterProperty(health, "points", &Health::points, &ValidHealth));
        speed = Take(registry.RegisterProperty(movement, "speed", &Movement::speed, &ValidSpeed));
        label = Take(registry.RegisterProperty(movement, "label", &Movement::label));
        registry.Freeze();
    }

};

} // namespace composition_tests
