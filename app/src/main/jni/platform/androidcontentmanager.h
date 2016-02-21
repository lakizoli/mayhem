#pragma once

#include "../management/IContentManager.h"

class AndroidContentManager : public IContentManager {
public:
	AndroidContentManager (jobject activity, jobject assetManager);
	~AndroidContentManager ();

	virtual Image LoadImage (const string & asset) override;
	virtual void UnloadImage (Image& image) override;

	virtual const uint8_t * LockPixels (Image image) override;
	virtual void UnlockPixels (Image image) override;
	virtual int GetWidth (const Image image) const override;
	virtual int GetHeight (const Image image) const override;

	virtual void InitAdMob () const override;

	virtual int LoadSound (const string& asset) override;
	virtual void UnloadSound (int soundID) override;

	virtual void PlaySound (int soundID, float volume, bool looped) override;
	virtual void StopSound (int soundID) override;
	virtual bool IsSoundEnded (int soundID) const override;

	virtual void PausePCM () override;

	virtual string ReadFile (const string& fileName) const override;
	virtual void WriteFile (const string& fileName, const string& content) override;

	virtual void DisplayStatus (const string& status) const override;

private:
	jobject openAsset (const string& asset) const;
	void closeStream (jobject istream) const;
	jobject loadBitmap (const string& asset) const;
};
