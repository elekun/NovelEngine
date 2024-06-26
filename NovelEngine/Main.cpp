#include <Siv3D.hpp>
#include "GameScene.hpp"

#include "Initialize.hpp"
#include "Engine.hpp"


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
	Window::Resize(1800, 1080);
	Scene::SetResizeMode(ResizeMode::Keep);
	Window::Resize(1200, 720);

	Window::SetTitle(U"Test Novel Engine");


	registerFontAsset();
	registerPixelShaderAsset();

	GameManager manager;
	manager.add<Initialize>(GameScene::Initialize);
	manager.add<Engine>(GameScene::Engine);

	manager.init(GameScene::Initialize);

	while (System::Update()) {
		if (!manager.update()) {
			break;
		}
	}
}
