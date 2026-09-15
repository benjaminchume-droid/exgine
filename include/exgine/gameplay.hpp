#pragma once

#include "exgine/character.hpp"
#include "exgine/physics.hpp"
#include "exgine/vehicle.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class CharacterMovementState : std::uint8_t { Idle, Walk, Run, Crouch, Jump, Fall, Climb, Swim, Dead };
enum class CharacterPosture : std::uint8_t { Standing, Crouching, Sitting };
enum class DamageType : std::uint8_t { Physical, Blunt, Slash, Pierce, Projectile, Fire, Fall, Drowning };
enum class ItemType : std::uint8_t { Generic, Food, Drink, Clothing, Equipment, MeleeWeapon, RangedWeapon, Medical, Quest };
enum class InteractionType : std::uint8_t { Use, Pickup, Drop, Open, Close, Sit, EnterVehicle, ExitVehicle, Talk, Attack };
enum class ActivityType : std::uint8_t { Idle, Eat, Sleep, Work, Socialize, Shop, Travel, Rest };

enum class ScheduleAction : std::uint8_t { Idle, MoveTo, Work, Eat, Sleep, Socialize, Shop, Rest };

using CharacterId = std::uint64_t;
using ItemId = std::uint64_t;
using InteractionId = std::uint64_t;
inline constexpr CharacterId invalid_character = 0;
inline constexpr ItemId invalid_item = 0;
inline constexpr InteractionId invalid_interaction = 0;

struct CharacterControllerConfig {
    float radius = 0.34f;
    float standing_height = 1.8f;
    float crouching_height = 1.1f;
    float walk_speed = 2.0f;
    float run_speed = 5.5f;
    float crouch_speed = 1.1f;
    float acceleration = 24.0f;
    float braking = 30.0f;
    float jump_speed = 5.2f;
    float gravity_scale = 1.0f;
    float step_height = 0.35f;
    float slope_limit_degrees = 48.0f;
    float climb_speed = 1.6f;
    float swim_speed = 2.5f;
    PhysicsLayer layer = 1U;
    PhysicsMask mask = 0xffffffffU;
    [[nodiscard]] bool valid() const noexcept;
};

struct CharacterControllerInput {
    float move_x = 0;
    float move_z = 0;
    bool run = false;
    bool crouch = false;
    bool jump = false;
    bool climb = false;
    bool swim = false;
    [[nodiscard]] bool valid() const noexcept;
};

struct CharacterControllerState {
    CharacterMovementState movement = CharacterMovementState::Idle;
    CharacterPosture posture = CharacterPosture::Standing;
    Vec3 velocity{};
    Vec3 ground_normal{0,1,0};
    bool grounded = false;
    bool climbing = false;
    bool swimming = false;
    [[nodiscard]] bool valid() const noexcept;
};

struct CharacterController {
    CharacterControllerConfig config{};
    CharacterControllerState state{};
    PhysicsBodyId body = invalid_physics_body;
    PhysicsColliderId collider = invalid_physics_collider;
    void reset() noexcept;
    [[nodiscard]] bool valid() const noexcept;
    bool create(PhysicsWorld&, Vec3 position);
    bool destroy(PhysicsWorld&) noexcept;
    bool update(PhysicsWorld&, CharacterControllerInput, float dt) noexcept;
    [[nodiscard]] std::optional<PhysicsTransform> transform(const PhysicsWorld&) const noexcept;
};

struct ItemDefinition {
    ItemId id = invalid_item;
    std::string name;
    ItemType type = ItemType::Generic;
    float mass = 0.1f;
    float value = 0;
    float nutrition = 0;
    float hydration = 0;
    float damage = 0;
    float range = 1.5f;
    std::uint32_t max_stack = 1;
    [[nodiscard]] bool valid() const noexcept;
};

struct InventorySlot { ItemId item = invalid_item; std::uint32_t quantity = 0; };
class Inventory {
public:
    explicit Inventory(std::uint32_t capacity = 32);
    [[nodiscard]] std::uint32_t capacity() const noexcept;
    [[nodiscard]] const std::vector<InventorySlot>& slots() const noexcept;
    bool add(ItemId item, std::uint32_t quantity = 1, std::uint32_t max_stack = 1);
    bool remove(ItemId item, std::uint32_t quantity = 1);
    [[nodiscard]] std::uint32_t count(ItemId item) const noexcept;
    void clear() noexcept;
private:
    std::uint32_t capacity_ = 32;
    std::vector<InventorySlot> slots_;
};

