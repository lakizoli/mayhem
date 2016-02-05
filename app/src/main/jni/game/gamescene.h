#pragma once

#include "../management/scene.h"

class TexAnimMesh;
class ColoredMesh;

class GameScene : public Scene {
//Definitions
private:

//Data
private:
	shared_ptr<TexAnimMesh> mC64Screen;
	vector<uint8_t> mC64Pixels;

	shared_ptr<ColoredMesh> mLeft;
	shared_ptr<ColoredMesh> mRight;
	shared_ptr<ColoredMesh> mUp;
	shared_ptr<ColoredMesh> mDown;
	shared_ptr<ColoredMesh> mFireLeft;
	shared_ptr<ColoredMesh> mFireRight;

//Construction
public:
	GameScene () {}

//Interface
public:
	virtual void Init (float width, float height) override;
	virtual void Shutdown () override;

	virtual void Resize (float oldWidth, float oldHeight, float newWidth, float newHeight) override;

	virtual void Update (float elapsedTime) override;
	virtual void Render () override;

//Input handlers
public:
	virtual void TouchDown (int fingerID, float x, float y) override;
	virtual void TouchUp (int fingerID, float x, float y) override;
	virtual void TouchMove (int fingerID, float x, float y) override;

//Helper methods
private:
	void DestroyButtons ();

	void InitVerticalLayout (bool initButtons);
	void InitHorizontalLayout (bool initButtons);
};
