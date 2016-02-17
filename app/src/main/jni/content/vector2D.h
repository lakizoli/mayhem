#pragma once

class Vector2D {
//Data
public:
	float x, y;

//Construction
public:
	Vector2D () : x (0), y (0) {}
	Vector2D (float x, float y) : x (x) , y (y) {}
	Vector2D (const Vector2D& vec) : x (vec.x), y (vec.y) {}
	Vector2D (Vector2D&& vec) : x (move (vec.x)), y (move (vec.y)) {}

//Arithmetic Operators
public:
	Vector2D operator +(const Vector2D& vec) const {
		return Vector2D (x + vec.x, y + vec.y);
	}

	Vector2D operator -(const Vector2D& vec) const {
		return Vector2D (x - vec.x, y - vec.y);
	}

	Vector2D operator *(const Vector2D& multiplier) const {
		return Vector2D (x * multiplier.x, y * multiplier.y);
	}

	Vector2D operator *(float multiplier) const {
		return Vector2D (x * multiplier, y * multiplier);
	}

	Vector2D operator /(const Vector2D& divider) const {
		return Vector2D (x / divider.x, y / divider.y);
	}

	Vector2D operator /(float divider) const {
		return Vector2D (x / divider, y / divider);
	}

	Vector2D operator %(const Vector2D& divider) const {
		return Vector2D (fmodf (x, divider.x), fmodf (y, divider.y));
	}

	Vector2D operator %(float divider) const {
		return Vector2D (fmodf (x, divider), fmodf (y, divider));
	}

//Assignment operators
public:
	Vector2D& operator = (const Vector2D& vec) {
		x = vec.x;
		y = vec.y;
		return *this;
	}

	Vector2D& operator = (Vector2D&& vec) {
		x = move (vec.x);
		y = move (vec.y);
		return *this;
	}

	Vector2D& operator += (const Vector2D& vec) {
		x += vec.x;
		y += vec.y;
		return *this;
	}

	Vector2D& operator -=(const Vector2D& vec) {
		x -= vec.x;
		y -= vec.y;
		return *this;
	}

	Vector2D& operator *=(const Vector2D& multiplier) {
		x *= multiplier.x;
		y *= multiplier.y;
		return *this;
	}

	Vector2D& operator *=(float multiplier) {
		x *= multiplier;
		y *= multiplier;
		return *this;
	}

	Vector2D& operator /=(const Vector2D& divider) {
		x /= divider.x;
		y /= divider.y;
		return *this;
	}

	Vector2D& operator /=(float divider) {
		x /= divider;
		y /= divider;
		return *this;
	}

	Vector2D& operator %=(const Vector2D& divider) {
		x = fmodf (x, divider.x);
		y = fmodf (y, divider.y);
		return *this;
	}

	Vector2D& operator %=(float divider) {
		x = fmodf (x, divider);
		y = fmodf (y, divider);
		return *this;
	}

//Comparison operators
public:
	bool operator >(const Vector2D& vec) const {
		return x > vec.x && y > vec.y;
	}

	bool operator <(const Vector2D& vec) const {
		return x < vec.x && y < vec.y;
	}

	bool operator >=(const Vector2D& vec) const {
		return *this == vec || (x > vec.x && y > vec.y);
	}

	bool operator <=(const Vector2D& vec) const {
		return *this == vec || (x < vec.x && y < vec.y);
	}

	bool operator == (const Vector2D& vec) const {
		// If they are equal anyway, just return True.
		if (x == vec.x && y == vec.y)
			return true;

		// Handle NaN, Infinity.
		if (isnan (x) || isnan (vec.x))
			return isnan (x) == isnan (vec.x);
		else if (isinf (x) || isinf (vec.x))
			return isinf (x) == isinf (vec.x);
		else if (isnan (y) || isnan (vec.y))
			return isnan (y) == isnan (vec.y);
		else if (isinf (y) || isinf (vec.y))
			return isinf (y) == isinf (vec.y);

		// Handle zero to avoid division by zero
		return fabsf (x - vec.x) <= 0.00001f && fabsf (y - vec.y) <= 0.00001f;
	}

	bool operator !=(const Vector2D& vec) const {
		return !(*this == vec);
	}

//Operations
public:
	float SquareLength () const {
		return x * x + y * y;
	}

	float Length () const {
		return sqrtf (SquareLength ());
	}

	static float Dot (const Vector2D& v1, const Vector2D& v2) {
		return v1.x * v2.x + v1.y * v2.y;
	}

	float Dot (const Vector2D& vec) const {
		return x * vec.x + y * vec.y;
	}

	static float Cross (const Vector2D& v1, const Vector2D& v2) {
		return v1.x * v2.y - v1.y * v2.x;
	}

	float Cross (const Vector2D& vec) const {
		return x * vec.y - y * vec.x;
	}

	static Vector2D Normalize (const Vector2D& vec) {
		return vec / vec.Length ();
	}

	Vector2D Normalize () const {
		return *this / Length ();
	}
};

inline static Vector2D operator *(float value, const Vector2D& multiplier) {
	return Vector2D (value * multiplier.x, value * multiplier.y);
}

inline static Vector2D operator /(float value, const Vector2D& divider) {
	return Vector2D (value / divider.x, value / divider.y);
}

inline static Vector2D operator %(float value, const Vector2D& divider) {
	return Vector2D (fmodf (value, divider.x), fmodf (value, divider.y));
}

inline static ostream& operator << (ostream& stream, const Vector2D& vec) {
	stream << "{\"x\":" << vec.x << ",\"y\":" << vec.y << "}";
	return stream;
}

//TODO: istream JSON beolvas�s kezel�se a Vector2D-n�l
