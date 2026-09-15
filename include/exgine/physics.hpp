#pragma once

#include "exgine/geometry.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace exgine {

enum class PhysicsBodyType : std::uint8_t { Static, Dynamic, Kinematic };
enum class PhysicsMotionQuality : std::uint8_t { Discrete, LinearCast, Continuous, ContinuousAndAngular };
enum class PhysicsShapeType : std::uint8_t { Sphere, Box, Capsule, Cylinder, ConvexHull, TriangleMesh, HeightField, Compound };
enum class PhysicsConstraintType : std::uint8_t { Fixed, BallSocket, Hinge, Slider, Distance, Spring, ConeTwist, SixDegreesOfFreedom };

enum class PhysicsBodyFlag : std::uint32_t {
    None = 0,
    AllowSleep = 1U << 0U,
    StartAwake = 1U << 1U,
    EnableGravity = 1U << 2U,
    EnableCCD = 1U << 3U,
    KinematicFollowsTransform = 1U << 4U
};

using PhysicsBodyFlags = std::uint32_t;
using PhysicsLayer = std::uint32_t;
using PhysicsMask = std::uint32_t;
using PhysicsBodyId = std::uint64_t;
using PhysicsColliderId = std::uint64_t;
using PhysicsConstraintId = std::uint64_t;
inline constexpr PhysicsBodyId invalid_physics_body = 0;
inline constexpr PhysicsColliderId invalid_physics_collider = 0;
inline constexpr PhysicsConstraintId invalid_physics_constraint = 0;

struct PhysicsTransform { Vec3 position{}; Vec3 rotation{}; };
struct PhysicsAabb {
    Vec3 min{}; Vec3 max{};
    [[nodiscard]] bool valid() const noexcept { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
};

struct PhysicsMaterial {
    float density = 1000.0f;
    float static_friction = 0.6f;
    float dynamic_friction = 0.5f;
    float restitution = 0.05f;
    float rolling_friction = 0.01f;
    float spinning_friction = 0.01f;
    [[nodiscard]] bool valid() const noexcept;
};

struct PhysicsSphereShape { float radius = 0.5f; };
struct PhysicsBoxShape { Vec3 half_extents{0.5f, 0.5f, 0.5f}; };
struct PhysicsCapsuleShape { float radius = 0.25f; float half_height = 0.5f; };
struct PhysicsCylinderShape { float radius = 0.5f; float half_height = 0.5f; };
struct PhysicsConvexHullShape { std::shared_ptr<const std::vector<Vec3>> points; };
struct PhysicsTriangleMeshShape { std::shared_ptr<const Mesh> mesh; bool double_sided = false; };
struct PhysicsHeightFieldShape { std::uint32_t width=0, depth=0; float cell_size=1.0f; std::shared_ptr<const std::vector<float>> heights; };
struct PhysicsCompoundShape;
struct PhysicsShape;
struct PhysicsCompoundChild { PhysicsTransform local_transform{}; std::shared_ptr<const PhysicsShape> shape; };
struct PhysicsCompoundShape { std::shared_ptr<const std::vector<PhysicsCompoundChild>> children; };

struct PhysicsShape {
    PhysicsShapeType type = PhysicsShapeType::Box;
    std::variant<PhysicsSphereShape, PhysicsBoxShape, PhysicsCapsuleShape, PhysicsCylinderShape,
                 PhysicsConvexHullShape, PhysicsTriangleMeshShape, PhysicsHeightFieldShape,
                 PhysicsCompoundShape> data = PhysicsBoxShape{};
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] PhysicsAabb local_bounds() const noexcept;
};

struct PhysicsMassProperties {
    float mass = 1.0f;
    Vec3 center_of_mass{};
    Vec3 diagonal_inertia{1.0f, 1.0f, 1.0f};
    [[nodiscard]] bool valid() const noexcept;
};

