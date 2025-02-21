#pragma once

#include "../management/scene.h"
#include "../content/vector2D.h"
#include "../content/color.h"

class TexAnimMesh;
class ColoredMesh;
class ImageMesh;
class FrameAnimation;

class GameScene : public Scene {
//Definitions
private:
	enum class GameStates {
		Blue, ///< First state -> turn off input and sound.
		AfterBlue, ///< Second state -> over 10 million color componentsum (red, green, blue)
		DemoPressSpace, ///< When Screen is under 10 million color componentsum (red, green, blue) -> we turn off screen and push the space button... (Space press)
		DemoReleaseSpace, ///< When Screen is under 10 million color componentsum (red, green, blue) -> we turn off screen and push the space button... (Space release)
		AfterDemo, ///< After the demo screen -> We pushed space on the demo screen
		BeforeHack, ///< The state before hack screen -> all component sum is 0
		HackPressF1, ///< Hack screen -> We need to turn on unlimited capabilities... (F1 press)
		HackReleaseF1, ///< Hack screen -> We need to turn on unlimited capabilities... (F1 release)
		HackPressF3, ///< Hack screen -> We need to turn on unlimited capabilities... (F3 press)
		HackReleaseF3, ///< Hack screen -> We need to turn on unlimited capabilities... (F3 release)
		HackPressF5, ///< Hack screen -> We need to turn on unlimited capabilities... (F5 press)
		HackReleaseF5, ///< Hack screen -> We need to turn on unlimited capabilities... (F5 release)
		HackPressSpace, ///< Hack screen -> We need to turn on unlimited capabilities... (Space press)
		HackReleaseSpace, ///< Hack screen -> We need to turn on unlimited capabilities... (Space release)
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

	uint32_t mRedSum;
	uint32_t mGreenSum;
	uint32_t mBlueSum;

	GameStates mState;

	int mHackCycleCounter;

	//Graphic data
	shared_ptr<ImageMesh> mBackground;
	shared_ptr<ImageMesh> mTitle;

	vector<shared_ptr<ImageMesh>> mMayhemAnimFrames;
	vector<shared_ptr<ImageMesh>> mStartingAnimFrames;

	shared_ptr<FrameAnimation> mMayhemAnim;
	shared_ptr<FrameAnimation> mStartingAnim;

	//Button data
	map<Buttons, shared_ptr<ColoredMesh>> mButtons;
	uint32_t mButtonStates;
	map<int, Buttons> mFingerIDButtons;
	map<Buttons, int> mButtonFingerIDs;

	map<Buttons, shared_ptr<ImageMesh>> mButtonPresses;

	//Reset watch
	bool mIsResetInProgress;
	int mResetFingerID;
	double mResetStartTime;
	bool mIsResetStarted;
	bool mIsAutoStartInited;

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

	bool IsDirtyState () const;
	void ExecStateTransitions ();

	void DestroyButtons ();
	void CreateButton (bool isVerticalLayout, Buttons button, const Color& color, const Vector2D& pos, const Vector2D& scale, const string& pressAsset, const Vector2D& posPress, const Vector2D& scalePress);

	void DestroyAnims ();
	void InitAnims (const Vector2D& titlePos, const Vector2D& startingPos, const Vector2D& mayhemPos);

	shared_ptr<ImageMesh> LoadAnimFrame (const string& asset, int idx, const string& assetPostfix, const Vector2D& pos, const Vector2D& scale) const;

	Vector2D ConvertRefPercentCoordToLocal (bool isVerticalLayout, Vector2D percentCoord) const;

	void InitVerticalLayout (bool initButtons);
	void InitHorizontalLayout (bool initButtons);

	void PressButton (int fingerID, Buttons button);
	void ReleaseButton (int fingerID, Buttons button);
	bool IsButtonPressed (Buttons button) const;

	void HandleKey (Buttons button, bool pressed);

	void HandleResetProgressStart (int fingerID, const Vector2D& pos);
	void HandleResetProgressEnd (int fingerID);
	void HandleResetProgressMove (int fingerID, const Vector2D& pos);
};
