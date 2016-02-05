#pragma once

#include "mesh2D.h"
#include "color.h"

class ColoredMesh : public Mesh2D {
private:
	int mWidth;
	int mHeight;
	int mBPP;

	Color mColor;

	GLuint mTex;
	vector<GLuint> mVbo;

public:
	ColoredMesh (int width, int height, int bpp, const Color& color) : mWidth (width), mHeight (height), mBPP (bpp), mColor (color) {}

	virtual void Init () override;
	virtual void Shutdown () override;

	int GetWidth () const { return mWidth; }
	int GetHeight () const { return mHeight; }
	int GetBPP () const { return mBPP; }
	const Color& GetColor () const { return mColor; }

protected:
	virtual void RenderMesh () override;
};
