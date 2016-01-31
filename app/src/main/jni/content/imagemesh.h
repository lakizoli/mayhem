#pragma once

#include "mesh2D.h"

class ImageMesh : public Mesh2D {
private:
	string mAsset;
	GLuint mTex;
	vector<GLuint> mVbo;

public:
	ImageMesh (const string& asset) : mAsset (asset) {}

	virtual void Init () override;
	virtual void Shutdown () override;

protected:
	virtual void RenderMesh () override;
};
