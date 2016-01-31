#include "../pch.h"
#include "geom.h"

vector<Vector2D> Geom::LineCircleIntersection (const Vector2D& dir, const Vector2D& pt, const Vector2D& origin, float radius) {
	if (dir.x == 0.0f) {
		vector<Vector2D>&& res = Geom::LineCircleIntersection (Vector2D (dir.y, dir.x), Vector2D (pt.y, pt.x), Vector2D (origin.y, origin.x), radius);
		if (res.size () > 0)
			swap (res[0].x, res[0].y);

		if (res.size () > 1)
			swap (res[1].x, res[1].y);

		return res;
	}

	if (dir.x == 0.0f)
		return vector<Vector2D> ();

	float t1 = dir.x * dir.x;
	float t3 = dir.x * dir.y;
	float t6 = dir.y * dir.y;
	if (t1 + t6 == 0.0f)
		return vector<Vector2D> ();

	float t8 = t1 * t1;
	float t9 = origin.y * origin.y;
	float t14 = pt.y * pt.y;
	float t16 = radius * radius;
	float t19 = t1 * dir.x * dir.y;
	float t32 = t1 * t6;
	float t33 = origin.x * origin.x;
	float t38 = pt.x * pt.x;
	float t41 = 2 * t19 * origin.x * origin.y + 2 * t32 * origin.x * pt.x - 2 * t19 * origin.x * pt.y - 2 * t19 * origin.y * pt.x + 2 * t8 * origin.y * pt.y + 2 * t19 * pt.x * pt.y - t8 * t14 + t32 * t16 + t8 * t16 - t32 * t33 - t32 * t38 - t8 * t9;
	if (t41 < 0.0f)
		return vector<Vector2D> ();

	float t42 = sqrtf (t41);

	float x1 = (t1 * origin.x + t3 * origin.y + t6 * pt.x - t3 * pt.y + t42) / (t1 + t6);
	float x2 = -(-(t1 * origin.x) - t3 * origin.y - t6 * pt.x + t3 * pt.y + t42) / (t1 + t6);

	float y1 = t6 = (dir.x * pt.y - dir.y * pt.x + dir.y * x1) / dir.x;
	float y2 = t6 = (dir.x * pt.y - dir.y * pt.x + dir.y * x2) / dir.x;

	return{ Vector2D (x1, y1), Vector2D (x2, y2) };
}