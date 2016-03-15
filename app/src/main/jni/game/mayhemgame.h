#pragma once

#include "../management/game.h"
#include "gamescene.h"

struct GameState {
	int highScore;

	GameState ();
	const GameState &operator<< (istream &stream);
};

inline static ostream &operator<< (ostream &stream, const GameState &state) {
	stream << "{\"highScore\":" << state.highScore << "}";
	return stream;
}

class MayhemGame : public Game {
	GameState _state;

public:
	MayhemGame (IContentManager& contentManager);

	virtual void Init (int screenWidth, int screenHeight, int refWidth, int refHeight) override;
	virtual void Shutdown () override;

	const GameState &State () const {
		return _state;
	}

	GameState &State () {
		return _state;
	}

	void ReadGameState ();
	void WriteGameState ();
};
