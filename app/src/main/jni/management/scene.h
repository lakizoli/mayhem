#pragma once

///
/// Abstract base class of a scene in the game. (All coordinates are in local system!)
///
class Scene {
public:
	/// Init scene once before draw.
	virtual void Init (float width, float height) = 0;

	/// Shutdown once after draw.
	virtual void Shutdown () = 0;

	/// Resize scene.
	virtual void Resize (float oldWidth, float oldHeight, float newWidth, float newHeight) = 0;

	/// Update scene's state in each step.
	virtual void Update (float elapsedTime) = 0;

	/// Render scene in each step.
	virtual void Render () = 0;

	/// Handle touch down input event.
	virtual void TouchDown (int fingerID, float x, float y) {
	}

	/// Handle touch up input event.
	virtual void TouchUp (int fingerID, float x, float y) {
	}

	/// Handle touch move input event.
	virtual void TouchMove (int fingerID, float x, float y) {
	}
};