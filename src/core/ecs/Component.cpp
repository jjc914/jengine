#include "Component.hpp"

bool ComponentSet::operator==(const ComponentSet& other) const {
    return component_types == other.component_types;
}