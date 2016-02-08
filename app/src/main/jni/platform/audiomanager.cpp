#include "../pch.h"
#include "audiomanager.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "../jnihelper/jniload.h"

AudioManager::Player::Player (int soundID) :
	soundID (soundID),
	looped (false),
	ended (false),
	player (nullptr),
	play (nullptr),
	seek (nullptr),
	volume (nullptr),
	prefetch (nullptr) {
}

AudioManager::Player::~Player () {
	if (play != nullptr) {
		(*play)->SetPlayState (play, SL_PLAYSTATE_STOPPED);
	}

	if (player != nullptr) {
		(*player)->Destroy (player);
	}

	soundID = 0;
	looped = false;
	ended = false;
	player = nullptr;
	play = nullptr;
	seek = nullptr;
	volume = nullptr;
	prefetch = nullptr;
}

AudioManager::PlayerPCM::PlayerPCM () :
	player (nullptr),
	play (nullptr),
	volume (nullptr),
	queue (nullptr) {
}

AudioManager::PlayerPCM::~PlayerPCM () {
	if (play != nullptr) {
		(*play)->SetPlayState (play, SL_PLAYSTATE_STOPPED);
	}

	if (player != nullptr) {
		(*player)->Destroy (player);
	}

	player = nullptr;
	play = nullptr;
	volume = nullptr;
	queue = nullptr;
}

AudioManager::PCMSample::PCMSample (size_t len) :
	pos (0) {
	buffer.resize (len, 0);
}

size_t AudioManager::PCMSample::Write (const uint8_t* src, size_t size) {
	size_t currentSize = buffer.size ();
	if (pos >= currentSize)
		return 0;

	size_t writeSize = size;
	if (pos + writeSize > currentSize)
		writeSize = currentSize - pos;

	memcpy (&buffer[pos], src, writeSize);
	pos += writeSize;

	return writeSize;
}

int AudioManager::mNextID = 1;

AudioManager::AudioManager () :
	mInited (false),
	mEngineObject (nullptr),
	mEngine (nullptr),
	mOutputMixObject (nullptr),
	mPCMNumChannels (0),
	mPCMSampleRate (0),
	mPCMBytesPerSample (0),
	mAssetManager (nullptr) {
}

AudioManager::~AudioManager () {
	Shutdown ();
}

