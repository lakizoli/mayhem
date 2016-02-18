#include "../pch.h"
#include "gamescene.h"
#include "../engine.h"
#include "../management/game.h"
#include "../content/texanimmesh.h"
#include "../content/coloredmesh.h"
#include "../content/imagemesh.h"
#include "../jnihelper/jniload.h"

extern engine_s g_engine;
extern "C" void keyboard_key_pressed (signed long key);
extern "C" void keyboard_key_released (signed long key);

#define MACHINE_RESET_MODE_HARD 1
extern "C" void vsync_suspend_speed_eval ();
extern "C" void machine_trigger_reset (const unsigned int reset_mode);

#define AUTOSTART_MODE_RUN  0
extern "C" int autostart_disk (const char *file_name, const char *program_name, unsigned int program_number, unsigned int runmode);

//TODO: -> a pause javitasa

//TODO: az aktualis allapot mentesenek implementalasa (berakas a pause-be is, mert kiszallas elott pause jon!)
//TODO: reklam elhelyezese a jatekban

void GameScene::Init (float width, float height) {
	mC64Screen.reset (); //created in update phase
	mBackground.reset ();

	mScreenCounter = 0;
	mDrawCounter = 0;
	mNoDrawCounter = 0;
	mRedSum = 0;
	mGreenSum = 0;
	mBlueSum = 0;

	mState = GameStates::Blue;

	if (width <= height)
		InitVerticalLayout (true);
	else
		InitHorizontalLayout (true);

	mButtonStates = (uint32_t) Buttons::None;
	mButtonLastStates = (uint32_t) Buttons::None;

	mIsResetInProgress = false;
	mResetFingerID = -1;
	mResetStartTime = 0;
}

void GameScene::Shutdown () {
	DestroyButtons ();

	if (mBackground) {
		mBackground->Shutdown ();
		mBackground.reset ();
	}

	if (mC64Screen)
		mC64Screen->Shutdown ();
	mC64Screen.reset ();
}

void GameScene::Pause () {
	Shutdown ();
}

void GameScene::Continue () {
	Game& game = Game::Get ();
	if (game.Width () <= game.Height ())
		InitVerticalLayout (true);
	else
		InitHorizontalLayout (true);
}

void GameScene::Resize (float oldWidth, float oldHeight, float newWidth, float newHeight) {
	if (newWidth <= newHeight)
		InitVerticalLayout (true);
	else
		InitHorizontalLayout (true);
}

