#include "exgine/vehicle_dynamics.hpp"
#include <cassert>
using namespace exgine;
int main(){PhysicsWorld world;PhysicsRigidBodyDesc body;body.transform.position={0,1,0};body.mass_properties.mass=1200;auto id=world.create_body(body);assert(id!=invalid_physics_body);VehicleDynamicsConfig c;for(auto&i:c.wheels)i.driven=true;VehicleDynamicsController car(world,c);assert(car.bind_body(id));assert(car.possess(true));assert(car.set_input({1,0,0,false}));assert(car.update(1.0f/60.0f));assert(car.state().speed>0);return 0;}
