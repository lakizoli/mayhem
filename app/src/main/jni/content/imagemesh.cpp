#include "../pch.h"
#include "imagemesh.h"

void ImageMesh::Init () {
	Mesh2D::Init ();

	mTex = LoadTextureFromAsset (mAsset);
	mVbo = NewTexturedVBO (mTex);
}

void ImageMesh::Shutdown () {
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

void ImageMesh::RenderMesh () {
	RenderTexturedVBO (mTex, mVbo[0], mVbo[1]);
}