void GameScene::Update (float elapsedTime) {
	//Create C64 screen texture
	if (!mC64Screen && g_engine.canvas_inited) {
		Game& game = Game::Get ();

		mC64Screen.reset (new TexAnimMesh (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel));
		mC64Screen->Init ();

		if (game.Width () <= game.Height ())
			InitVerticalLayout (false);
		else
			InitHorizontalLayout (false);

		uint32_t bytePerPixel = g_engine.canvas_bit_per_pixel / 8;
		mC64Pixels.resize (g_engine.visible_width * g_engine.visible_height * bytePerPixel);

		mScreenCounter = 0;
		mDrawCounter = 0;
		mNoDrawCounter = 0;
		mRedSum = 0;
		mGreenSum = 0;
		mBlueSum = 0;

		mState = GameStates::Blue;
	}

	//Update C64 screen texture
	if (mC64Screen) {
		if (g_engine.canvas_dirty) { //Something changed on the screen, so we need to refresh the texture
//			LOGI ("dirty");

			++mScreenCounter;
			++mDrawCounter;
			mNoDrawCounter = 0;

			mRedSum = 0;
			mGreenSum = 0;
			mBlueSum = 0;

			//Trim screen pixel buffer to visible size (convert BGR to RGB)
			if (mState == GameStates::Game)
				ConvertBGRAInGame ();
			else
				ConvertBGRADuringLoad ();

			//Handle game state transitions during initialization (C64 load process)
			switch (mState) {
				case GameStates::Blue:
					if (mRedSum > 10000000 && mGreenSum > 10000000 && mBlueSum > 10000000)
						mState = GameStates::AfterBlue;
					break;
				case GameStates::AfterBlue:
					if (mRedSum < 10000000 && mGreenSum < 10000000 && mBlueSum < 10000000) {
						mScreenCounter = 0;
						mState = GameStates::Demo;
					}
					break;
				case GameStates::Demo:
					if (mScreenCounter == 4) {
						keyboard_key_pressed (57); //press space on keyboard
					} else if (mScreenCounter > 4) {
						keyboard_key_released (57); //release space on keyboard
						mState = GameStates::AfterDemo;
					}
					break;
				case GameStates::AfterDemo:
					if (mRedSum == 0 && mGreenSum == 0 && mBlueSum == 0) {
						mScreenCounter = 0;
						mState = GameStates::BeforeHack;
					}
					break;
				case GameStates::BeforeHack:
					if (mRedSum > 0 && mGreenSum > 0 && mBlueSum > 0) {
						mScreenCounter = 0;
						mState = GameStates::Hack;
					}
					break;
				case GameStates::Hack:
					if (mScreenCounter == 1) {
						keyboard_key_pressed (59); //F1
					} else if (mScreenCounter == 2) {
						keyboard_key_released (59); //F1
						keyboard_key_pressed (61); //F3
					} else if (mScreenCounter == 3) {
						keyboard_key_released (61); //F3
						keyboard_key_pressed (63); //F5
					} else if (mScreenCounter == 4) {
						keyboard_key_released (63); //F5
						mScreenCounter = 0;
						mState = GameStates::AfterHack;
					}
					break;
				case GameStates::AfterHack:
					if (mScreenCounter == 1) {
						keyboard_key_released (57); //release space
					} else if (mScreenCounter > 1) {
						mRedSum = mGreenSum = mBlueSum = 0;
						mState = GameStates::Game;
					}
					break;
				default:
					break;
			}

			//Draw the screen of the game
			/*if (mState == GameStates::Game)*/ {
				mC64Screen->SetPixels (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel, &mC64Pixels[0]);
			}
			g_engine.canvas_dirty = false;
		} else { //Nothing to draw
			mDrawCounter = 0;
			++mNoDrawCounter;

			//Handle game state transitions during initialization (load process)
			switch (mState) {
				case GameStates::AfterHack:
					if (mNoDrawCounter == 3) {
						keyboard_key_pressed (57); //press space
					}
					break;
				default:
					break;
			}
		}
	}

	//Handle input events
	if (mState == GameStates::Game && mButtonStates != mButtonLastStates) {
		HandleKeyStates (Buttons::Left);
		HandleKeyStates (Buttons::Right);
		HandleKeyStates (Buttons::Up);
		HandleKeyStates (Buttons::Down);
		HandleKeyStates (Buttons::Fire);
		HandleKeyStates (Buttons::C64);

		mButtonLastStates = mButtonStates;
	}

	//Handle reset
	if (mIsResetInProgress) {
		double currentTime = Game::Util ().GetTime ();
		if (currentTime - mResetStartTime > 5) { //Hold fire button until 5 sec to reset machine...
			mButtonStates = (uint32_t) Buttons::None;
			mButtonLastStates = (uint32_t) Buttons::None;
			mButtonFingerIDs.clear ();

			mScreenCounter = 0;
			mDrawCounter = 0;
			mNoDrawCounter = 0;
			mRedSum = 0;
			mGreenSum = 0;
			mBlueSum = 0;

			mState = GameStates::Blue;

			vsync_suspend_speed_eval ();
			machine_trigger_reset (MACHINE_RESET_MODE_HARD);

			autostart_disk (g_engine.diskImage.c_str (), nullptr, 0, AUTOSTART_MODE_RUN);
		}
	}
}

