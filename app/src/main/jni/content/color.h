#pragma once

class Color {
//Data
public:
	float r, g, b, a;

//Construction
public:
	Color () : r (0), g (0), b (0), a (0) {}
	Color (uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = (uint8_t)255) : r (255.0f / (float)red) , g (255.0f / (float)green), b (255.0f / (float)blue), a (255.0f / (float)alpha) {}
	Color (float red, float green, float blue, float alpha = 1.0f) : r (red) , g (green), b (blue), a (alpha) {}
	Color (const Color& col) : r (col.r), g (col.g), b (col.b), a (col.a) {}
	Color (Color&& col) : r (move (col.r)), g (move (col.g)), b (move (col.b)), a (move (col.a)) {}

	static Color CreateFromRGBA (uint32_t rgba) {
		return Color (
			  255.0f / (float)((rgba & 0xFF000000) >> 24)
			, 255.0f / (float)((rgba & 0x00FF0000) >> 16)
			, 255.0f / (float)((rgba & 0x0000FF00) >> 8)
			, 255.0f / (float) (rgba & 0x000000FF)
		);
	}

	static Color CreateFromRGB (uint32_t rgb) {
		return Color (
			  255.0f / (float)((rgb & 0xFF000000) >> 24)
			, 255.0f / (float)((rgb & 0x00FF0000) >> 16)
			, 255.0f / (float)((rgb & 0x0000FF00) >> 8)
			, 1.0f
		);
	}

	static Color CreateFromBGRA (uint32_t bgra) {
		return Color (
			  255.0f / (float)((bgra & 0x0000FF00) >> 8)
			, 255.0f / (float)((bgra & 0x00FF0000) >> 16)
			, 255.0f / (float)((bgra & 0xFF000000) >> 24)
			, 255.0f / (float) (bgra & 0x000000FF)
		);
	}

	static Color CreateFromBGR (uint32_t bgr) {
		return Color (
			  255.0f / (float)((bgr & 0x0000FF00) >> 8)
			, 255.0f / (float)((bgr & 0x00FF0000) >> 16)
			, 255.0f / (float)((bgr & 0xFF000000) >> 24)
			, 1.0f
		);
	}

//Pack Interface
public:
	uint8_t RedByte () const {
		return (uint8_t) (r * 255.0f + 0.5f);
	}

	uint8_t GreenByte () const {
		return (uint8_t) (g * 255.0f + 0.5f);
	}

	uint8_t BlueByte () const {
		return (uint8_t) (b * 255.0f + 0.5f);
	}

	uint8_t AlphaByte () const {
		return (uint8_t) (a * 255.0f + 0.5f);
	}

	uint32_t ToRGBA () const {
		return (uint32_t) RedByte () << 24 | (uint32_t) GreenByte () << 16 | (uint32_t) BlueByte () << 8 | (uint32_t) AlphaByte ();
	}

	uint32_t ToBGRA () const {
		return (uint32_t) BlueByte () << 24 | (uint32_t) GreenByte () << 16 | (uint32_t) RedByte () << 8 | (uint32_t) AlphaByte ();
	}

//Arithmetic Operators
public:
	Color operator +(const Color& col) const {
		return Color (r + col.r, g + col.g, b + col.b, a + col.a);
	}

	Color operator -(const Color& col) const {
		return Color (r - col.r, g - col.g, b - col.b, a - col.a);
	}

	Color operator *(const Color& multiplier) const {
		return Color (r * multiplier.r, g * multiplier.g, b * multiplier.b, a * multiplier.a);
	}

	Color operator *(float multiplier) const {
		return Color (r * multiplier, g * multiplier, b * multiplier, a * multiplier);
	}

	Color operator /(const Color& divider) const {
		return Color (r / divider.r, g / divider.g, b / divider.b, a / divider.a);
	}

	Color operator /(float divider) const {
		return Color (r / divider, g / divider, b / divider, a / divider);
	}

	Color operator %(const Color& divider) const {
		return Color (fmodf (r, divider.r), fmodf (g, divider.g), fmodf (b, divider.b), fmodf (a, divider.a));
	}

	Color operator %(float divider) const {
		return Color (fmodf (r, divider), fmodf (g, divider), fmodf (b, divider), fmodf (a, divider));
	}

//Assignment operators
public:
	Color& operator = (const Color& col) {
		r = col.r;
		g = col.g;
		b = col.b;
		a = col.a;
		return *this;
	}

