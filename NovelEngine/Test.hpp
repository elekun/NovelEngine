#pragma once
#include <Siv3D.hpp>
#include "GameScene.hpp"

class Test : public GameManager::Scene{
public:
	Test(const InitData& init);
	void update() override;
	void draw() const override;

private:
	void readScript();


	size_t effectIndex;
	MSRenderTexture renderTexture;


	struct EffectSetting {
		float level;
	};
	ConstantBuffer<EffectSetting> cb;

	Texture room;

	TextReader reader;

	// 全処理共通変数
	Array<std::function<void()>> mainProcesses; // メイン処理のリスト
	struct SubProcess {
		int32 id;
		int32 triggerID; // 発生のトリガーになる
		std::function<void()> process;
		double time;
	};
	Array<SubProcess> subProcesses; // サブ処理のリスト
	bool processEndFlag; // 処理が終わったかどうか
	size_t processIndex; // 今どの処理をしているか

	// テキスト表示用変数
	size_t strIndex; // 何文字目まで表示するか(0 index)
	double indexCT; // 1文字あたり何秒で表示するか
	double prevTimeUpdateIndex; // 文字送り用変数
	String nowSentence; // 今表示される文章
	String nowName; // 今表示される名前

	// 画像表示用変数
	struct ImageInfo {
		Texture texture;
		Point position;
		int32 layer;
		String tag;
	};
	Array<ImageInfo> images;

	// 音量用変数
	Audio nowMusic;
};
