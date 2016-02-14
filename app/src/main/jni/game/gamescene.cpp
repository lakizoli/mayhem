#include "../pch.h"
#include "gamescene.h"
#include "../engine.h"
#include "../management/game.h"
#include "../content/texanimmesh.h"
#include "../content/coloredmesh.h"
#include "../content/imagemesh.h"

extern engine_s g_engine;
extern "C" void keyboard_key_pressed (signed long key);
extern "C" void keyboard_key_released (signed long key);

void GameScene::Init (float width, float height) {
	mC64Screen.reset (); //created in update phase

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
}

void GameScene::Shutdown () {
	DestroyButtons ();

	if (mC64Screen)
		mC64Screen->Shutdown ();
	mC64Screen.reset ();
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
		mC64Screen.reset (new TexAnimMesh (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel));
		mC64Screen->Init ();

		float c64aspect = (float)g_engine.visible_height / (float)g_engine.visible_width;
		mC64Screen->Scale = Vector2D (0.95f, 0.95f * c64aspect);

		Game& game = Game::Get ();
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
			}
		}
	}
}

void GameScene::TouchUp (int fingerID, const Vector2D& pos) {
	Scene::TouchUp (fingerID, pos);

	if (mState == GameStates::Game) {
		for (auto it = mButtons.begin (); it != mButtons.end (); ++it) {
			if (it->second && (mButtonStates & (uint32_t) it->first) == (uint32_t) it->first && it->second->TransformedBoundingBox ().Contains (pos)) { //If already in pressed state
				HandleKey (it->first, false);

				mButtonStates &= !((uint32_t) it->first);
				mButtonFingerIDs.erase (fingerID);
			}
		}
	}
}

void GameScene::TouchMove (int fingerID, const Vector2D& pos) {
	Scene::TouchMove (fingerID, pos);

	if (mState == GameStates::Game) {
		for (auto it = mButtons.begin (); it != mButtons.end (); ++it) {
			if (it->second && it->second->TransformedBoundingBox ().Contains (pos)) { //Handle finger moves
				//Handle key release by move
				auto itFinger = mButtonFingerIDs.find (fingerID);
				if (itFinger != mButtonFingerIDs.end () && itFinger->second != it->first) { //Touch up of button, when finger moved to another button
					HandleKey (itFinger->second, false);

					mButtonStates &= !((uint32_t) itFinger->second);
					mButtonFingerIDs.erase (fingerID);
				}

				//Handle key press by move
				if ((mButtonStates & (uint32_t) it->first) != (uint32_t) it->first) {
					mButtonStates |= (uint32_t) it->first;
					mButtonFingerIDs[fingerID] = it->first;

					HandleKey (it->first, true);
				}
			}
		}
	}
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
	mButtonFingerIDs.clear ();
}

void GameScene::CreateButton (Buttons button, const Color& color, const Vector2D& pos, const Vector2D& scale,
							  const string& pressAsset, const Vector2D& posPress, const Vector2D& scalePress) {
	shared_ptr <ColoredMesh> left (new ColoredMesh (1, 1, 32, color));
	left->Init ();
	left->Pos = pos;
	left->Scale = scale;
	mButtons[button] = left;

	shared_ptr <ImageMesh> leftPress (new ImageMesh (pressAsset));
	leftPress->Init ();
	leftPress->Pos = posPress;
	leftPress->Scale = scalePress;
	mButtonPresses[button] = leftPress;
}

