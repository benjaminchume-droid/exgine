#pragma once

#include "exgine/geometry.hpp"

#include <cstdint>
#include <string>

namespace exgine {

enum class CharacterType { Player, NPC };
enum class ClothingSlot { Head, Torso, Legs, Feet, Hands, Accessory };

enum class CharacterArchetype { Civilian, Worker, Police, Soldier, Custom };

struct CharacterAppearance {
    std::uint64_t seed = 1;
    CharacterArchetype archetype = CharacterArchetype::Civilian;
    float height = 1.75f;
    float build = 1.0f;
    std::string skin_material = "skin";
    std::string hair_material = "hair";
    std::string shirt_material = "fabric";
    std::string pants_material = "denim";
    std::string shoe_material = "leather";
    bool watch = false;
    bool backpack = false;
};

struct CharacterDefinition {
    CharacterType type = CharacterType::NPC;
    CharacterAppearance appearance{};
};

// Generates a deterministic humanoid assembly from a seed. Geometry is kept
// as reusable parts so animation, clothing, materials, and physics can attach
// without baking gameplay into the mesh generator.
MeshAssembly generate_character(const CharacterDefinition& definition);

} // namespace exgine
