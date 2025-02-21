#include "../pch.h"
#include "gamescene.h"
#include "../engine.h"
#include "../management/game.h"
#include "../content/texanimmesh.h"
#include "../content/coloredmesh.h"
#include "../content/imagemesh.h"
#include "../content/animation.h"

extern engine_s g_engine;
extern "C" void keyboard_key_pressed (signed long key);
extern "C" void keyboard_key_released (signed long key);
extern "C" void keyboard_key_clear ();

#define MACHINE_RESET_MODE_HARD 1
extern "C" void vsync_suspend_speed_eval ();
extern "C" void machine_trigger_reset (const unsigned int reset_mode);

#define AUTOSTART_MODE_RUN  0
extern "C" int autostart_disk (const char *file_name, const char *program_name, unsigned int program_number, unsigned int runmode);

extern "C" int ui_quicksnapshot_load ();
extern "C" void ui_quicksnapshot_remove ();
extern "C" void ui_quicksnapshot_save ();
extern "C" int resources_set_int (const char *name, int value);

//TODO: snapshot betoltes nem mindig zarodik le... (orokke starting...)

void GameScene::Init (float width, float height) {
	mC64Screen.reset (); //created in update phase
	mBackground.reset ();

	mRedSum = 0;
	mGreenSum = 0;
	mBlueSum = 0;

	mState = GameStates::Blue;

	mHackCycleCounter = 0;

	if (width <= height)
		InitVerticalLayout (true);
	else
		InitHorizontalLayout (true);

	mButtonStates = (uint32_t) Buttons::None;

	mIsResetInProgress = false;
	mResetFingerID = -1;
	mResetStartTime = 0;
	mIsResetStarted = false;
	mIsAutoStartInited =false;
}

void GameScene::Shutdown () {
	Game::ContentManager ().ClosePCM ();

	DestroyButtons ();
	DestroyAnims ();

	if (mBackground) {
		mBackground->Shutdown ();
		mBackground.reset ();
	}

	if (mC64Screen)
		mC64Screen->Shutdown ();
	mC64Screen.reset ();
}

