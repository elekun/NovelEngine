#include <Siv3D.hpp>
#include "GameScene.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"

void registerFontAsset() {
	for (auto i : Range(1, 256)) {
		FontAsset::Register(U"Medium{}"_fmt(i), i, Typeface::Medium);
	}
}

void registerPixelShaderAsset() {
	PixelShaderAsset::Register(U"OnlyRed", HLSL{ U"assets/shader/hlsl/only_one_color.hlsl", U"OnlyRed" });
	PixelShaderAsset::Register(U"OnlyGreen", HLSL{ U"assets/shader/hlsl/only_one_color.hlsl", U"OnlyGreen" });
	PixelShaderAsset::Register(U"OnlyBlue", HLSL{ U"assets/shader/hlsl/only_one_color.hlsl", U"OnlyBlue" });
}


void Main() {
	Window::Resize(1280, 720);
	Scene::SetResizeMode(ResizeMode::Keep);
	Window::Resize(1280, 720);

	Window::SetTitle(U"Test Novel Engine");


	registerFontAsset();
	registerPixelShaderAsset();

	GameManager manager;
	manager.add<Test>(GameScene::Test);
	manager.add<Initialize>(GameScene::Initialize);
	manager.add<Test2>(GameScene::Test2);

	manager.init(GameScene::Initialize);

	String text = U"\
	var ar = \"abcde\"\
	println(ar[2])\
	";
	auto tokens = Lexer().init(text).tokenize();
	auto blk = Parser().init(tokens).block();
	HashTable<String, shared_ptr<Variable>> vars = {};
	vars = Interpreter().init(blk, vars).run();
	

	while (System::Update()) {
		if (!manager.update()) {
			break;
		}
	}
}