	Color& operator = (Color&& col) {
		r = move (col.r);
		g = move (col.g);
		b = move (col.b);
		a = move (col.a);
		return *this;
	}

	Color& operator += (const Color& col) {
		r += col.r;
		g += col.g;
		b += col.b;
		a += col.a;
		return *this;
	}

	Color& operator -=(const Color& col) {
		r -= col.r;
		g -= col.g;
		b -= col.b;
		a -= col.a;
		return *this;
	}

	Color& operator *=(const Color& multiplier) {
		r *= multiplier.r;
		g *= multiplier.g;
		b *= multiplier.b;
		a *= multiplier.a;
		return *this;
	}

	Color& operator *=(float multiplier) {
		r *= multiplier;
		g *= multiplier;
		b *= multiplier;
		a *= multiplier;
		return *this;
	}

	Color& operator /=(const Color& divider) {
		r /= divider.r;
		g /= divider.g;
		b /= divider.b;
		a /= divider.a;
		return *this;
	}

	Color& operator /=(float divider) {
		r /= divider;
		g /= divider;
		b /= divider;
		a /= divider;
		return *this;
	}

	Color& operator %=(const Color& divider) {
		r = fmodf (r, divider.r);
		g = fmodf (g, divider.g);
		b = fmodf (b, divider.b);
		a = fmodf (a, divider.a);
		return *this;
	}

	Color& operator %=(float divider) {
		r = fmodf (r, divider);
		g = fmodf (g, divider);
		b = fmodf (b, divider);
		a = fmodf (a, divider);
		return *this;
	}

//Comparison operators
public:
	bool operator >(const Color& col) const {
		return r > col.r && g > col.g && b > col.b && a > col.a;
	}

	bool operator <(const Color& col) const {
		return r < col.r && g < col.g && b < col.b && a < col.a;
	}

	bool operator >=(const Color& col) const {
		return *this == col || (r > col.r && g > col.g && b > col.b && a > col.a);
	}

	bool operator <=(const Color& col) const {
		return *this == col || (r < col.r && g < col.g && b < col.b && a < col.a);
	}

	bool operator == (const Color& col) const {
		// If they are equal anyway, just return True.
		if (r == col.r && g == col.g && b == col.b && a == col.a)
			return true;

		// Handle NaN, Infinity.
		if (isnan (r) || isnan (col.r))
			return isnan (r) == isnan (col.r);
		else if (isinf (r) || isinf (col.r))
			return isinf (r) == isinf (col.r);
		else if (isnan (g) || isnan (col.g))
			return isnan (g) == isnan (col.g);
		else if (isinf (g) || isinf (col.g))
			return isinf (g) == isinf (col.g);
		else if (isnan (b) || isnan (col.b))
			return isnan (b) == isnan (col.b);
		else if (isinf (b) || isinf (col.b))
			return isinf (b) == isinf (col.b);
		else if (isnan (a) || isnan (col.a))
			return isnan (a) == isnan (col.a);
		else if (isinf (a) || isinf (col.a))
			return isinf (a) == isinf (col.a);

		// Handle zero to avoid division by zero
		return fabsf (r - col.r) <= 0.00001f && fabsf (g - col.g) <= 0.00001f && fabsf (b - col.b) <= 0.00001f && fabsf (a - col.a) <= 0.00001f;
	}

	bool operator !=(const Color& col) const {
		return !(*this == col);
	}
};

inline static Color operator *(float multiplier, const Color& col) {
	return Color (col.r * multiplier, col.g * multiplier, col.b * multiplier, col.a * multiplier);
}

inline static Color operator /(float divider, const Color& col) {
	return Color (divider / col.r, divider / col.g, divider / col.b, divider / col.a);
}

inline static Color operator %(float divider, const Color& col) {
	return Color (fmodf (divider, col.r), fmodf (divider, col.g), fmodf (divider, col.b), fmodf (divider, col.a));
}

inline static ostream& operator << (ostream& stream, const Color& col) {
	stream << "{\"r\":" << col.r << ",\"g\":" << col.g << ",\"b\":" << col.b << ",\"a\":" << col.a << "}";
	return stream;
}