#pragma once

#include "vector2D.h"

class Geom {
public:
    /// <summary>
    /// Calculate intersection point of a line and a circle.
    /// 
    /// Circle equation: (x - u)^2 + (y - v)^2 = r^2, where [u,v] is the origin of the circle, and r is the radius of the circle
    /// Line equation: v2 * x - v1 * y = v2 * x0 - v1 * y0, where [v1, v2] is the normalized direction vector of the line, and [x0,y0] is a point of the line
    /// </summary>
    /// <param name="dir">The normalized direction vector of the line.</param>
    /// <param name="pt">On point of the line.</param>
    /// <param name="origin">The origin of the circle.</param>
    /// <param name="radius">The radius of the circle.</param>
    /// <returns>empty vector, when no intersection found.</returns>
	static vector<Vector2D> LineCircleIntersection (const Vector2D& dir, const Vector2D& pt, const Vector2D& origin, float radius);
};
