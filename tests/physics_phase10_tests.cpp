#include "exgine/physics.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
using namespace exgine;

PhysicsColliderDesc make_box(float x=0.5f, float y=0.5f, float z=0.5f) {
    PhysicsColliderDesc c;
    c.shape.type = PhysicsShapeType::Box;
    c.shape.data = PhysicsBoxShape{{x,y,z}};
    c.material.static_friction = 0.8f;
    c.material.dynamic_friction = 0.6f;
    c.material.restitution = 0.0f;
    return c;
}

void test_fixed_step_and_gravity() {
    PhysicsWorldSettings s;
    s.fixed_timestep = 1.0f / 120.0f;
    s.max_substeps = 4;
    s.max_accumulated_time = 0.05f;
    PhysicsWorld w(s);

    PhysicsRigidBodyDesc d;
    d.transform.position = {0, 10, 0};
    const auto id = w.create_body(d);
    assert(id != invalid_physics_body);
    assert(w.add_collider(id, make_box()) != invalid_physics_collider);

    const auto before = w.body_transform(id);
    assert(before);
    const auto step = w.step(1.0f);
    assert(step.substeps == s.max_substeps);
    assert(step.interpolation_alpha >= 0.0f && step.interpolation_alpha < 1.0f);
    const auto after = w.body_transform(id);
    assert(after && after->position.y < before->position.y);
}

void test_collision_and_contacts() {
    PhysicsWorldSettings s;
    s.fixed_timestep = 1.0f / 120.0f;
    PhysicsWorld w(s);

    PhysicsRigidBodyDesc floor_desc;
    floor_desc.type = PhysicsBodyType::Static;
    floor_desc.transform.position = {0,-0.5f,0};
    const auto floor = w.create_body(floor_desc);
    assert(w.add_collider(floor, make_box(5,0.5f,5)));

    PhysicsRigidBodyDesc ball_desc;
    ball_desc.transform.position = {0,2,0};
    const auto ball = w.create_body(ball_desc);
    assert(w.add_collider(ball, make_box()));

    std::uint32_t begins = 0;
    std::uint32_t persists = 0;
    w.set_callbacks({[&](const PhysicsContactEvent& e) {
        if (e.type == PhysicsContactEvent::Type::Begin) ++begins;
        if (e.type == PhysicsContactEvent::Type::Persist) ++persists;
    }, {}, {}, {}});

    for (int i=0; i<300; ++i) w.step(1.0f/120.0f);
    const auto t = w.body_transform(ball);
    assert(t && t->position.y > -0.1f && t->position.y < 1.0f);
    assert(begins > 0);
    assert(persists > 0);
    assert(w.stats().contacts > 0);
}

void test_sleep_wake_and_forces() {
    PhysicsWorldSettings s;
    s.fixed_timestep = 1.0f/120.0f;
    s.time_to_sleep = 0.05f;
    PhysicsWorld w(s);
    PhysicsRigidBodyDesc d;
    d.flags = static_cast<PhysicsBodyFlags>(PhysicsBodyFlag::AllowSleep);
    d.transform.position = {0,0,0};
    const auto id = w.create_body(d);
    assert(id);
    assert(w.add_collider(id, make_box()));
    for (int i=0;i<30;++i) w.step(1.0f/120.0f);
    assert(!w.is_awake(id));
    assert(w.wake_body(id));
    assert(w.is_awake(id));
    assert(w.apply_force(id,{0,100,0},{0,0,0}));
    w.step(1.0f/120.0f);
    const auto v = w.body_transform(id);
    assert(v && v->position.y > 0);
}

void test_queries_and_filters() {
    PhysicsWorld w;
    PhysicsRigidBodyDesc d;
    d.type = PhysicsBodyType::Static;
    d.transform.position = {0,0,5};
    const auto body = w.create_body(d);
    auto c = make_box();
    c.layer = 2U;
    c.mask = 4U;
    const auto collider = w.add_collider(body,c);
    assert(collider);

    PhysicsQueryFilter wrong;
    wrong.layer = 1U;
    wrong.mask = 1U;
    assert(w.raycast({{0,0,0},{0,0,1},20},wrong).empty());

    PhysicsQueryFilter right;
    right.layer = 4U;
    right.mask = 2U;
    const auto hits = w.raycast({{0,0,0},{0,0,1},20},right);
    assert(!hits.empty() && hits.front().valid());
    assert(hits.front().collider == collider);

    const auto overlaps = w.overlap(PhysicsShape{PhysicsShapeType::Sphere,PhysicsSphereShape{2}},{{0,0,5},{0,0,0}},right);
    assert(!overlaps.empty());
    const auto casts = w.shapecast({PhysicsShape{PhysicsShapeType::Sphere,PhysicsSphereShape{.5f}},{{0,0,0},{0,0,0}},{0,0,10}},right);
    assert(!casts.empty());
}

