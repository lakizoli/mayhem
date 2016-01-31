#pragma once

#include "../management/scene.h"
#include "../content/imagemesh.h"

class GameScene : public Scene {
//Definitions
private:

//Data
private:

//Construction
public:
	GameScene () {}

//Interface
public:
	virtual void Init (int width, int height) override;
	virtual void Shutdown () override;

	virtual void Resize (int newWidth, int newHeight) override;

	virtual void Update (float elapsedTime) override;
	virtual void Render () override;

//Input handlers
public:
	virtual void TouchDown (int fingerID, float x, float y) override;
	virtual void TouchUp (int fingerID, float x, float y) override;
	virtual void TouchMove (int fingerID, float x, float y) override;

//Helper methods
private:
};
