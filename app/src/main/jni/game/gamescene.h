#pragma once

#include "../management/scene.h"

class TexAnimMesh;
class ColoredMesh;

class GameScene : public Scene {
//Definitions
private:
	enum class GameStates {
		Blue, ///< First state -> turn off input and sound.
		AfterBlue, ///< Second state -> over 10 million color componentsum (red, green, blue)
		Demo, ///< When Screen is under 10 million color componentsum (red, green, blue) -> we turn off screen and push the space button...
		AfterDemo, ///< After the demo screen -> We pushed space on the demo screen
		BeforeHack, ///< The state before hack screen -> all component sum is 0
		Hack, ///< Hack screen -> We need to turn on unlimited capabilities...
		AfterHack, ///< After the hack screen -> We turned the unlimited capabilities...
		Game, ///< After the hack screen -> turn on input, screen and sound...
	};

	enum class Buttons : uint32_t {
		None = 		0x0000,
		All =		0x007F,

		Left = 		0x0001,
		Right = 	0x0002,
		Up = 		0x0004,
		Down = 		0x0008,
		FireLeft = 	0x0010,
		FireRight = 0x0020,
		C64 = 		0x0040
	};

//Data
private:
	shared_ptr<TexAnimMesh> mC64Screen;
	vector<uint8_t> mC64Pixels;

	uint64_t mScreenCounter;
	uint64_t mDrawCounter;
	uint64_t mNoDrawCounter;
	uint32_t mRedSum;
	uint32_t mGreenSum;
	uint32_t mBlueSum;

	GameStates mState;

	map<Buttons, shared_ptr<ColoredMesh>> mButtons;
	uint32_t mButtonStates;
	map<int, Buttons> mButtonFingerIDs;

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
	virtual void TouchDown (int fingerID, const Vector2D& pos) override;
	virtual void TouchUp (int fingerID, const Vector2D& pos) override;
	virtual void TouchMove (int fingerID, const Vector2D& pos) override;

//Helper methods
private:
	void ConvertBGRADuringLoad ();
	void ConvertBGRAInGame ();

	void DestroyButtons ();

	void InitVerticalLayout (bool initButtons);
	void InitHorizontalLayout (bool initButtons);

	void HandleKey (Buttons button, bool pressed);
};
