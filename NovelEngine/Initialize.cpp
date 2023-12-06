#include "stdafx.h"
#include "Initialize.hpp"
#include "ElephantLib.hpp"

Initialize::Initialize(const InitData& init) : IScene{ init } {
	//ElephantLib::makeArchive(U"assets/img", U"data/test.arc");
	//ElephantLib::readArchive(U"data/test.arc");

	getData().scriptPath = U"assets/script/init.es";
}

void Initialize::update() {
	changeScene(GameScene::Test2, 0.0s);
}

void Initialize::draw() const {
	//TextureAsset(U"test").draw(Point::Zero());
}