struct PhysicsRigidBodyDesc {
    PhysicsBodyType type = PhysicsBodyType::Dynamic;
    PhysicsTransform transform{};
    Vec3 linear_velocity{};
    Vec3 angular_velocity{};
    Vec3 linear_damping{0.01f, 0.01f, 0.01f};
    Vec3 angular_damping{0.01f, 0.01f, 0.01f};
    Vec3 gravity_scale{1.0f, 1.0f, 1.0f};
    PhysicsMassProperties mass_properties{};
    PhysicsMotionQuality motion_quality = PhysicsMotionQuality::Discrete;
    PhysicsBodyFlags flags = static_cast<PhysicsBodyFlags>(PhysicsBodyFlag::AllowSleep) |
                              static_cast<PhysicsBodyFlags>(PhysicsBodyFlag::StartAwake) |
                              static_cast<PhysicsBodyFlags>(PhysicsBodyFlag::EnableGravity);
    float max_linear_speed = 500.0f;
    float max_angular_speed = 100.0f;
    [[nodiscard]] bool valid() const noexcept;
};

struct PhysicsColliderDesc {
    PhysicsShape shape{};
    PhysicsTransform local_transform{};
    PhysicsMaterial material{};
    PhysicsLayer layer = 1U;
    PhysicsMask mask = std::numeric_limits<PhysicsMask>::max();
    bool sensor = false;
    bool enabled = true;
    float contact_offset = 0.01f;
    float rest_offset = 0.0f;
    [[nodiscard]] bool valid() const noexcept;
};

struct PhysicsContactPoint {
    Vec3 position{};
    Vec3 normal{};
    float penetration = 0.0f;
    float normal_impulse = 0.0f;
    float tangent_impulse_0 = 0.0f;
    float tangent_impulse_1 = 0.0f;
};
struct PhysicsContactManifold {
    PhysicsBodyId body_a=invalid_physics_body, body_b=invalid_physics_body;
    PhysicsColliderId collider_a=invalid_physics_collider, collider_b=invalid_physics_collider;
    std::array<PhysicsContactPoint,4> points{};
    std::uint8_t point_count=0;
    bool sensor=false;
    [[nodiscard]] bool valid() const noexcept;
};
struct PhysicsContactEvent { enum class Type : std::uint8_t { Begin, Persist, End }; Type type=Type::Begin; PhysicsContactManifold manifold{}; };

struct PhysicsConstraintLimits { bool enabled=false; float lower=0.0f; float upper=0.0f; float softness=1.0f; float restitution=0.0f; float damping=0.0f; };
struct PhysicsConstraintMotor { bool enabled=false; float target_velocity=0.0f; float max_force=0.0f; float max_torque=0.0f; };
struct PhysicsConstraintDesc {
    PhysicsConstraintType type=PhysicsConstraintType::Fixed;
    PhysicsBodyId body_a=invalid_physics_body, body_b=invalid_physics_body;
    PhysicsTransform frame_a{}, frame_b{};
    PhysicsConstraintLimits linear_limits[3]{};
    PhysicsConstraintLimits angular_limits[3]{};
    PhysicsConstraintMotor motor{};
    float stiffness=0.0f, damping=0.0f;
    float break_force=std::numeric_limits<float>::infinity();
    float break_torque=std::numeric_limits<float>::infinity();
    bool collide_connected=false;
    [[nodiscard]] bool valid() const noexcept;
};

struct PhysicsRay { Vec3 origin{}; Vec3 direction{0,0,1}; float max_distance=1000.0f; };
struct PhysicsShapeCast { PhysicsShape shape{}; PhysicsTransform start{}; Vec3 translation{}; };
struct PhysicsQueryFilter {
    PhysicsLayer layer=1U;
    PhysicsMask mask=std::numeric_limits<PhysicsMask>::max();
    PhysicsBodyId ignore_body=invalid_physics_body;
    PhysicsColliderId ignore_collider=invalid_physics_collider;
    bool include_sensors=false;
};
struct PhysicsQueryHit {
    PhysicsBodyId body=invalid_physics_body;
    PhysicsColliderId collider=invalid_physics_collider;
    float fraction=0.0f, distance=0.0f;
    Vec3 position{}, normal{};
    [[nodiscard]] bool valid() const noexcept;
};

