#include "../pch.h"
#include "mayhemgame.h"

GameState::GameState () :
	highScore (0) {
}

const GameState &GameState::operator<< (istream & stream) {
	//TODO: implement a general purpose JSON serializer/deserializer into base code...

	bool foundEnd = false;
	while (!foundEnd && stream.good ()) {
		char ch = 0;
		stream.read (&ch, 1);

		if (stream.good ()) {
			switch (ch) {
				case '}':
					foundEnd = true;
					break;
				case ':':
					stream >> highScore;
					break;
				default:
					break;
			}
		}
	}
	return *this;
}

MayhemGame::MayhemGame (IUtil& util, IContentManager& contentManager) :
	Game (util, contentManager) {
}

void MayhemGame::Init (int screenWidth, int screenHeight, int refWidth, int refHeight) {
	Game::Init (screenWidth, screenHeight, refWidth, refHeight);
	ReadGameState ();

	SetCurrentScene (shared_ptr<Scene> (new GameScene ()));
}

void MayhemGame::Shutdown () {
	Game::Shutdown ();

	//...
}

void MayhemGame::ReadGameState () {
	istringstream ss (ContentManager ().ReadFile ("state.save"));
	_state << ss;
}

void MayhemGame::WriteGameState () {
	stringstream ss;
	ss << _state;
	ContentManager ().WriteFile ("state.save", ss.str ());
}