bool AudioManager::Init (AAssetManager * assetManager) {
	if (mInited)
		return true;

	CHECKMSG (assetManager != nullptr, "AudioManager::Init () - assetManager cannot be nullptr!");
	mAssetManager = assetManager;

	const unsigned int NUM_OPTIONS = 1;
	SLEngineOption options[NUM_OPTIONS] = {
		{ (SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE }
	};

	SLresult result = slCreateEngine (&mEngineObject, NUM_OPTIONS, options, 0, NULL, NULL);
	CHECKMSG (result == SL_RESULT_SUCCESS && mEngineObject != nullptr, "AudioManager::Init () - slCreateEngine () failed");

	result = (*mEngineObject)->Realize (mEngineObject, SL_BOOLEAN_FALSE);
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Init () - EngineObject::Realize () failed");

	result = (*mEngineObject)->GetInterface (mEngineObject, SL_IID_ENGINE, &mEngine);
	CHECKMSG (result == SL_RESULT_SUCCESS && mEngine != nullptr, "AudioManager::Init () - GetInterface () -> Engine failed");

	result = (*mEngine)->CreateOutputMix (mEngine, &mOutputMixObject, 0, NULL, NULL);
	CHECKMSG (result == SL_RESULT_SUCCESS && mOutputMixObject != nullptr, "AudioManager::Init () - CreateOutputMix () failed");

	result = (*mOutputMixObject)->Realize (mOutputMixObject, SL_BOOLEAN_FALSE);
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Init () - OutputMixObject->Realize () failed");

	mInited = true;
	return mInited;
}

void AudioManager::Shutdown () {
	if (!mInited)
		return;

	mPCMPlayer.reset ();
	mPCMs.clear ();

	mPCMNumChannels = 0;
	mPCMSampleRate = 0;
	mPCMBytesPerSample = 0;

	for (auto& it : mPlayers)
		delete it.second;
	mPlayers.clear ();

	mAssetManager = nullptr;

	CHECKMSG (mOutputMixObject != nullptr, "AudioManager::Shutdown () - mOutputMixObject cannot be nullptr!");
	(*mOutputMixObject)->Destroy (mOutputMixObject);
	mOutputMixObject = nullptr;

	CHECKMSG (mEngine != nullptr, "AudioManager::Shutdown () - mEngine cannot be nullptr!");
	mEngine = nullptr;

	CHECKMSG (mEngineObject != nullptr, "AudioManager::Shutdown () - mEngineObject cannot be nullptr!");
	(*mEngineObject)->Destroy (mEngineObject);
	mEngineObject = nullptr;

	mInited = false;
}

int AudioManager::Load (const string & assetName) {
	CHECKMSG (mInited, "AudioManager::Load () - Can be called only after Init ()!");

	AAsset* asset = AAssetManager_open (mAssetManager, assetName.c_str (), AASSET_MODE_UNKNOWN);
	CHECKMSG (asset != nullptr, "AudioManager::Load () - AAssetManager_open () returns nullptr!");

	off_t start = 0;
	off_t length = 0;
	int fd = AAsset_openFileDescriptor (asset, &start, &length);
	CHECKMSG (fd > 0, "AudioManager::Load () - AAsset_openFileDescriptor () returns invalid file descriptor!");

	AAsset_close (asset);
	asset = nullptr;

	// configure audio source
	SLDataLocator_AndroidFD loc_fd = {
		SL_DATALOCATOR_ANDROIDFD,
		fd,
		start,
		length
	};

	SLDataFormat_MIME format_mime = {
		SL_DATAFORMAT_MIME,
		NULL,
		SL_CONTAINERTYPE_UNSPECIFIED
	};

	SLDataSource audioSrc = {
		&loc_fd,
		&format_mime
	};

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = {
		SL_DATALOCATOR_OUTPUTMIX,
		mOutputMixObject
	};

	SLDataSink audioSnk = {
		&loc_outmix,
		NULL
	};

	// allocate played sound instance
	struct Player* player = new struct Player (mNextID++);

	// create audio player
	const unsigned int NUM_INTERFACES = 4;
	const SLInterfaceID ids[NUM_INTERFACES] = { SL_IID_PLAY, SL_IID_SEEK, SL_IID_VOLUME, SL_IID_PREFETCHSTATUS };
	const SLboolean req[NUM_INTERFACES] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	SLresult result = (*mEngine)->CreateAudioPlayer (mEngine, &player->player, &audioSrc, &audioSnk, NUM_INTERFACES, ids, req);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->player != nullptr, "AudioManager::Load () - CreateAudioPlayer () failed");

	// realize the player
	result = (*player->player)->Realize (player->player, SL_BOOLEAN_FALSE);
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Load () - Player::Realize () failed");

	// get the play interface
	result = (*player->player)->GetInterface (player->player, SL_IID_PLAY, &player->play);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->play != nullptr, "AudioManager::Load () - Player::GetInterface (SL_IID_PLAY) failed");

	// get the seek interface
	result = (*player->player)->GetInterface (player->player, SL_IID_SEEK, &player->seek);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->seek != nullptr, "AudioManager::Load () - Player::GetInterface (SL_IID_SEEK) failed");

	// get the volume interface
	result = (*player->player)->GetInterface (player->player, SL_IID_VOLUME, &player->volume);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->volume != nullptr, "AudioManager::Load () - Player::GetInterface (SL_IID_VOLUME) failed");

	// get the prefetch status interface
	result = (*player->player)->GetInterface (player->player, SL_IID_PREFETCHSTATUS, &player->prefetch);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->prefetch != nullptr, "AudioManager::Load () - Player::GetInterface (SL_IID_PREFETCHSTATUS) failed");

	(*player->play)->RegisterCallback (player->play, AudioManager::PlayCallback, player);
	(*player->play)->SetCallbackEventsMask (player->play, SL_PLAYEVENT_HEADATEND);

	mPlayers[player->soundID] = player;
	return player->soundID;
}

void AudioManager::Unload (int soundID) {
	CHECKMSG (mInited, "AudioManager::Unload () - Can be called only after Init ()!");

	auto it = mPlayers.find (soundID);
	if (it == mPlayers.end ())
		return;

	delete it->second;
	mPlayers.erase (it);
}

void AudioManager::Play (int soundID, float volume, bool looped) {
	CHECKMSG (mInited, "AudioManager::Play () - Can be called only after Init ()!");

	auto it = mPlayers.find (soundID);
	if (it == mPlayers.end ())
		return;

	struct Player* player = (struct Player*)it->second;
	if (player == nullptr)
		return;

	player->looped = looped;
	player->ended = false;

	if (player->play != nullptr) {
		SLresult result = (*player->play)->SetPlayState (player->play, SL_PLAYSTATE_STOPPED);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Play::SetPlayState (Stopped) failed");
	}

	if (player->seek != nullptr) {
		SLresult result = (*player->seek)->SetPosition (player->seek, 0, SL_SEEKMODE_ACCURATE);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Seek::SetPosition () failed");

		result = (*player->seek)->SetLoop (player->seek, looped ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE, 0, SL_TIME_UNKNOWN);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Seek::SetLoop () failed");
	}

	if (player->volume != nullptr) {
		SLmillibel maxLevel = 0;
		(*player->volume)->GetMaxVolumeLevel (player->volume, &maxLevel);

		// Convert UI volume to linear factor (cube)
		float vol = volume * volume * volume;

		// millibels from linear amplification
		int level = lroundf (2000.f * log10f (vol));
		if (level < SL_MILLIBEL_MIN)
			level = SL_MILLIBEL_MIN;
		else if (level > (int)maxLevel)
			level = (int) maxLevel;

		SLresult result = (*player->volume)->SetVolumeLevel (player->volume, (SLmillibel) level);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Volume::SetVolumeLevel () failed");
	}

	if (player->play != nullptr) {
		//Set player to paused
		SLresult result = (*player->play)->SetPlayState (player->play, SL_PLAYSTATE_PAUSED);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Play::SetPlayState (Paused) failed");

		// Wait until there's data to play
		if (player->prefetch != nullptr) {
			SLpermille fillLevel = 0;
			while (fillLevel != 1000) {
				result = (*player->prefetch)->GetFillLevel (player->prefetch, &fillLevel);
				CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Prefetch::GetFillLevel () failed");
			}
		}

		//Start playing
		result = (*player->play)->SetPlayState (player->play, SL_PLAYSTATE_PLAYING);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Play () - Play::SetPlayState (Play) failed");
	}
}

