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
	bufferIndex (-1),
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

void AudioManager::PCMSample::Rewind () {
	memset (&buffer[0], 0, buffer.size () * sizeof (decltype (buffer)::value_type));
	pos = 0;
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
	mPCMWriteBufferIndex (0),
	mPCMVolume (0),
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
	mPCMWriteBufferIndex = 0;
	mPCMVolume = 0;

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

void AudioManager::OpenPCM (float volume, int numChannels, int sampleRate, int bytesPerSample) {
	CHECKMSG (numChannels > 0, "AudioManager::OpenPCM () - numChannels must be greater than 0!");
	CHECKMSG (sampleRate > 0, "AudioManager::OpenPCM () - sampleRate must be greater than 0!");
	CHECKMSG (bytesPerSample > 0, "AudioManager::OpenPCM () - bytesPerSample must be greater than 0!");

	mPCMNumChannels = numChannels;
	mPCMSampleRate = sampleRate;
	mPCMBytesPerSample = bytesPerSample;
	mPCMWriteBufferIndex = 0;
	mPCMVolume = volume;

	mPCMs.clear ();
	mPCMs.push_back (shared_ptr<PCMSample> (new PCMSample ((size_t) mPCMBytesPerSample)));
	mPCMs.push_back (shared_ptr<PCMSample> (new PCMSample ((size_t) mPCMBytesPerSample)));

	mPCMPlayer.reset ();
}

void AudioManager::ClosePCM () {
	mPCMPlayer.reset ();
	mPCMs.clear ();
}

void AudioManager::WritePCM (const uint8_t* buffer, size_t size) {
	CHECKMSG (buffer != nullptr, "AudioManager::WritePCM () - buffer cannot be nullptr!");
	CHECKMSG (size > 0, "AudioManager::WritePCM () - size must be greater than 0!");

	size_t writeSize = size;
	size_t writtenBytes = 0;
	while ((writtenBytes = mPCMs[mPCMWriteBufferIndex]->Write (buffer, writeSize)) < writeSize) {
		size_t remaining = size - writtenBytes;
		buffer += writtenBytes;
		size -= writtenBytes;
		writeSize = size;
		mPCMWriteBufferIndex = (mPCMWriteBufferIndex + 1) % mPCMs.size ();
		mPCMs[mPCMWriteBufferIndex]->Rewind ();
	}

	if (mPCMPlayer == nullptr) //Start playing in the first moment
		StartPCM ();
}

void AudioManager::StartPCM () {
	CHECKMSG (mInited, "AudioManager::StartPCM () - Can be called only after Init ()!");
	CHECKMSG (mPCMPlayer == nullptr, "AudioManager::StartPCM () - Can be called only after Init ()!");

	// configure audio source
	SLDataLocator_AndroidFD loc_fd = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
		2
	};

	SLuint16 bitsPerSample = (SLuint16) (mPCMBytesPerSample / mPCMSampleRate * 8);

	SLuint16 containerSize = 0;
	switch (bitsPerSample) {
		case SL_PCMSAMPLEFORMAT_FIXED_8:
			containerSize = SL_PCMSAMPLEFORMAT_FIXED_8;
			break;
		case SL_PCMSAMPLEFORMAT_FIXED_16:
			containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
			break;
		default:
		case SL_PCMSAMPLEFORMAT_FIXED_20:
		case SL_PCMSAMPLEFORMAT_FIXED_24:
		case SL_PCMSAMPLEFORMAT_FIXED_28:
		case SL_PCMSAMPLEFORMAT_FIXED_32:
			containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
			break;
	}

	SLuint32 channelMask = 0;
	if (mPCMNumChannels == 1)
		channelMask = SL_SPEAKER_FRONT_CENTER;
	else if (mPCMNumChannels == 2)
		channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	else {
		CHECKMSG (mPCMNumChannels == 1 || mPCMNumChannels == 2, "Number of channels must be 1 or 2! Other channel count is not supported yet.");
	}

	SLDataFormat_PCM format_pcm;
	format_pcm.formatType       = SL_DATAFORMAT_PCM;
	format_pcm.numChannels      = (SLuint32) mPCMNumChannels;
	format_pcm.samplesPerSec    = (SLuint32) mPCMSampleRate * 1000;
	format_pcm.bitsPerSample    = bitsPerSample;
	format_pcm.containerSize    = containerSize;
	format_pcm.channelMask      = channelMask;
	format_pcm.endianness       = SL_BYTEORDER_LITTLEENDIAN;

	SLDataSource audioSrc = {
		&loc_fd,
		&format_pcm
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
	mPCMPlayer.reset (new PlayerPCM ());
	PlayerPCM* player = mPCMPlayer.get ();

	// create audio player
	const unsigned int NUM_INTERFACES = 3;
	const SLInterfaceID ids[NUM_INTERFACES] = { SL_IID_PLAY, SL_IID_VOLUME, SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
	const SLboolean req[NUM_INTERFACES] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	SLresult result = (*mEngine)->CreateAudioPlayer (mEngine, &player->player, &audioSrc, &audioSnk, NUM_INTERFACES, ids, req);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->player != nullptr, "AudioManager::StartPCM () - CreateAudioPlayer () failed");

	// realize the player
	result = (*player->player)->Realize (player->player, SL_BOOLEAN_FALSE);
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::StartPCM () - Player::Realize () failed");

	// get the play interface
	result = (*player->player)->GetInterface (player->player, SL_IID_PLAY, &player->play);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->play != nullptr, "AudioManager::StartPCM () - Player::GetInterface (SL_IID_PLAY) failed");

	// get the volume interface
	result = (*player->player)->GetInterface (player->player, SL_IID_VOLUME, &player->volume);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->volume != nullptr, "AudioManager::StartPCM () - Player::GetInterface (SL_IID_VOLUME) failed");

	// get the simple buffer queue interface
	result = (*player->player)->GetInterface (player->player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &player->queue);
	CHECKMSG (result == SL_RESULT_SUCCESS && player->queue != nullptr, "AudioManager::StartPCM () - Player::GetInterface (SL_IID_ANDROIDSIMPLEBUFFERQUEUE) failed");

	result = (*player->queue)->RegisterCallback (player->queue, AudioManager::QueueCallback, this);
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::StartPCM () - Queue::RegisterCallback () failed");

	QueueCallback (player->queue, this);

	//Start playing
	result = (*player->play)->SetPlayState (player->play, SL_PLAYSTATE_PLAYING);
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::StartPCM () - Play::SetPlayState (Play) failed");
}

