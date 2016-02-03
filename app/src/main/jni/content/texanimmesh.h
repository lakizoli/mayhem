#pragma once

#include "mesh2D.h"

class TexAnimMesh : public Mesh2D {
private:
	int mWidth;
	int mHeight;
	const uint8_t* mPixels;
	bool mDirty;

	GLuint mTex;
	vector<GLuint> mVbo;

public:
	TexAnimMesh (int width, int height) : mWidth (width), mHeight (height), mPixels (nullptr), mDirty (false) {}

	virtual void Init () override;
	virtual void Shutdown () override;

	void SetPixels (const uint8_t* pixels) { mPixels = pixels; }
	const uint8_t* GetPixels () const { return mPixels; }

	void SetDirty (bool dirty) { mDirty = dirty; }
	bool IsDirty () const { return mDirty; }

protected:
	virtual void RenderMesh () override;
};
