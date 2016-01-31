#pragma once

#include "vector2D.h"

/// 2D rectangle.
class Rect2D {
//Data
public:
	Vector2D leftTop;
	Vector2D rightBottom;

//Construction
public:
	Rect2D () : leftTop (0, 0), rightBottom (0, 0) {}
	Rect2D (float left, float top, float right, float bottom) : leftTop (left, top), rightBottom (right, bottom) {}
	Rect2D (const Vector2D& leftTop, const Vector2D& rightBottom) : leftTop (leftTop), rightBottom (rightBottom) {}

//Operations
public:
	Rect2D Offset (const Vector2D& pos) const {
		return Rect2D (leftTop + pos, rightBottom + pos);
	}

	Rect2D Scale (const Vector2D& scale) const {
		return Rect2D (leftTop * scale, rightBottom * scale);
	}

	bool Contains (const Vector2D& vec) const {
		return vec >= leftTop && vec <= rightBottom;
	}

	bool Intersects (const Rect2D& rect) const {
		return (leftTop >= rect.leftTop && leftTop <= rect.rightBottom) ||
			(rightBottom >= rect.leftTop && rightBottom <= rect.rightBottom) ||
			(rect.leftTop >= leftTop && rect.leftTop <= rightBottom) ||
			(rect.rightBottom >= leftTop && rect.rightBottom <= rightBottom);
	}

//Data
public:
	float Width () const {
		return rightBottom.x - leftTop.x;
	}

	float Height () const {
		return rightBottom.y - leftTop.y;
	}

	Vector2D Size () const {
		return Vector2D (Width (), Height ());
	}
};

inline static ostream& operator << (ostream& stream, const Rect2D& rect) {
	stream << "{\"leftTop\":" << rect.leftTop << ",\"rightBottom\":" << rect.rightBottom << "}";
	return stream;
}

//TODO: istream JSON beolvasás kezelése a Rect2D-nél