void GameScene::Render () {
	//glClearColor (1.0f, 0.5f, 0.5f, 1.0f);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	if (mBackground)
		mBackground->Render ();

	if (mC64Screen)
		mC64Screen->Render ();

//	for (auto it = mButtons.begin ();it != mButtons.end ();++it) {
//		if (it->second)
//			it->second->Render ();
//	}

	if (mButtonStates & (uint32_t)Buttons::Left)
		mButtonPresses[Buttons::Left]->Render ();
	if (mButtonStates & (uint32_t)Buttons::Right)
		mButtonPresses[Buttons::Right]->Render ();
	if (mButtonStates & (uint32_t)Buttons::Up)
		mButtonPresses[Buttons::Up]->Render ();
	if (mButtonStates & (uint32_t)Buttons::Down)
		mButtonPresses[Buttons::Down]->Render ();
	if (mButtonStates & (uint32_t)Buttons::Fire)
		mButtonPresses[Buttons::Fire]->Render ();
	if (mButtonStates & (uint32_t)Buttons::C64)
		mButtonPresses[Buttons::C64]->Render ();
}

void GameScene::TouchDown (int fingerID, const Vector2D& pos) {
	Scene::TouchDown (fingerID, pos);

	if (mState == GameStates::Game) {
		for (auto it = mButtons.begin (); it != mButtons.end (); ++it) {
			if (it->second && (mButtonStates & (uint32_t) it->first) != (uint32_t) it->first && it->second->TransformedBoundingBox ().Contains (pos)) { //If button not pressed
				mButtonStates |= (uint32_t) it->first;
				mButtonFingerIDs[fingerID] = it->first;

				HandleKey (it->first, true);
				break;
			}
		}
	}

	//Handle reset progress start
	HandleResetProgressStart (fingerID, pos);
}

void GameScene::TouchUp (int fingerID, const Vector2D& pos) {
	Scene::TouchUp (fingerID, pos);

	if (mState == GameStates::Game) {
		for (auto it = mButtons.begin (); it != mButtons.end (); ++it) {
			if (it->second && (mButtonStates & (uint32_t) it->first) == (uint32_t) it->first && it->second->TransformedBoundingBox ().Contains (pos)) { //If already in pressed state
				HandleKey (it->first, false);

				mButtonStates &= !((uint32_t) it->first);
				mButtonFingerIDs.erase (fingerID);
				break;
			}
		}
	}

	//Handle reset progress cancel
	HandleResetProgressEnd (fingerID);
}

void GameScene::TouchMove (int fingerID, const Vector2D& pos) {
	Scene::TouchMove (fingerID, pos);

	if (mState == GameStates::Game) {
		//Handle key move out (release touch under finger)
		{
			auto it = mButtonFingerIDs.find (fingerID);
			if (it != mButtonFingerIDs.end ()) {
				auto itButton = mButtons.find (it->second);
				if (itButton != mButtons.end () && !itButton->second->TransformedBoundingBox ().Contains (pos)) { //Touch up of button, when finger moved out from region
					HandleKey (it->second, false);

					mButtonStates &= !((uint32_t) it->second);
					mButtonFingerIDs.erase (fingerID);
				}
			}
		}

		//Handle key move in (touch button under finger)
		for (auto it = mButtons.begin (); it != mButtons.end (); ++it) {
			if (it->second && (mButtonStates & (uint32_t) it->first) != (uint32_t) it->first && it->second->TransformedBoundingBox ().Contains (pos)) { //Handle key press by move
				mButtonStates |= (uint32_t) it->first;
				mButtonFingerIDs[fingerID] = it->first;

				HandleKey (it->first, true);
				break;
			}
		}
	}

	//Handle reset progress cancel
	HandleResetProgressMove (fingerID, pos);
}

