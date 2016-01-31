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

	int Load (const string& assetName);
	void Unload (int soundID);

	void Play (int soundID, float volume, bool looped);
	void Stop (int soundID);
	bool IsEnded (int soundID);

private:
	static void SLAPIENTRY PlayCallback (SLPlayItf play, void *context, SLuint32 event);

private:
	static int mNextID;

	bool mInited;

	SLObjectItf mEngineObject;
	SLEngineItf mEngine;
	SLObjectItf mOutputMixObject;

	map<int, Player*> mPlayers;

	AAssetManager* mAssetManager;
};