void test_shape_contracts() {
    PhysicsShape convex;
    convex.type = PhysicsShapeType::ConvexHull;
    convex.data = PhysicsConvexHullShape{std::make_shared<const std::vector<Vec3>>(std::vector<Vec3>{{-1,0,0},{1,0,0},{0,1,0},{0,0,1}})};
    assert(convex.valid() && convex.local_bounds().valid());

    auto mesh = std::make_shared<Mesh>(make_box({1,1,1}));
    PhysicsShape triangle;
    triangle.type = PhysicsShapeType::TriangleMesh;
    triangle.data = PhysicsTriangleMeshShape{mesh,false};
    assert(triangle.valid() && triangle.local_bounds().valid());

    PhysicsShape height;
    height.type = PhysicsShapeType::HeightField;
    height.data = PhysicsHeightFieldShape{3,3,1.0f,std::make_shared<const std::vector<float>>(std::vector<float>{0,0,0,0,1,0,0,0,0})};
    assert(height.valid() && height.local_bounds().valid());

    auto child = std::make_shared<const PhysicsShape>(PhysicsShape{PhysicsShapeType::Sphere,PhysicsSphereShape{1}});
    PhysicsShape compound;
    compound.type = PhysicsShapeType::Compound;
    compound.data = PhysicsCompoundShape{std::make_shared<const std::vector<PhysicsCompoundChild>>(std::vector<PhysicsCompoundChild>{{{{2,0,0},{0,0,0}},child}})};
    assert(compound.valid() && compound.local_bounds().valid());
}

void test_constraints_and_cleanup() {
    PhysicsWorld w;
    PhysicsRigidBodyDesc a_desc;
    a_desc.type = PhysicsBodyType::Static;
    const auto a = w.create_body(a_desc);
    PhysicsRigidBodyDesc b_desc;
    b_desc.transform.position = {2,0,0};
    const auto b = w.create_body(b_desc);
    assert(a && b);
    PhysicsConstraintDesc d;
    d.type = PhysicsConstraintType::Distance;
    d.body_a = a;
    d.body_b = b;
    d.linear_limits[0].enabled = true;
    d.linear_limits[0].lower = 2;
    d.linear_limits[0].upper = 2;
    d.stiffness = 1;
    const auto constraint = w.add_constraint(d);
    assert(constraint && w.has_constraint(constraint));
    PhysicsConstraintMotor motor;
    motor.enabled = true;
    motor.target_velocity = 2;
    motor.max_force = 10;
    motor.max_torque = 10;
    assert(w.set_constraint_motor(constraint,motor));
    assert(w.destroy_body(b));
    assert(!w.has_body(b) && !w.has_constraint(constraint));
}

void test_determinism() {
    PhysicsWorldSettings s;
    s.deterministic = true;
    s.fixed_timestep = 1.0f/120.0f;
    PhysicsWorld a(s), b(s);
    PhysicsRigidBodyDesc d;
    d.transform.position = {0,5,0};
    const auto aa = a.create_body(d), bb = b.create_body(d);
    assert(a.add_collider(aa,make_box()) && b.add_collider(bb,make_box()));
    for (int i=0;i<180;++i) { a.step(1.0f/120.0f); b.step(1.0f/120.0f); }
    const auto ta=a.body_transform(aa), tb=b.body_transform(bb);
    assert(ta && tb);
    assert(ta->position.x==tb->position.x && ta->position.y==tb->position.y && ta->position.z==tb->position.z);
    assert(a.stats().simulation_step==b.stats().simulation_step);
}
}

int main() {
    test_fixed_step_and_gravity();
    test_collision_and_contacts();
    test_sleep_wake_and_forces();
    test_queries_and_filters();
    test_shape_contracts();
    test_constraints_and_cleanup();
    test_determinism();
    return 0;
}