void GameScene::Pause () {
	if (mState == GameStates::Game) { //Save the game (only when game loaded at first time!)
		ui_quicksnapshot_save ();
	}
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
	Game::ContentManager ().ClosePCM ();

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

		mRedSum = 0;
		mGreenSum = 0;
		mBlueSum = 0;

		mState = GameStates::Blue;

		mHackCycleCounter = 0;
	}

	//Update C64 screen texture
	if (mC64Screen) {
		if (g_engine.canvas_dirty || IsDirtyState ()) { //Something changed on the screen, so we need to refresh the texture
//			Game::ContentManager ().Log ("dirty");

			mRedSum = 0;
			mGreenSum = 0;
			mBlueSum = 0;

			//Trim screen pixel buffer to visible size (convert BGR to RGB)
			if (mState == GameStates::Game)
				ConvertBGRAInGame ();
			else
				ConvertBGRADuringLoad ();

//			stringstream ss;
//			ss << "draw -> R: " << mRedSum << ", G: " << mGreenSum << ", B: " << mBlueSum;
//			Game::ContentManager ().Log (ss.str ());

			//Handle game state transitions during initialization (C64 load process)
			ExecStateTransitions ();

			//Draw the screen of the game
			if (mState == GameStates::Game) {
				mC64Screen->SetPixels (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel, &mC64Pixels[0]);
			}
			g_engine.canvas_dirty = false;
		}
	}

	//Update starting animations
	if (mState != GameStates::Game) { //If not in game state, then show starting anims
		if (mMayhemAnim) {
			if (!mMayhemAnim->IsStarted ())
				mMayhemAnim->Start ();

			mMayhemAnim->Update (elapsedTime);
		}

		if (mStartingAnim) {
			if (!mStartingAnim->IsStarted ())
				mStartingAnim->Start ();

			mStartingAnim->Update (elapsedTime);
		}
	} else { //In game state hide the starting anims
		if (mMayhemAnim && mMayhemAnim->IsStarted ())
			mMayhemAnim->Stop ();

		if (mStartingAnim && mStartingAnim->IsStarted ())
			mStartingAnim->Stop ();
	}

	//Update C64 sound
	if (g_engine.pcm_dirty) {
		IContentManager& contentManager = Game::ContentManager ();

		if (!contentManager.IsOpenedPCM ())
			contentManager.OpenPCM (1.0f, g_engine.pcm_numChannels, g_engine.pcm_sampleRate, g_engine.pcm_bytesPerSec, g_engine.deviceBufferFrames, g_engine.deviceBufferCount);

		{
			lock_guard <recursive_mutex> lock (g_engine.pcm_lock);

//			stringstream ss;
//			ss << "pcm size: " << g_engine.pcm.size ();
//			Game::ContentManager ().Log (ss.str ());

			while (g_engine.pcm.size () > 0) {
				if (mState == GameStates::Game) {
					const vector <uint8_t>& data = g_engine.pcm[0];
					contentManager.WritePCM (&data[0], data.size ());
				}
				g_engine.pcm.pop_front ();
			}
		}

		g_engine.pcm_dirty = false;
	}

	//Handle reset
	if (mIsResetInProgress) {
		IContentManager& contentManager = Game::ContentManager ();
		double currentTime = contentManager.GetTime ();
		if (!mIsResetStarted && currentTime - mResetStartTime > 5) { //Hold fire button until 5 sec to reset machine...
			contentManager.Log ("Reset C64");

			memset (&mC64Pixels[0], 0, mC64Pixels.size () * sizeof (decltype (mC64Pixels)::value_type));
			mC64Screen->SetPixels (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel, &mC64Pixels[0]);

			g_engine.is_warp = true;

			mIsResetStarted = true;
			mIsAutoStartInited = false;

			mButtonStates = (uint32_t) Buttons::None;
			mFingerIDButtons.clear ();
			mButtonFingerIDs.clear ();

			mRedSum = 0;
			mGreenSum = 0;
			mBlueSum = 0;

			mState = GameStates::Blue;

			mHackCycleCounter = 0;

			ui_quicksnapshot_remove ();

			vsync_suspend_speed_eval ();
			machine_trigger_reset (MACHINE_RESET_MODE_HARD);

			HandleKey (Buttons::Left, false);
			HandleKey (Buttons::Right, false);
			HandleKey (Buttons::Up, false);
			HandleKey (Buttons::Down, false);
			HandleKey (Buttons::Fire, false);
			HandleKey (Buttons::C64, false);

			keyboard_key_clear ();

			Game::ContentManager ().ClosePCM ();
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

	if (mState != GameStates::Game) { //If not in game state, then show starting anims
		if (mTitle)
			mTitle->Render ();

		if (mMayhemAnim) {
			uint32_t frame = mMayhemAnim->Frame () % mMayhemAnimFrames.size ();
			mMayhemAnimFrames[frame]->Render ();
		}

		if (mStartingAnim) {
			uint32_t frame = mStartingAnim->Frame () % mStartingAnimFrames.size ();
			mStartingAnimFrames[frame]->Render ();
		}
	}

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
			if (it->second && !IsButtonPressed (it->first) && it->second->TransformedBoundingBox ().Contains (pos)) { //If button not pressed
				PressButton (fingerID, it->first);
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
			if (it->second && IsButtonPressed (it->first) && it->second->TransformedBoundingBox ().Contains (pos)) { //If already in pressed state
				ReleaseButton (fingerID, it->first);
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
			auto it = mFingerIDButtons.find (fingerID);
			if (it != mFingerIDButtons.end ()) {
				auto itButton = mButtons.find (it->second);
				if (itButton != mButtons.end () && !itButton->second->TransformedBoundingBox ().Contains (pos)) { //Touch up of button, when finger moved out from region
					ReleaseButton (fingerID, it->second);
				}
			}
		}

		//Handle key move in (touch button under finger)
		for (auto it = mButtons.begin (); it != mButtons.end (); ++it) {
			if (it->second && !IsButtonPressed (it->first) && it->second->TransformedBoundingBox ().Contains (pos)) { //Handle key press by move
				PressButton (fingerID, it->first);
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

bool GameScene::IsDirtyState () const {
	return mState == GameStates::HackPressF1 ||
		mState == GameStates::HackReleaseF1 ||
		mState == GameStates::HackPressF3 ||
		mState == GameStates::HackReleaseF3 ||
		mState == GameStates::HackPressF5 ||
		mState == GameStates::HackReleaseF5 ||
		mState == GameStates::HackPressSpace ||
		mState == GameStates::HackReleaseSpace;
}

void GameScene::ExecStateTransitions () {
	switch (mState) {
		case GameStates::Blue:
			if (mIsResetInProgress && mIsResetStarted) {
				if (!mIsAutoStartInited) {
					mIsAutoStartInited = true;

					Game::ContentManager ().Log ("Auto starting disk image");
					autostart_disk (g_engine.diskImage.c_str (), nullptr, 0, AUTOSTART_MODE_RUN);

					Game::ContentManager ().ClosePCM ();
				}
			} else if (mRedSum > 10000000 && mGreenSum > 10000000 && mBlueSum > 10000000) {
				mState = GameStates::AfterBlue;
			}
			break;
		case GameStates::AfterBlue:
			if (mRedSum < 10000000 && mGreenSum < 10000000 && mBlueSum < 10000000) {
				g_engine.is_warp = false;
				resources_set_int ("WarpMode", 0);
				keyboard_key_clear ();

				mState = GameStates::DemoPressSpace;
			} else {
				//Try to load snapshot
				int snapshot_loaded = ui_quicksnapshot_load ();
				if (snapshot_loaded) {
					g_engine.is_warp = false;
					resources_set_int ("WarpMode", 0);

					Game::ContentManager ().ClosePCM ();

					mRedSum = mGreenSum = mBlueSum = 0;
					mState = GameStates::Game;
				}
			}
			break;
		case GameStates::DemoPressSpace:
			if (mRedSum > 5000000 && mGreenSum > 5000000 && mBlueSum > 5000000) {
//				Game::ContentManager ().Log ("Space pressed (Demo)");
				keyboard_key_pressed (57); //press space on keyboard

				mState = GameStates::DemoReleaseSpace;
			}
			break;
		case GameStates::DemoReleaseSpace:
//			Game::ContentManager ().Log ("Space pressed (Released)");
			keyboard_key_released (57); //release space on keyboard

			mState = GameStates::AfterDemo;
			break;
		case GameStates::AfterDemo:
			if (mRedSum == 0 && mGreenSum == 0 && mBlueSum == 0) {
				keyboard_key_released (57); //release space on keyboard
				mState = GameStates::BeforeHack;
			} else {
//				Game::ContentManager ().Log ("Space pressed (After demo)");
				keyboard_key_pressed (57); //press space on keyboard
			}
			break;
		case GameStates::BeforeHack:
			if (mRedSum > 10000000 && mGreenSum > 10000000 && mBlueSum > 10000000) {
				keyboard_key_clear ();

				mHackCycleCounter = 0;
				mState = GameStates::HackPressF1;
			}
			break;
		case GameStates::HackPressF1:
			if (mRedSum < 10000000 && mGreenSum < 10000000 && mBlueSum < 10000000) {
//				Game::ContentManager ().Log ("F1 pressed");
				keyboard_key_pressed (59); //F1

				mState = GameStates::HackReleaseF1;
			}
			break;
		case GameStates::HackReleaseF1:
			++mHackCycleCounter;

			if (mHackCycleCounter > 1) {
//				Game::ContentManager ().Log ("F1 released");
				keyboard_key_released (59); //F1

				mHackCycleCounter = 0;
				mState = GameStates::HackPressF3;
			}
			break;
		case GameStates::HackPressF3:
//			Game::ContentManager ().Log ("F3 pressed");
			keyboard_key_pressed (61); //F3

			mState = GameStates::HackReleaseF3;
			break;
		case GameStates::HackReleaseF3:
			++mHackCycleCounter;

			if (mHackCycleCounter > 1) {
//				Game::ContentManager ().Log ("F3 released");
				keyboard_key_released (61); //F3

				mHackCycleCounter = 0;
				mState = GameStates::HackPressF5;
			}
			break;
		case GameStates::HackPressF5:
//			Game::ContentManager ().Log ("F5 pressed");
			keyboard_key_pressed (63); //F5

			mState = GameStates::HackReleaseF5;
			break;
		case GameStates::HackReleaseF5:
			++mHackCycleCounter;

			if (mHackCycleCounter > 1) {
//				Game::ContentManager ().Log ("F5 released");
				keyboard_key_released (63); //F5

				mHackCycleCounter = 0;
				mState = GameStates::HackPressSpace;
			}
			break;
		case GameStates::HackPressSpace:
//			Game::ContentManager ().Log ("Space pressed (After hack)");
			keyboard_key_pressed (57); //press space

			mState = GameStates::HackReleaseSpace;
			break;
		case GameStates::HackReleaseSpace:
//			Game::ContentManager ().Log ("Space released (After hack)");
			keyboard_key_released (57); //release space
			++mHackCycleCounter;

			if (mHackCycleCounter < 3)
				mState = GameStates::HackPressSpace;
			else
				mState = GameStates::AfterHack;
			break;
		case GameStates::AfterHack:
			keyboard_key_clear ();

			mRedSum = mGreenSum = mBlueSum = 0;
			mState = GameStates::Game;
			break;
		default:
			break;
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
	mFingerIDButtons.clear ();
	mButtonFingerIDs.clear ();

	mIsResetInProgress = false;
	mResetFingerID = -1;
	mResetStartTime = 0;
	mIsResetStarted = false;
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

void GameScene::DestroyAnims () {
	if (mTitle) {
		mTitle->Shutdown ();
		mTitle.reset ();
	}

	for (size_t i = 0;i < mMayhemAnimFrames.size ();++i)
		mMayhemAnimFrames[i]->Shutdown ();
	mMayhemAnimFrames.clear ();

	mMayhemAnim.reset ();

	for (size_t i = 0;i < mStartingAnimFrames.size ();++i)
		mStartingAnimFrames[i]->Shutdown ();
	mStartingAnimFrames.clear ();

	mStartingAnim.reset ();
}

void GameScene::InitAnims (const Vector2D& titlePos, const Vector2D& startingPos, const Vector2D& mayhemPos) {
	Game& game = Game::Get ();
	Vector2D screenRefScale = game.ScreenRefScale () * game.AspectScaleFactor ();
	Vector2D screenRefPos = game.ScreenRefPos ();

	mTitle = shared_ptr<ImageMesh> (new ImageMesh ("title.png"));
	mTitle->Init ();
	mTitle->Pos = screenRefPos + titlePos * screenRefScale;
	mTitle->Scale = game.RefToLocal (192 * 4, 111 * 4) * screenRefScale;

	for (int i = 2;i <= 11;++i)
		mMayhemAnimFrames.push_back (LoadAnimFrame ("mayhem_anim/mayhem_", i, ".png", mayhemPos, game.RefToLocal (48 * 4, 42 * 4)));

	mMayhemAnim = shared_ptr<FrameAnimation> (new FrameAnimation (0.1f));

	for (int i = 1;i <= 3;++i)
		mStartingAnimFrames.push_back (LoadAnimFrame ("ss_anim/ss_", i, ".png", startingPos, game.RefToLocal (579, 58)));

	mStartingAnim = shared_ptr<FrameAnimation> (new FrameAnimation (0.2f));
}

shared_ptr<ImageMesh> GameScene::LoadAnimFrame (const string& asset, int idx, const string& assetPostfix, const Vector2D& pos, const Vector2D& scale) const {
	stringstream name;
	name << asset << setw (2) << setfill('0') << idx << assetPostfix;

	Game& game = Game::Get ();
	Vector2D screenRefScale = game.ScreenRefScale () * game.AspectScaleFactor ();
	Vector2D screenRefPos = game.ScreenRefPos ();

	shared_ptr<ImageMesh> frame (new ImageMesh (name.str ()));
	frame->Init ();
	frame->Pos = screenRefPos + pos * screenRefScale;
	frame->Scale = scale * screenRefScale;

	return frame;
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

	//Init anims
	if (mC64Screen) {
		DestroyAnims ();
		InitAnims (mC64Screen->Pos - game.RefToLocal (0, 200), mC64Screen->Pos + game.RefToLocal (0, 180), mC64Screen->Pos + game.RefToLocal (0, 340));
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

	//Init anims
	if (mC64Screen) {
		DestroyAnims ();
		InitAnims (mC64Screen->Pos - game.RefToLocal (0, 200), mC64Screen->Pos + game.RefToLocal (0, 180), mC64Screen->Pos + game.RefToLocal (0, 340));
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

void GameScene::PressButton (int fingerID, Buttons button) {
	//Release opposite button if pressed
	Buttons oppositeButton = Buttons::None;
	switch (button) {
		case Buttons::Left: oppositeButton = Buttons::Right; break;
		case Buttons::Right: oppositeButton = Buttons::Left; break;
		case Buttons::Up: oppositeButton = Buttons::Down; break;
		case Buttons::Down: oppositeButton = Buttons::Up; break;
		default:
			break;
	}

	if (oppositeButton != Buttons::None && IsButtonPressed (oppositeButton)) {
		auto it = mButtonFingerIDs.find (oppositeButton);
		if (it != mButtonFingerIDs.end ())
			ReleaseButton (it->second, oppositeButton);
	}

	//Press button
	mButtonStates |= (uint32_t) button;
	mFingerIDButtons[fingerID] = button;
	mButtonFingerIDs[button] = fingerID;

	HandleKey (button, true);
}

void GameScene::ReleaseButton (int fingerID, Buttons button) {
	HandleKey (button, false);

	mButtonStates &= !((uint32_t) button);
	mFingerIDButtons.erase (fingerID);
	mButtonFingerIDs.erase (button);
}

bool GameScene::IsButtonPressed (Buttons button) const {
	return (mButtonStates & (uint32_t) button) == (uint32_t) button;
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

	auto it = mButtons.find (Buttons::C64);
	if (it != mButtons.end () && it->second && it->second->TransformedBoundingBox ().Contains (pos)) {
		mIsResetInProgress = true;
		mResetFingerID = fingerID;
		mResetStartTime = Game::ContentManager ().GetTime ();
		mIsResetStarted = false;
		mIsAutoStartInited = false;
	}
}

void GameScene::HandleResetProgressEnd (int fingerID) {
	if (!mIsResetInProgress)
		return;

	if (fingerID == mResetFingerID) {
		mIsResetInProgress = false;
		mResetFingerID = -1;
		mResetStartTime = 0;
		mIsResetStarted = false;
		mIsAutoStartInited = false;
	}
}

void GameScene::HandleResetProgressMove (int fingerID, const Vector2D& pos) {
	if (!mIsResetInProgress)
		return;

	if (fingerID == mResetFingerID) {
		auto it = mButtons.find (Buttons::C64);
		if (it != mButtons.end () && it->second && !it->second->TransformedBoundingBox ().Contains (pos)) {
			mIsResetInProgress = false;
			mResetFingerID = -1;
			mResetStartTime = 0;
			mIsResetStarted = false;
			mIsAutoStartInited = false;
		}
	}
}
