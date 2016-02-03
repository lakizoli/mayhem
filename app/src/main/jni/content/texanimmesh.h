#pragma once

#include "mesh2D.h"

class TexAnimMesh : public Mesh2D {
private:
	int mWidth;
	int mHeight;
	int mBPP;

	GLuint mTex;
	vector<GLuint> mVbo;

public:
	TexAnimMesh (int width, int height, int bpp) : mWidth (width), mHeight (height), mBPP (bpp) {}

	virtual void Init () override;
	virtual void Shutdown () override;

	int GetWidth () const { return mWidth; }
	int GetHeight () const { return mHeight; }
	int GetBPP () const { return mBPP; }

	void SetPixels (int width, int height, int bpp, const uint8_t* pixels);

protected:
	virtual void RenderMesh () override;
};
