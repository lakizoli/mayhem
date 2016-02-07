#include "../pch.h"
#include "texanimmesh.h"
#include "color.h"

void TexAnimMesh::Init () {
	Mesh2D::Init ();

	mTex = CreateColoredTexture (mWidth, mHeight, mBPP, Color (0.0f, 0.0f, 0.0f));
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

void TexAnimMesh::SetPixels (int width, int height, int bpp, const uint8_t *pixels) {
	assert (mWidth == width && mHeight == height && (bpp == 24 || bpp == 32) && pixels != nullptr);

	GLint format = bpp == 24 ? GL_RGB : GL_RGBA;
	glBindTexture (GL_TEXTURE_2D, mTex);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, format, GL_UNSIGNED_BYTE, pixels);
}

void TexAnimMesh::RenderMesh () {
	RenderTexturedVBO (mTex, mVbo[0], mVbo[1]);
}