void GameScene::ConvertBGRADuringLoad () {
	lock_guard <recursive_mutex> lock (g_engine.canvas_lock);

	assert (g_engine.visible_height <= g_engine.canvas_height);

	uint32_t bytePerPixel = g_engine.canvas_bit_per_pixel / 8;
	uint32_t pitch_src = g_engine.canvas_width * bytePerPixel;
	uint32_t pitch_dest = g_engine.visible_width * bytePerPixel;
	assert (mC64Pixels.size () == pitch_dest * g_engine.visible_height);

	for (uint32_t y = 0, yEnd = g_engine.visible_height; y < yEnd; ++y) {
		uint64_t* src_pixel = (uint64_t*) (&g_engine.canvas[y * pitch_src]);
		uint64_t* dst_pixel = (uint64_t*) (&mC64Pixels[y * pitch_dest]);

		for (uint32_t x = 0, xEnd = g_engine.visible_width; x < xEnd; x += 8) {
			uint64_t src = *src_pixel++;
			mRedSum +=		(uint8_t)((src & 0x00FF000000000000ull) >> 48) + (uint8_t)((src & 0x0000000000FF0000ull) >> 16);
			mGreenSum +=	(uint8_t)((src & 0x0000FF0000000000ull) >> 40) + (uint8_t)((src & 0x000000000000FF00ull) >> 8);
			mBlueSum +=		(uint8_t)((src & 0x000000FF00000000ull) >> 32) + (uint8_t)(src & 0x00000000000000FFull);
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;

			src = *src_pixel++;
			mRedSum +=		(uint8_t)((src & 0x00FF000000000000ull) >> 48) + (uint8_t)((src & 0x0000000000FF0000ull) >> 16);
			mGreenSum +=	(uint8_t)((src & 0x0000FF0000000000ull) >> 40) + (uint8_t)((src & 0x000000000000FF00ull) >> 8);
			mBlueSum +=		(uint8_t)((src & 0x000000FF00000000ull) >> 32) + (uint8_t)(src & 0x00000000000000FFull);
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;

			src = *src_pixel++;
			mRedSum +=		(uint8_t)((src & 0x00FF000000000000ull) >> 48) + (uint8_t)((src & 0x0000000000FF0000ull) >> 16);
			mGreenSum +=	(uint8_t)((src & 0x0000FF0000000000ull) >> 40) + (uint8_t)((src & 0x000000000000FF00ull) >> 8);
			mBlueSum +=		(uint8_t)((src & 0x000000FF00000000ull) >> 32) + (uint8_t)(src & 0x00000000000000FFull);
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;

			src = *src_pixel++;
			mRedSum +=		(uint8_t)((src & 0x00FF000000000000ull) >> 48) + (uint8_t)((src & 0x0000000000FF0000ull) >> 16);
			mGreenSum +=	(uint8_t)((src & 0x0000FF0000000000ull) >> 40) + (uint8_t)((src & 0x000000000000FF00ull) >> 8);
			mBlueSum +=		(uint8_t)((src & 0x000000FF00000000ull) >> 32) + (uint8_t)(src & 0x00000000000000FFull);
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;
		}
	}
}

void GameScene::ConvertBGRAInGame () {
	lock_guard <recursive_mutex> lock (g_engine.canvas_lock);

	assert (g_engine.visible_height <= g_engine.canvas_height);

	uint32_t bytePerPixel = g_engine.canvas_bit_per_pixel / 8;
	uint32_t pitch_src = g_engine.canvas_width * bytePerPixel;
	uint32_t pitch_dest = g_engine.visible_width * bytePerPixel;
	assert (mC64Pixels.size () == pitch_dest * g_engine.visible_height);

	for (uint32_t y = 0, yEnd = g_engine.visible_height; y < yEnd; ++y) {
		uint64_t* src_pixel = (uint64_t*) (&g_engine.canvas[y * pitch_src]);
		uint64_t* dst_pixel = (uint64_t*) (&mC64Pixels[y * pitch_dest]);

		for (uint32_t x = 0, xEnd = g_engine.visible_width; x < xEnd; x += 8) {
			uint64_t src = *src_pixel++;
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;

			src = *src_pixel++;
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;

			src = *src_pixel++;
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;

			src = *src_pixel++;
			*dst_pixel++ = (src & 0x00FF000000FF0000ull) >> 16 | (src & 0x0000FF000000FF00ull) | (src & 0x000000FF000000FFull) << 16 | 0xFF000000FF000000;
		}
	}
}

