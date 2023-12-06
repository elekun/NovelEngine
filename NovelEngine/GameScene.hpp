#pragma once
#include <Siv3D.hpp>

enum class GameScene {
	Initialize,
	Test,
	Test2,
};

struct GameData {
	FilePath scriptPath;
};

using GameManager = SceneManager<GameScene, GameData>;

#include "Test.hpp"
#include "Test2.hpp"
#include "Initialize.hpp"
