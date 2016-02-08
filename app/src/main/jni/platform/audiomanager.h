#pragma once

struct AAssetManager;

class AudioManager {
private:
	struct Player {
		int soundID;
		bool looped;
		bool ended;

		SLObjectItf player;
		SLPlayItf play;
		SLSeekItf seek;
		SLVolumeItf volume;
		SLPrefetchStatusItf prefetch;

		Player (int soundID);
		~Player ();
	};

	struct PlayerPCM {
		SLObjectItf player;
		SLPlayItf play;
		SLVolumeItf volume;
		SLAndroidSimpleBufferQueueItf queue;

		PlayerPCM ();
		~PlayerPCM ();
	};

	struct PCMSample {
		vector<uint8_t> buffer;
		size_t pos;

		PCMSample (size_t len);
		size_t Write (const uint8_t* src, size_t size);
	};

//Construction
private:
	AudioManager ();

public:
	~AudioManager ();

	static AudioManager& Get () {
		static AudioManager inst;
		return inst;
	}

	bool Init (AAssetManager* assetManager);
	void Shutdown ();

//Asset player interface
public:
	int Load (const string& assetName);
	void Unload (int soundID);

	void Play (int soundID, float volume, bool looped);
	void Stop (int soundID);
	bool IsEnded (int soundID);

//PCM player interface (in memory)
public:
	void OpenPCM (int numChannels, int sampleRate, int bytesPerSample);
	void ClosePCM ();

	void WritePCM (const uint8_t* buffer, size_t size);

	void PlayPCM (float volume);
	void StopPCM ();

//Helper methods
private:
	static void SLAPIENTRY PlayCallback (SLPlayItf play, void *context, SLuint32 event);

//Data
private:
	static int mNextID;

	bool mInited;

	SLObjectItf mEngineObject;
	SLEngineItf mEngine;
	SLObjectItf mOutputMixObject;

	map<int, Player*> mPlayers;

	deque<shared_ptr<PCMSample>> mPCMs;
	shared_ptr<PlayerPCM> mPCMPlayer;
	int mPCMNumChannels;
	int mPCMSampleRate;
	int mPCMBytesPerSample;

	AAssetManager* mAssetManager;
};
