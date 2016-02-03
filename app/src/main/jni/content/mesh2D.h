#pragma once

#include "vector2D.h"
#include "rect2D.h"

/// Base class for 2D meshes.
class Mesh2D : public enable_shared_from_this<Mesh2D> {
//Data
public:
	Vector2D Pos;
	float Rotation;
	Vector2D Scale;

	Rect2D boundingBox;

//Construction
protected:
	Mesh2D () : Rotation (0.0f) {}

//Interface
public:
	virtual void Init ();
	virtual void Shutdown ();
	void Render ();

protected:
    virtual void RenderMesh () = 0;

//Helper methods
public:
	Rect2D TransformedBoundingBox () const {
		return boundingBox.Scale (Scale).Offset (Pos);
	}

protected:
	GLuint CreateTexture (int width, int height, int bpp) const;
	GLuint CreateColoredTexture (int width, int height, int bpp, float red, float green, float blue, float alpha = 1.0f) const;
	GLuint LoadTextureFromAsset (const string& asset) const;

	Rect2D CalculateBoundingBox (const vector<float>& vertices) const;

	GLuint NewVBO (const vector<float>& data) const;
	vector<GLuint> NewTexturedVBO (GLuint texID, const vector<float>& vertices = vector<float> (), const vector<float>& texCoords = vector<float> ());

	void RenderTexturedVBO (GLuint texID, GLuint vertCoordID, GLuint texCoordID, GLenum mode = GL_TRIANGLE_STRIP, int vertexCount = 4) const;
};
