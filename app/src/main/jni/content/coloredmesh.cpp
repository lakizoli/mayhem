#include "../pch.h"
#include "coloredmesh.h"

void ColoredMesh::Init () {
	Mesh2D::Init ();

	mTex = CreateColoredTexture (mWidth, mHeight, mBPP, mColor);
	mVbo = NewTexturedVBO (mTex);
}

void ColoredMesh::Shutdown () {
	if (mVbo.size () > 0) {
		glDeleteBuffers (mVbo.size (), &mVbo[0]);
		mVbo.clear ();
	}

	if (mTex > 0) {
		glDeleteTextures (1, &mTex);
		mTex = 0;
	}

	Mesh2D::Shutdown ();
}

void ColoredMesh::RenderMesh () {
	RenderTexturedVBO (mTex, mVbo[0], mVbo[1]);
}