void GameScene::DestroyButtons () {
	for (auto it = mButtonPresses.begin ();it != mButtonPresses.end ();++it) {
		if (it->second)
			it->second->Shutdown ();
	}
	mButtonPresses.clear ();

	for (auto it = mButtons.begin ();it != mButtons.end ();++it) {
		if (it->second)
			it->second->Shutdown ();
	}
	mButtons.clear ();

	mButtonStates = (uint32_t) Buttons::None;
	mButtonLastStates = (uint32_t) Buttons::None;
	mButtonFingerIDs.clear ();

	mIsResetInProgress = false;
	mResetFingerID = -1;
	mResetStartTime = 0;
}

void GameScene::CreateButton (bool isVerticalLayout, Buttons button, const Color& color, const Vector2D& pos, const Vector2D& scale,
							  const string& pressAsset, const Vector2D& posPress, const Vector2D& scalePress) {
	Game& game = Game::Get ();
	Vector2D screenRefScale = game.ScreenRefScale () * game.AspectScaleFactor ();
	Vector2D screenRefPos = game.ScreenRefPos ();

	shared_ptr <ColoredMesh> left (new ColoredMesh (1, 1, 32, color));
	left->Init ();
	left->Pos = screenRefPos + ConvertRefPercentCoordToLocal (isVerticalLayout, pos) * screenRefScale;
	left->Scale = ConvertRefPercentCoordToLocal (isVerticalLayout, scale) * screenRefScale;
	mButtons[button] = left;

	shared_ptr <ImageMesh> leftPress (new ImageMesh (pressAsset));
	leftPress->Init ();
	leftPress->Pos = screenRefPos + posPress * screenRefScale;
	leftPress->Scale = scalePress * screenRefScale;
	mButtonPresses[button] = leftPress;
}

Vector2D GameScene::ConvertRefPercentCoordToLocal (bool isVerticalLayout, Vector2D percentCoord) const {
	Game& game = Game::Get ();
	if (isVerticalLayout) { //Vertcal position correction :)
		float refAspect = (float)game.RefHeight () / (float)game.RefWidth ();
		percentCoord.y /= refAspect;
	} else { //Horizontal position correction :)
		float refAspect = (float)game.RefWidth () / (float)game.RefHeight ();
		percentCoord.x /= refAspect;
	}
	return game.RefToLocal (percentCoord * game.RefSize ());
}