struct PhysicsWorldSettings {
    Vec3 gravity{0.0f,-9.80665f,0.0f};
    float fixed_timestep=1.0f/120.0f;
    std::uint32_t max_substeps=8;
    std::uint32_t velocity_iterations=8;
    std::uint32_t position_iterations=3;
    std::uint32_t velocity_iterations_toi=12;
    float max_accumulated_time=0.25f;
    float sleep_linear_threshold=0.05f;
    float sleep_angular_threshold=0.05f;
    float time_to_sleep=0.5f;
    float allowed_penetration=0.005f;
    float baumgarte=0.2f;
    bool deterministic=true;
    bool parallel_broadphase=true;
    [[nodiscard]] bool valid() const noexcept;
};
struct PhysicsStepResult { std::uint32_t substeps=0; float interpolation_alpha=0.0f; std::uint64_t simulation_step=0; };
struct PhysicsWorldStats { std::uint64_t bodies=0, colliders=0, constraints=0, broadphase_pairs=0, contacts=0, awake_bodies=0; };
struct PhysicsCallbacks {
    std::function<void(const PhysicsContactEvent&)> on_contact;
    std::function<void(PhysicsBodyId)> on_body_wake;
    std::function<void(PhysicsBodyId)> on_body_sleep;
    std::function<void(PhysicsConstraintId)> on_constraint_broken;
};

class PhysicsWorld {
public:
    explicit PhysicsWorld(PhysicsWorldSettings settings={});
    ~PhysicsWorld();
    PhysicsWorld(PhysicsWorld&&) noexcept;
    PhysicsWorld& operator=(PhysicsWorld&&) noexcept;
    PhysicsWorld(const PhysicsWorld&)=delete;
    PhysicsWorld& operator=(const PhysicsWorld&)=delete;

    [[nodiscard]] const PhysicsWorldSettings& settings() const noexcept;
    bool set_gravity(Vec3 gravity) noexcept;

    PhysicsBodyId create_body(const PhysicsRigidBodyDesc& desc);
    bool destroy_body(PhysicsBodyId id) noexcept;
    [[nodiscard]] bool has_body(PhysicsBodyId id) const noexcept;
    [[nodiscard]] std::optional<PhysicsTransform> body_transform(PhysicsBodyId id) const noexcept;
    bool set_body_transform(PhysicsBodyId id, PhysicsTransform transform, bool wake=true) noexcept;
    bool set_linear_velocity(PhysicsBodyId id, Vec3 velocity, bool wake=true) noexcept;
    bool set_angular_velocity(PhysicsBodyId id, Vec3 velocity, bool wake=true) noexcept;
    bool apply_force(PhysicsBodyId id, Vec3 force, Vec3 world_position) noexcept;
    bool apply_torque(PhysicsBodyId id, Vec3 torque) noexcept;
    bool wake_body(PhysicsBodyId id) noexcept;
    bool sleep_body(PhysicsBodyId id) noexcept;
    [[nodiscard]] bool is_awake(PhysicsBodyId id) const noexcept;

    PhysicsColliderId add_collider(PhysicsBodyId body, const PhysicsColliderDesc& desc);
    bool remove_collider(PhysicsColliderId id) noexcept;
    [[nodiscard]] bool has_collider(PhysicsColliderId id) const noexcept;
    bool set_collider_enabled(PhysicsColliderId id, bool enabled) noexcept;

    PhysicsConstraintId add_constraint(const PhysicsConstraintDesc& desc);
    bool remove_constraint(PhysicsConstraintId id) noexcept;
    [[nodiscard]] bool has_constraint(PhysicsConstraintId id) const noexcept;
    bool set_constraint_motor(PhysicsConstraintId id, PhysicsConstraintMotor motor) noexcept;

    [[nodiscard]] PhysicsStepResult step(float real_dt) noexcept;
    [[nodiscard]] float interpolation_alpha() const noexcept;
    [[nodiscard]] PhysicsWorldStats stats() const noexcept;

    [[nodiscard]] std::vector<PhysicsQueryHit> raycast(const PhysicsRay& ray, const PhysicsQueryFilter& filter={}) const;
    [[nodiscard]] std::vector<PhysicsQueryHit> shapecast(const PhysicsShapeCast& cast, const PhysicsQueryFilter& filter={}) const;
    [[nodiscard]] std::vector<PhysicsQueryHit> overlap(const PhysicsShape& shape, const PhysicsTransform& transform, const PhysicsQueryFilter& filter={}) const;

    void set_callbacks(PhysicsCallbacks callbacks);
    void clear_callbacks();
    void clear() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace exgine
