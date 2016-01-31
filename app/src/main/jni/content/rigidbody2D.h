#pragma once

#include "vector2D.h"
#include "geom.h"
#include "mesh2D.h"

class RigidBody2D;
typedef function<shared_ptr<RigidBody2D> (const RigidBody2D*)> fn_find_collision;

/// Class for implement basic rigid body physics.
class RigidBody2D {
public:
    constexpr static const float PhysicalScale = 30.0f; ///< Physical scale [pixel / meter]
	constexpr static const float Gravity = 9.81f; ///< [m/sec^2]

	shared_ptr<Mesh2D> Mesh;
	Vector2D LastPos;
	Vector2D Velocity;
	float Mass;
	Vector2D Force;

	shared_ptr<RigidBody2D> CollideBody;

public:
	RigidBody2D () : Mass (0) {}

	void Update (float elapsedTime, fn_find_collision findCollision);
};
