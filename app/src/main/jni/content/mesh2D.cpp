#include "../pch.h"
#include "mesh2D.h"
#include "../management/game.h"

void Mesh2D::Init ()
{
    Scale = Vector2D (1, 1);
}

void Mesh2D::Shutdown ()
{
    Pos = Vector2D ();
    Rotation = 0;
    Scale = Vector2D (1, 1);
}

void Mesh2D::Render ()
{
	glPushMatrix ();

	glTranslatef (Pos.x, Pos.y, 0.0f);
	glRotatef (Rotation, 0.0f, 0.0f, 1.0f);
	glScalef (Scale.x, Scale.y, 1);

	RenderMesh ();

	glPopMatrix ();
}

GLuint Mesh2D::LoadTextureFromAsset (const string& asset) const {
    GLuint texID = 0;
    glGenTextures (1, &texID);
    glBindTexture (GL_TEXTURE_2D, texID);

	IContentManager& contentManager = Game::ContentManager ();
	Image image = contentManager.LoadImage (asset);

	const uint8_t* pixels = contentManager.LockPixels (image);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, contentManager.GetWidth (image), contentManager.GetHeight (image), 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) pixels);
	contentManager.UnlockPixels (image);

	contentManager.UnloadImage (image);

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texID;
}

Rect2D Mesh2D::CalculateBoundingBox (const vector<float>& vertices) const {
    Vector2D min (FLT_MAX, FLT_MAX);
    Vector2D max (-FLT_MAX, -FLT_MAX);

    for (int i = 0; i < vertices.size (); i += 2) {
        Vector2D pt (vertices[i], vertices[i+1]);
        if (pt < min)
            min = pt;

        if (pt > max)
            max = pt;
    }

    return Rect2D (min, max);
}

GLuint Mesh2D::NewVBO (const vector<float>& data) const {
    GLuint vboID = 0;
    glGenBuffers (1, &vboID);

    glBindBuffer (GL_ARRAY_BUFFER, vboID);
    glBufferData (GL_ARRAY_BUFFER, data.size () * sizeof (float), &data[0], GL_STATIC_DRAW);

    return vboID;
}

vector<GLuint> Mesh2D::NewTexturedVBO (GLuint texID, const vector<float>& vertices, const vector<float>& texCoords) {
	GLuint vboID = 0;
    if (vertices.size () <= 0) {
        vector<float> quad ({
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f
        });

		vboID = NewVBO (quad);
		boundingBox = CalculateBoundingBox (quad);
	} else {
		vboID = NewVBO (vertices);
		boundingBox = CalculateBoundingBox (vertices);
	}

    return {
        vboID,
        NewVBO (texCoords.size () <= 0 ? vector<float> ({
            0.0f, 0.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f
        }) : texCoords)
    };
}

void Mesh2D::RenderTexturedVBO (GLuint texID, GLuint vertCoordID, GLuint texCoordID, GLenum mode, int vertexCount) const {
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable (GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, texID);

    glBindBuffer (GL_ARRAY_BUFFER, vertCoordID);
    glVertexPointer (2, GL_FLOAT, 0, nullptr);
    glEnableClientState (GL_VERTEX_ARRAY);

    glBindBuffer (GL_ARRAY_BUFFER, texCoordID);
    glTexCoordPointer (2, GL_FLOAT, 0, nullptr);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    glDrawArrays (mode, 0, vertexCount);
}