struct CharacterVitals {
    float health = 100;
    float max_health = 100;
    float stamina = 100;
    float max_stamina = 100;
    float hunger = 0;
    float thirst = 0;
    float fatigue = 0;
    float oxygen = 100;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool alive() const noexcept;
};

struct DamageEvent { CharacterId source = invalid_character; CharacterId target = invalid_character; DamageType type = DamageType::Physical; float amount = 0; Vec3 point{}; Vec3 normal{}; };
struct CombatResult { bool hit = false; bool killed = false; float damage = 0; };

struct CharacterInstance {
    CharacterId id = invalid_character;
    CharacterType type = CharacterType::NPC;
    CharacterDefinition definition{};
    CharacterController controller{};
    CharacterVitals vitals{};
    Inventory inventory{};
    ItemId equipped_item = invalid_item;
    CharacterPosture posture = CharacterPosture::Standing;
    bool unconscious = false;
    bool recovering = false;
    float recovery_time = 0;
    [[nodiscard]] bool valid() const noexcept;
};

struct InteractionTarget {
    InteractionId id = invalid_interaction;
    InteractionType type = InteractionType::Use;
    Vec3 position{};
    float radius = 1.5f;
    std::uint64_t target_entity = 0;
    ItemId item = invalid_item;
    bool enabled = true;
    [[nodiscard]] bool valid() const noexcept;
};

struct ScheduleEntry {
    std::uint16_t start_minute = 0;
    std::uint16_t end_minute = 0;
    ScheduleAction action = ScheduleAction::Idle;
    Vec3 target{};
    std::uint64_t target_entity = 0;
    [[nodiscard]] bool valid() const noexcept;
};
struct NPCSchedule { std::vector<ScheduleEntry> entries; [[nodiscard]] bool valid() const noexcept; };
struct NPCState { ActivityType activity = ActivityType::Idle; std::size_t schedule_index = 0; Vec3 target{}; std::uint64_t target_entity = 0; };

struct GameplayWorldSettings { float time_scale = 1.0f; float seconds_per_game_minute = 1.0f; bool deterministic = true; [[nodiscard]] bool valid() const noexcept; };

class GameplayWorld {
public:
    explicit GameplayWorld(GameplayWorldSettings settings = {});
    ~GameplayWorld();
    GameplayWorld(GameplayWorld&&) noexcept;
    GameplayWorld& operator=(GameplayWorld&&) noexcept;
    GameplayWorld(const GameplayWorld&) = delete;
    GameplayWorld& operator=(const GameplayWorld&) = delete;

    [[nodiscard]] const GameplayWorldSettings& settings() const noexcept;
    CharacterId create_character(CharacterDefinition definition = {}, Vec3 position = {});
    bool destroy_character(CharacterId) noexcept;
    [[nodiscard]] CharacterInstance* character(CharacterId) noexcept;
    [[nodiscard]] const CharacterInstance* character(CharacterId) const noexcept;
    [[nodiscard]] std::vector<CharacterId> character_ids() const;
    bool update_character(CharacterId, CharacterControllerInput, float dt) noexcept;
    bool damage(const DamageEvent&, CombatResult* result = nullptr) noexcept;
    bool recover(CharacterId, float amount) noexcept;
    bool set_equipped_item(CharacterId, ItemId) noexcept;
    bool consume(CharacterId, ItemId) noexcept;

    bool define_item(ItemDefinition);
    [[nodiscard]] const ItemDefinition* item(ItemId) const noexcept;
    bool remove_item_definition(ItemId) noexcept;

    InteractionId add_interaction(InteractionTarget);
    bool remove_interaction(InteractionId) noexcept;
    [[nodiscard]] const InteractionTarget* interaction(InteractionId) const noexcept;
    bool interact(CharacterId, InteractionId, InteractionType) noexcept;

    bool set_schedule(CharacterId, NPCSchedule);
    [[nodiscard]] const NPCSchedule* schedule(CharacterId) const noexcept;
    [[nodiscard]] const NPCState* npc_state(CharacterId) const noexcept;

    void update(float dt) noexcept;
    void clear() noexcept;
    [[nodiscard]] std::uint64_t game_minute() const noexcept;
    [[nodiscard]] const std::vector<DamageEvent>& damage_events() const noexcept;

    void bind_physics(PhysicsWorld* physics) noexcept;
    [[nodiscard]] PhysicsWorld* physics() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace exgine
