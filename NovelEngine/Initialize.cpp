#include "stdafx.h"
#include "Initialize.hpp"
#include "ElephantLib.hpp"

Initialize::Initialize(const InitData& init) : IScene{ init } {
	//ElephantLib::makeArchive(U"assets/img", U"data/test.arc");
	//ElephantLib::readArchive(U"data/test.arc");

	getData().scriptPath = U"assets/script/init.es";

	saveVariable();
	loadVariable();
}

void Initialize::update() {
	changeScene(GameScene::Test2, 0.0s);
}

void Initialize::draw() const {
	//TextureAsset(U"test").draw(Point::Zero());
}

void Initialize::saveVariable() {
	/*
	Serializer<BinaryWriter> writer{ U"grobal.bin" };
	HashTable<String, Variable> vars;
	vars[U"test"] = Variable{ U"int", U"30" };
	vars[U"test2"] = Variable{ U"double", U"2.0" };
	vars[U"test3"] = Variable{ U"string", U"variable!" };
	writer(vars);
	*/
}

void Initialize::loadVariable() {
	Deserializer<BinaryReader> reader{ U"grobal.bin" };
	//HashTable<String, Variable> vars;
	//reader(vars);
	//getData().grobalVariable = vars;
}
