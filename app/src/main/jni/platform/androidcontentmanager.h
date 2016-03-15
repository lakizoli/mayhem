#pragma once

#include "../management/IContentManager.h"

class AndroidContentManager : public IContentManager {
public:
	AndroidContentManager (jobject activity, jobject assetManager);
	~AndroidContentManager ();

//Image interface
public:
	virtual Image LoadImage (const string & asset) override;
	virtual void UnloadImage (Image& image) override;

	virtual const uint8_t * LockPixels (Image image) override;
	virtual void UnlockPixels (Image image) override;
	virtual int GetWidth (const Image image) const override;
	virtual int GetHeight (const Image image) const override;

//Sound interface
public:
	virtual int LoadSound (const string& asset) override;
	virtual void UnloadSound (int soundID) override;

	virtual void PlaySound (int soundID, float volume, bool looped) override;
	virtual void StopSound (int soundID) override;
	virtual bool IsSoundEnded (int soundID) const override;

//PCM sound interface
public:
	virtual void OpenPCM (float volume, int numChannels, int sampleRate, int bytesPerSample) override;
	virtual void ClosePCM () override;
	virtual bool IsOpenedPCM () const override;

	virtual void WritePCM (const uint8_t* buffer, size_t size) override;

//Utility interface
public:
	virtual string ReadTextFile (const string& fileName) const override;
	virtual void WriteTextFile (const string& fileName, const string& content, bool append) override;

	virtual vector<uint8_t> ReadFile (const string& fileName) const override;
	virtual void WriteFile (const string& fileName, const vector<uint8_t>& content, bool append) override;

	virtual void DisplayStatus (const string& status) const override;

	virtual void Log (const string& log) override;
	virtual double GetTime () const override;

//Helper methods
private:
	jobject openAsset (const string& asset) const;
	void closeStream (jobject istream) const;
	jobject loadBitmap (const string& asset) const;
};
