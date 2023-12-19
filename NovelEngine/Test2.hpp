#pragma once
#include <Siv3D.hpp>
#include "GameScene.hpp"

class Test2 : public GameManager::Scene {
public:
	Test2(const InitData& init);
	void update() override;
	void draw() const override;

private:

	size_t effectIndex;
	MSRenderTexture renderTexture;


	struct EffectSetting {
		float level;
	};
	ConstantBuffer<EffectSetting> cb;

	// ゲーム内変数
	HashTable<VariableKey, v_type> variable;
	v_type stringToVariable(String type = U"string", String v = U"");
	void setVariable(bool isGrobal, VariableKey key, String type = U"string", String v = U"");
	
	void loadVariable();

	v_type testReturn(v_type v) { v_type value = v; return value; };

	// 汎用関数
	Array<String> splitArgs(String s);
	Array<String> _splitArgs(String s);
	template<typename T>
	void setVariable(T& v, String s);
	template<>
	void setVariable(String& v, String s);

	// ゲーム初期設定変数
	FilePath settingfile;
	int32 textSize;
	int32 nameSize;
	Point textPos;
	Point namePos;
	Rect textRect;
	void readSetting();
	
	// スクリプト用変数
	Array<String> scriptLines;
	size_t scriptIndex;
	bool readStopFlag;

	void readScriptLine(String& s);
	void readScript();
	void analyzeCode(String code);

	// 全処理共通変数
	std::function<bool()> mainProcess;
	Array<std::function<bool()>> subProcesses; // サブ処理のリスト
	String operateLine;

	InputGroup proceedInput;

	// テキスト表示用変数
	size_t strIndex; // 何文字目まで表示するか(0 index)
	double indexCT; // 1文字あたり何秒で表示するか
	String nowSentence; // 今表示される文章
	String nowName; // 今表示される名前
	void resetTextWindow(Array<String> strs);

	// 画面全体処理
	double sceneFade; // シーンのフェードの進行度 range: 0.0 - 1.0
	std::function<void()> fadeProcess;

	// 画像表示用変数
	class Graphic {
	protected:
		Texture texture;
		FilePath path;
		Texture nextTexture;
		FilePath nextPath;
		Vec2 position;
		Size size;
		double scale;
		double opacity;
		int32 layer;
		String tag;
	public:
		Graphic() : texture(Texture{ U"" }), position(0, 0), size(0, 0), scale(1.0), opacity(1.0), layer(0), tag(U"") {};
		Graphic(FilePath fp, Vec2 p, Size s, double sc, double o, int32 l, String t) :
			texture(Texture{ fp }), path(fp), nextTexture(Texture{ U""}), nextPath(U""), position(p), size(s), scale(sc), opacity(o), layer(l), tag(t) {};

		// getter
		int32 getLayer() const { return layer; };
		String getTag() const { return tag; };
		Vec2 getPosition() const { return position; };

		// setter
		void setPath(String p) { path = p; };
		virtual void setTexture() { texture = Texture{ path }; };
		void setNextPath(String p) { nextPath = p; };
		virtual void setNextTexture() { nextTexture = Texture{ nextPath }; };
		void setOpacity(double o) { opacity = o; };
		void setPosition(Vec2 p) { position = p; };

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(path, position, size, scale, opacity, layer, tag); };

		Rect region() const { return Rect{ position.asPoint(), size }; };
		virtual void changeTexture() { path = nextPath; texture = nextTexture; nextPath = U""; nextTexture = Texture{ U"" }; };

		virtual void update();
		void draw() const;
	};

	class Button : public Graphic{
	protected:
		Texture hover;
		FilePath hoverPath;
		Texture nextHover;
		FilePath nextHoverPath;
		Texture click;
		FilePath clickPath;
		Texture nextClick;
		FilePath nextClickPath;
		String exp; // クリック時に動作するスクリプト

	public:
		Button() : Graphic{} {};
		Button(FilePath n, FilePath h, FilePath c, Vec2 p, Size s, double sc, double o, int32 l, String t, String e) :
			hoverPath(h), clickPath(c), exp(e), Graphic{n, p, s, sc, o, l, t} {};

		void setPath(String p, String h, String c) { path = p; hoverPath = h; clickPath = c; };
		void setTexture() override { texture = Texture{ path }; hover = Texture{ hoverPath }; click = Texture{ clickPath }; };
		void setNextPath(String p, String h, String c) { nextPath = p; nextHoverPath = h; nextClickPath = c; };
		void setNextTexture() { nextTexture = Texture{ nextPath }; nextHover = Texture{ nextHoverPath }; nextClick = Texture{ nextClickPath }; };

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(path, position, size, scale, opacity, layer, tag); };

		void changeTexture() override {
			path = nextPath; texture = nextTexture; nextPath = U""; nextTexture = Texture{ U"" };
			hoverPath = nextHoverPath; hover = nextHover; nextHoverPath = U""; nextHover = Texture{ U"" };
			clickPath = nextClickPath; click = nextClick; nextClickPath = U""; nextClick = Texture{ U"" };
		};

		void update() override;
		void draw() const;
	};


	Array<std::shared_ptr<Graphic>> graphics;

	// 音声用変数
	Audio nowMusic;
	Array<Audio> sounds;

	// 選択肢ボタン用変数
	struct Choice{
		String text;
		String dst;
		String type;
		Point pos;
		Point size;
		Texture normal;
		Texture hover;
		Texture click;

		// Choice(String t, String d, Rect r) : text(t), dst(d), region(r) {}
		Choice(){}
		Choice(String t, String d, String ty, Point p, Point s, String n, String h, String c) : text(t), dst(d), type(ty), pos(p), size(s), normal(Texture{ n }), hover(Texture{ h }), click(Texture{ c }){}

		Rect region() const { return Rect{ pos, size }; };
		void draw() const {
			TextureRegion tr;
			if (click && region().leftPressed()) {
				tr = click.resized(size);
			}
			else if (hover && region().mouseOver()) {
				tr = hover.resized(size);
			}
			else {
				tr = normal.resized(size);
			}

			if(normal || hover || click) tr.draw(pos);
			FontAsset(U"Medium25")(text).drawAt(region().center(), Palette::Black);
		};
	};
	Array<Choice> selections;
	bool isShowSelection;

	// ログ
	Array<String> scenarioLog;

	// データセーブ用
	FilePath savefile;
	size_t forSaveIndex; // セーブするスクリプトの行番号, メイン処理のうち前に実行したもの
	Array<Graphic> forSaveGraphics;
	void setSaveVariable();

	// デバッグ用
	bool isShowingLog;
	bool isShowingVariable;
};