void GameScene::InitVerticalLayout (bool initButtons) {
	Game& game = Game::Get ();
	Vector2D screenRefScale = game.ScreenRefScale () * game.AspectScaleFactor ();
	Vector2D screenRefPos = game.ScreenRefPos ();

	//Graphics
	if (mBackground) {
		mBackground->Shutdown ();
		mBackground.reset ();
	}

	mBackground.reset (new ImageMesh ("main_background_vertical.png"));
	mBackground->Init ();
	mBackground->Pos = screenRefPos + game.Size () / 2.0f * screenRefScale;
	mBackground->Scale = game.Size () * screenRefScale;

	//place C64 screen to the right position (corrected coordinates :)
	if (mC64Screen) {
		mC64Screen->Pos = screenRefPos + game.RefToLocal (Vector2D (0.5f, 0.319f) * game.RefSize ()) * screenRefScale;

		float c64aspect = (float)g_engine.visible_height / (float)g_engine.visible_width;
		mC64Screen->Scale = ConvertRefPercentCoordToLocal (true, Vector2D (0.95f, 0.95f * c64aspect)) * screenRefScale;
	}

	//Init buttons
	if (initButtons) {
		DestroyButtons ();

		CreateButton (true, Buttons::Left, Color (1.0f, 0, 0, 0.5f), Vector2D (0.1f, 1.4f), Vector2D (0.14f, 0.4f),
					  "left_press.png", game.RefToLocal (75, 1985) + game.RefToLocal (75, 93) / 2.0f, game.RefToLocal (75, 93));
		CreateButton (true, Buttons::Right, Color (0, 1.0f, 0, 0.5f), Vector2D (0.25f, 1.4f), Vector2D (0.14f, 0.4f),
					  "right_press.png", game.RefToLocal (363, 1985) + game.RefToLocal (74, 95) / 2.0f, game.RefToLocal (74, 95));
		CreateButton (true, Buttons::Up, Color (0, 0, 1.0f, 0.5f), Vector2D (0.82f, 1.31f), Vector2D (0.28f, 0.18f),
					  "up_press.png", game.RefToLocal (1128, 1847) + game.RefToLocal (96, 75) / 2.0f, game.RefToLocal (96, 75));
		CreateButton (true, Buttons::Down, Color (1.0f, 0, 0, 0.5f), Vector2D (0.82f, 1.51f), Vector2D (0.28f, 0.18f),
					  "down_press.png", game.RefToLocal (1129, 2133) + game.RefToLocal (93, 73) / 2.0f, game.RefToLocal (93, 73));
		CreateButton (true, Buttons::Fire, Color (1.0f, 1.0f, 0, 0.5f), Vector2D (0.495f, 1.4f), Vector2D (0.32f, 0.4f),
					  "fire_press.png", game.RefToLocal (579, 1896) + game.RefToLocal (262, 272) / 2.0f, game.RefToLocal (262, 272));
		CreateButton (true, Buttons::C64, Color (0, 1.0f, 1.0f, 0.5f), Vector2D (0.09f, 1.01f), Vector2D (0.18f, 0.18f),
					  "c64_press.png", game.RefToLocal (37, 1370) + game.RefToLocal (184, 183) / 2.0f, game.RefToLocal (184, 183));
	}
}

void GameScene::InitHorizontalLayout (bool initButtons) {
	Game& game = Game::Get ();
	Vector2D screenRefScale = game.ScreenRefScale () * game.AspectScaleFactor ();
	Vector2D screenRefPos = game.ScreenRefPos ();

	//Graphics
	if (mBackground) {
		mBackground->Shutdown ();
		mBackground.reset ();
	}

	mBackground.reset (new ImageMesh ("main_background_horizontal.png"));
	mBackground->Init ();
	mBackground->Pos = screenRefPos + game.Size () / 2.0f * screenRefScale;
	mBackground->Scale = game.Size () * screenRefScale;


	//place C64 screen to the right position (corrected coordinates :)
	if (mC64Screen) {
		mC64Screen->Pos = screenRefPos + game.RefToLocal (Vector2D (0.5f, 0.517f) * game.RefSize ()) * screenRefScale;

		float c64aspect = (float)g_engine.visible_height / (float)g_engine.visible_width;
		mC64Screen->Scale = ConvertRefPercentCoordToLocal (false, Vector2D (0.95f, 0.95f * c64aspect)) * screenRefScale;
	}

	//Init buttons
	if (initButtons) {
		DestroyButtons ();

		CreateButton (false, Buttons::Left, Color (1.0f, 0, 0, 0.5f), Vector2D (0.095f, 0.81f), Vector2D (0.14f, 0.35f),
					  "left_press.png", game.RefToLocal (71, 1130) + game.RefToLocal (75, 93) / 2.0f, game.RefToLocal (75, 93));
		CreateButton (false, Buttons::Right, Color (0, 1.0f, 0, 0.5f), Vector2D (0.245f, 0.81f), Vector2D (0.14f, 0.35f),
					  "right_press.png", game.RefToLocal (359, 1130) + game.RefToLocal (74, 95) / 2.0f, game.RefToLocal (74, 95));
		CreateButton (false, Buttons::Up, Color (0, 0, 1.0f, 0.5f), Vector2D (1.48f, 0.73f), Vector2D (0.28f, 0.16f),
					  "up_press.png", game.RefToLocal (2083, 993) + game.RefToLocal (96, 75) / 2.0f, game.RefToLocal (96, 75));
		CreateButton (false, Buttons::Down, Color (1.0f, 0, 0, 0.5f), Vector2D (1.48f, 0.90f), Vector2D (0.28f, 0.16f),
					  "down_press.png", game.RefToLocal (2084, 1279) + game.RefToLocal (93, 73) / 2.0f, game.RefToLocal (93, 73));
		CreateButton (false, Buttons::Fire, Color (1.0f, 1.0f, 0, 0.5f), Vector2D (1.48f, 0.48f), Vector2D (0.25f, 0.25f),
					  "fire_press.png", game.RefToLocal (1996, 574) + game.RefToLocal (262, 272) / 2.0f, game.RefToLocal (262, 272));
		CreateButton (false, Buttons::C64, Color (0, 1.0f, 1.0f, 0.5f), Vector2D (0.09f, 0.355f), Vector2D (0.18f, 0.18f),
					  "c64_press.png", game.RefToLocal (33, 428) + game.RefToLocal (184, 183) / 2.0f, game.RefToLocal (184, 183));
	}
}

