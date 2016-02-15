#pragma once

#include "../management/scene.h"
#include "../content/vector2D.h"
#include "../content/color.h"

class TexAnimMesh;
class ColoredMesh;
class ImageMesh;

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
		Fire = 		0x0010,
		C64 = 		0x0020
	};

//Data
private:
	//C64 emulator specific data
	shared_ptr<TexAnimMesh> mC64Screen;
	vector<uint8_t> mC64Pixels;

	uint64_t mScreenCounter;
	uint64_t mDrawCounter;
	uint64_t mNoDrawCounter;
	uint32_t mRedSum;
	uint32_t mGreenSum;
	uint32_t mBlueSum;

	GameStates mState;

	//Graphic data
	shared_ptr<ImageMesh> mBackground;

	//Button data
	map<Buttons, shared_ptr<ColoredMesh>> mButtons;
	uint32_t mButtonStates;
	map<int, Buttons> mButtonFingerIDs;

	map<Buttons, shared_ptr<ImageMesh>> mButtonPresses;

//Construction
public:
	GameScene () {}

//Interface
public:
	virtual void Init (float width, float height) override;
	virtual void Shutdown () override;

	virtual void Pause () override;
	virtual void Continue () override;

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
	void CreateButton (Buttons button, const Color& color, const Vector2D& pos, const Vector2D& scale, const string& pressAsset, const Vector2D& posPress, const Vector2D& scalePress);

	void InitVerticalLayout (bool initButtons);
	void InitHorizontalLayout (bool initButtons);

	void HandleKey (Buttons button, bool pressed);
};