void SLAPIENTRY AudioManager::PlayCallback (SLPlayItf play, void *context, SLuint32 event) {
	CHECKMSG (play != nullptr, "AudioManager::PlayCallback () - play cannot be nullptr!");
	CHECKMSG (context != nullptr, "AudioManager::PlayCallback () - context cannot be nullptr!");

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

void SLAPIENTRY AudioManager::QueueCallback (SLAndroidSimpleBufferQueueItf queue, void *context) {
	CHECKMSG (queue != nullptr, "AudioManager::QueueCallback () - queue cannot be nullptr!");
	CHECKMSG (context != nullptr, "AudioManager::QueueCallback () - context cannot be nullptr!");

	AudioManager* man = (AudioManager*)context;
	shared_ptr<PlayerPCM> player = man->mPCMPlayer;
	CHECKMSG (player != nullptr, "AudioManager::QueueCallback () - player cannot be nullptr!");

	player->bufferIndex = (player->bufferIndex + 1) % 2;
//	LOGI ("starting to play buffer! index: %d", player->bufferIndex);

	shared_ptr<PCMSample> sample = man->mPCMs[player->bufferIndex];
	CHECKMSG (sample != nullptr, "AudioManager::QueueCallback () - sample cannot be nullptr!");

	SLresult result = (*queue)->Enqueue (queue, &(sample->buffer[0]), (SLuint32) sample->buffer.size ());
	CHECKMSG (result == SL_RESULT_SUCCESS, "AudioManager::QueueCallback () - Enqueue of new sample failed!");
}