void AudioManager::Stop (int soundID) {
	CHECKMSG (mInited, "AudioManager::Stop () - Can be called only after Init ()!");

	auto it = mPlayers.find (soundID);
	if (it == mPlayers.end ())
		return;

	struct Player* player = (struct Player*)it->second;
	if (player == nullptr)
		return;

	SLPlayItf play = player->play;
	if (play != nullptr) {
		SLresult result = (*play)->SetPlayState (play, SL_PLAYSTATE_STOPPED);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::Stop () - Play::SetPlayState () failed");
	}

	player->looped = false;
	player->ended = true;
}

bool AudioManager::IsEnded (int soundID) {
	CHECKMSG (mInited, "AudioManager::IsEnded () - Can be called only after Init ()!");

	auto it = mPlayers.find (soundID);
	if (it == mPlayers.end ())
		return true;

	struct Player* player = (struct Player*)it->second;
	if (player == nullptr)
		return true;

	if (player->ended)
		return true;

	bool isEnded = false;
	SLPlayItf play = player->play;
	if (play != nullptr) {
		SLmillisecond duration = 0;
		SLresult result = (*play)->GetDuration (play, &duration);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::IsEnded () - Play::GetDuration () failed");

		SLmillisecond pos = 0;
		result = (*play)->GetPosition (play, &pos);
		CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::IsEnded () - Play::GetPosition () failed");

		isEnded = pos >= duration;
		if (isEnded) { //Stop when ended
			result = (*play)->SetPlayState (play, SL_PLAYSTATE_STOPPED);
			CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::IsEnded () - Play::SetPlayState (Stop) failed");

			player->ended = true;
		}
	}

	return isEnded;
}

void AudioManager::OpenPCM (int numChannels, int sampleRate, int bytesPerSample) {
	CHECKMSG (numChannels > 0, "AudioManager::OpenPCM () - numChannels must be greater than 0!");
	CHECKMSG (sampleRate > 0, "AudioManager::OpenPCM () - sampleRate must be greater than 0!");
	CHECKMSG (bytesPerSample > 0, "AudioManager::OpenPCM () - bytesPerSample must be greater than 0!");

	mPCMNumChannels = numChannels;
	mPCMSampleRate = sampleRate;
	mPCMBytesPerSample = bytesPerSample;

	mPCMs.clear ();
	mPCMPlayer.reset (new PlayerPCM ());
}

void AudioManager::ClosePCM () {
	mPCMPlayer.reset ();
	mPCMs.clear ();
}

void AudioManager::WritePCM (const uint8_t* buffer, size_t size) {
	CHECKMSG (buffer != nullptr, "AudioManager::WritePCM () - buffer cannot be nullptr!");
	CHECKMSG (size > 0, "AudioManager::WritePCM () - size must be greater than 0!");
	CHECKMSG (mPCMPlayer != nullptr, "AudioManager::WritePCM () - PCM device must be opened before first write!");

	if (mPCMs.size () <= 0)
		mPCMs.push_back (shared_ptr<PCMSample> (new PCMSample (mPCMBytesPerSample)));

	size_t writeSize = size;
	size_t writtenBytes = 0;
	while ((writtenBytes = mPCMs[mPCMs.size () - 1]->Write (buffer, writeSize)) < writeSize) {
		size_t remaining = size - writtenBytes;
		buffer += writtenBytes;
		size -= writtenBytes;

		mPCMs.push_back (shared_ptr<PCMSample> (new PCMSample (mPCMBytesPerSample)));
	}
}

void AudioManager::PlayPCM (float volume) {
	//TODO: ... (lejatszasi seged: http://www.eerock.com/blog/android-opensl-es-loading-and-playing-wav-files/)
}

void AudioManager::StopPCM () {
	//TODO: ...
}

void SLAPIENTRY AudioManager::PlayCallback (SLPlayItf play, void *context, SLuint32 event) {
	CHECKMSG (play != nullptr, "play_callback () - play cannot be nullptr!");
	CHECKMSG (context != nullptr, "play_callback () - context cannot be nullptr!");

	if (event & SL_PLAYEVENT_HEADATEND) {
		struct Player* player = (struct Player*) context;
		if (player != nullptr && !player->ended) {
			SLresult result = (*play)->SetPlayState (play, SL_PLAYSTATE_STOPPED);
			CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::PlayCallback () - Play::SetPlayState (Stop) failed");

			if (!player->looped) { //Register sound end when not looped
				player->ended = true;
			}
		}
	}
}
