#include "../pch.h"
#include "rigidbody2D.h"

void RigidBody2D::Update (float elapsedTime, fn_find_collision findCollision) {
	//Store last pos
	LastPos = Mesh->Pos;
	CollideBody.reset ();

	//Calculate new position
	Vector2D accel = Force / Mass / PhysicalScale;
	Vector2D add = accel * elapsedTime;
	Mesh->Pos += Velocity * elapsedTime + add / 2.0f;
	Velocity += add;

	//Check collision (and calculate new position)
	if (findCollision != nullptr) {
		CollideBody = findCollision (this);
		if (CollideBody != nullptr) {
			//Find collision position (circle intersection with velocity direction line)
			float radius = Mesh->TransformedBoundingBox ().Width () / 2.0f + CollideBody->Mesh->TransformedBoundingBox ().Width () / 2.0f; //Most egyszerusitunk, mert minden mesh kor...
			vector<Vector2D>&& intersection = Geom::LineCircleIntersection (Velocity.Normalize (), Mesh->Pos, CollideBody->Mesh->Pos, radius);
			if (intersection.size () > 0) {
				float len0 = (intersection[0] - LastPos).SquareLength ();
				float len1 = intersection.size () > 1 ? (intersection[1] - LastPos).SquareLength () : 0;
				Vector2D collisionPos = intersection[len0 < len1 ? 0 : 1];

				//Calculate elapsed time before collision
				float fullLen = (Mesh->Pos - LastPos).Length ();
				float collLen = (collisionPos - LastPos).Length ();
				float percent = collLen / fullLen;
				float partialTime = percent * elapsedTime;
				float remainingTime = elapsedTime - partialTime;
				Mesh->Pos = collisionPos;

				//Calculate new velocity and position after collision (until remaining time)
				Vector2D dist = CollideBody->Mesh->Pos - Mesh->Pos;
				Vector2D norm = dist.Normalize ();
				Vector2D proj = Velocity.Dot (norm) * norm;
				Velocity = (2.0f * (Velocity - proj)) - Velocity;
				Mesh->Pos += Velocity * remainingTime;
			}
		}
	}

	//Clear all forces
	Force = Vector2D ();
}
