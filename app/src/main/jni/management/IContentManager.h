#pragma once

typedef void* Image;

/// The interface of the OS specific content managers.
class IContentManager {
public:
	virtual Image LoadImage (const string& asset) = 0;
	virtual void UnloadImage (Image& image) = 0;

	virtual const uint8_t* LockPixels (Image image) = 0;
	virtual void UnlockPixels (Image image) = 0;
	virtual int GetWidth (const Image image) const = 0;
	virtual int GetHeight (const Image image) const = 0;

	virtual void InitAdMob () const = 0;

	virtual int LoadSound (const string& asset) = 0;
	virtual void UnloadSound (int soundID) = 0;

	virtual void PlaySound (int soundID, float volume, bool looped) = 0;
	virtual void StopSound (int soundID) = 0;
	virtual bool IsSoundEnded (int soundID) const = 0;

	virtual void OpenPCM (float volume, int numChannels, int sampleRate, int bytesPerSample, int deviceBufferFrames, int deviceBufferCount) = 0;
	virtual void ClosePCM () = 0;
	virtual bool IsOpenedPCM () const = 0;

	virtual void WritePCM (const uint8_t* buffer, size_t size) = 0;

	virtual string ReadFile (const string& fileName) const = 0;
	virtual void WriteFile (const string& fileName, const string& content) = 0;

	virtual void DisplayStatus (const string& status) const = 0;
};
