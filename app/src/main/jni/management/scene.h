#pragma once

class Vector2D;

///
/// Abstract base class of a scene in the game. (All coordinates are in local system!)
///
class Scene {
public:
	/// Init scene once before draw.
	virtual void Init (float width, float height) = 0;

	/// Shutdown once after draw.
	virtual void Shutdown () = 0;

	/// Pause the scene
	virtual void Pause () = 0;

	/// Continue the scene
	virtual void Continue () = 0;

	/// Resize scene.
	virtual void Resize (float oldWidth, float oldHeight, float newWidth, float newHeight) = 0;

	/// Update scene's state in each step.
	virtual void Update (float elapsedTime) = 0;

	/// Render scene in each step.
	virtual void Render () = 0;

	/// Handle touch down input event.
	virtual void TouchDown (int fingerID, const Vector2D& pos) {
	}

	/// Handle touch up input event.
	virtual void TouchUp (int fingerID, const Vector2D& pos) {
	}

	/// Handle touch move input event.
	virtual void TouchMove (int fingerID, const Vector2D& pos) {
	}
};