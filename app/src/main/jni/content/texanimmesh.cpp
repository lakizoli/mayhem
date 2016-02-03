#include "../pch.h"
#include "texanimmesh.h"

void TexAnimMesh::Init () {
	Mesh2D::Init ();

	mTex = CreateTexture (mWidth, mHeight);
	mVbo = NewTexturedVBO (mTex);
}

void TexAnimMesh::Shutdown () {
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

void TexAnimMesh::RenderMesh () {
	//Update texture data
	if (mDirty && mPixels) {
		glBindTexture (GL_TEXTURE_2D, mTex);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mPixels);
	}

	//Render VBO
	RenderTexturedVBO (mTex, mVbo[0], mVbo[1]);
}
