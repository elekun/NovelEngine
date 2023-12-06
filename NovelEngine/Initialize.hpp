#pragma once
#include <Siv3D.hpp>
#include "GameScene.hpp"

class Initialize : public GameManager::Scene {
public:
	Initialize(const InitData& init);
	void update() override;
	void draw() const override;

private:
};