void GameScene::InitVerticalLayout (bool initButtons) {
	Game& game = Game::Get ();

	//Graphics
	if (mBackground) {
		mBackground->Shutdown ();
		mBackground.reset ();
	}

	mBackground.reset (new ImageMesh ("main_background_vertical.png"));
	mBackground->Init ();
	mBackground->Pos = game.Size () / 2.0f;
	mBackground->Scale = game.Size () * game.ScreenSize () / game.RefSize ();

	//place C64 screen to the right position
	if (mC64Screen) {
		mC64Screen->Pos = Vector2D (0.5f, 0.53f);
	}

	//Init buttons
	if (initButtons) {
		DestroyButtons ();

		CreateButton (Buttons::Left,  Color (1.0f, 0, 0, 0.5f),    Vector2D (0.1f , 1.4f),  Vector2D (0.14f, 0.4f), "left_press.png",  Vector2D (0.1f , 1.4f),  Vector2D (0.14f, 0.4f));
		CreateButton (Buttons::Right, Color (0, 1.0f, 0, 0.5f),    Vector2D (0.25f, 1.4f),  Vector2D (0.14f, 0.4f), "right_press.png", Vector2D (0.25f, 1.4f),  Vector2D (0.14f, 0.4f));
		CreateButton (Buttons::Up,    Color (0, 0, 1.0f, 0.5f),    Vector2D (0.82f, 1.31f), Vector2D (0.28f, 0.18f), "up_press.png",   Vector2D (0.82f, 1.31f), Vector2D (0.28f, 0.18f));
		CreateButton (Buttons::Down,  Color (1.0f, 0, 0, 0.5f),    Vector2D (0.82f, 1.51f), Vector2D (0.28f, 0.18f), "down_press.png", Vector2D (0.82f, 1.51f), Vector2D (0.28f, 0.18f));
		CreateButton (Buttons::Fire,  Color (1.0f, 1.0f, 0, 0.5f), Vector2D (0.495f, 1.4f), Vector2D (0.32f, 0.4f), "fire_press.png",  Vector2D (0.495f, 1.4f), Vector2D (0.32f, 0.4f));
		CreateButton (Buttons::C64,   Color (0, 1.0f, 1.0f, 0.5f), Vector2D (0.09f, 1.01f), Vector2D (0.18f, 0.18f), "c64_press.png", Vector2D (0.09f, 1.01f), Vector2D (0.18f, 0.18f));
	}
}

void GameScene::InitHorizontalLayout (bool initButtons) {
	Game& game = Game::Get ();

	//Graphics
	if (mBackground) {
		mBackground->Shutdown ();
		mBackground.reset ();
	}

	mBackground.reset (new ImageMesh ("main_background_horizontal.png"));
	mBackground->Init ();
	mBackground->Pos = game.Size () / 2.0f;
	mBackground->Scale = game.Size () * game.ScreenSize () / game.RefSize ();


	//place C64 screen to the right position
	if (mC64Screen) {
		mC64Screen->Pos = Vector2D (0.5f * game.Width (), 0.517f);
	}

	//Init buttons
	if (initButtons) {
		DestroyButtons ();

		CreateButton (Buttons::Left,  Color (1.0f, 0, 0, 0.5f),    Vector2D (0.095f, 0.81f), Vector2D (0.14f, 0.35f), "left_press.png",  Vector2D (0.095f, 0.81f), Vector2D (0.14f, 0.35f));
		CreateButton (Buttons::Right, Color (0, 1.0f, 0, 0.5f),    Vector2D (0.245f, 0.81f), Vector2D (0.14f, 0.35f), "right_press.png", Vector2D (0.245f, 0.81f), Vector2D (0.14f, 0.35f));
		CreateButton (Buttons::Up,    Color (0, 0, 1.0f, 0.5f),    Vector2D (1.48f, 0.73f),  Vector2D (0.28f, 0.16f), "up_press.png",    Vector2D (1.48f, 0.73f),  Vector2D (0.28f, 0.16f));
		CreateButton (Buttons::Down,  Color (1.0f, 0, 0, 0.5f),    Vector2D (1.48f, 0.90f),  Vector2D (0.28f, 0.16f), "down_press.png",  Vector2D (1.48f, 0.90f),  Vector2D (0.28f, 0.16f));
		CreateButton (Buttons::Fire,  Color (1.0f, 1.0f, 0, 0.5f), Vector2D (1.48f, 0.48f),  Vector2D (0.25f, 0.25f), "fire_press.png",  Vector2D (1.48f, 0.48f),  Vector2D (0.25f, 0.25f));
		CreateButton (Buttons::C64,   Color (0, 1.0f, 1.0f, 0.5f), Vector2D (0.09f, 0.355f), Vector2D (0.18f, 0.18f), "c64_press.png",   Vector2D (0.09f, 0.355f), Vector2D (0.18f, 0.18f));
	}
}

void GameScene::HandleKey (GameScene::Buttons button, bool pressed) {
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