void GameScene::HandleKeyStates (Buttons button) {
	if ((mButtonStates & (uint32_t)button) && !(mButtonLastStates & (uint32_t)button)) { //Button pressed
		HandleKey (button, true);
	} else if (!(mButtonStates & (uint32_t)button) && (mButtonLastStates & (uint32_t)button)) { //Button released
		HandleKey (button, false);
	}
}

void GameScene::HandleKey (Buttons button, bool pressed) {
	switch (button) {
		case Buttons::Left:
			if (pressed) {
				keyboard_key_pressed (75);
			} else {
				keyboard_key_released (75);
			}
			break;
		case Buttons::Right:
			if (pressed) {
				keyboard_key_pressed (77);
			} else {
				keyboard_key_released (77);
			}
			break;
		case Buttons::Up:
			if (pressed) {
				keyboard_key_pressed (72);
			} else {
				keyboard_key_released (72);
			}
			break;
		case Buttons::Down:
			if (pressed) {
				keyboard_key_pressed (80);
			} else {
				keyboard_key_released (80);
			}
			break;
		case Buttons::Fire:
			if (pressed) {
				keyboard_key_pressed (100);
			} else {
				keyboard_key_released (100);
			}
			break;
		case Buttons::C64: //Skip level
			if (pressed) {
				keyboard_key_pressed (29);
			} else {
				keyboard_key_released (29);
			}
			break;
		default:
			break;
	}
}

void GameScene::HandleResetProgressStart (int fingerID, const Vector2D& pos) {
	if (mIsResetInProgress)
		return;

	auto it = mButtons.find (Buttons::Fire);
	if (it != mButtons.end () && it->second && it->second->TransformedBoundingBox ().Contains (pos)) {
		mIsResetInProgress = true;
		mResetFingerID = fingerID;
		mResetStartTime = Game::Util ().GetTime ();
	}
}

void GameScene::HandleResetProgressEnd (int fingerID) {
	if (!mIsResetInProgress)
		return;

	if (fingerID == mResetFingerID) {
		mIsResetInProgress = false;
		mResetFingerID = -1;
		mResetStartTime = 0;
	}
}

void GameScene::HandleResetProgressMove (int fingerID, const Vector2D& pos) {
	if (!mIsResetInProgress)
		return;

	if (fingerID == mResetFingerID) {
		auto it = mButtons.find (Buttons::Fire);
		if (it != mButtons.end () && it->second && !it->second->TransformedBoundingBox ().Contains (pos)) {
			mIsResetInProgress = false;
			mResetFingerID = -1;
			mResetStartTime = 0;
		}
	}
}
