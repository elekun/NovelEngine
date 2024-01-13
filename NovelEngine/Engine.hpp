#pragma once
#include <Siv3D.hpp>
#include "GameScene.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"


class Engine : public GameManager::Scene {
public:
	Engine(const InitData& init);
	void update() override;
	void draw() const override;

private:
	// ゲーム内変数
	HashTable<String, shared_ptr<Variable>> vars;
	v_type stringToVariable(String type = U"string", String v = U"");

	// 汎用関数
	Array<String> splitArgs(String s);
	template<typename T>
	void setArgument(StringView op, StringView option, T& v, v_type arg);
	template<typename T>
	void setArgumentParse(StringView op, StringView option, T& v, v_type arg);
	template <>
	void setArgumentParse(StringView op, StringView option, String& v, v_type arg);

	// ゲーム用システム変数
	FilePath settingfile;
	int32 textSize;
	int32 nameSize;
	Point textPos;
	Point namePos;
	Rect textRect;
	void readSetting();


	// スクリプト用変数
	struct ScriptData {
		ScriptData(TextReader tr, size_t i, size_t si) : reader(tr), index(i), saveIndex(si) {};
		TextReader reader;
		size_t index;
		size_t saveIndex;
	};
	Array<ScriptData> scriptStack;

	bool readStopFlag;

	void readScriptLine(String& s);
	void readScript();
	std::pair<TextReader, size_t> getDistination(FilePath path, String dst);
	void interprete(String code);
	std::any getValueFromVariable(String var);
	bool boolCheck(std::any value);
	bool evalExpression(String expr);
	bool ifSkipFlag;
	void skipLineForIf();

	// 全処理共通変数
	Array<std::function<bool()>> mainProcess; // メイン処理のキュー
	Array<std::function<bool()>> subProcesses; // サブ処理のリスト
	String operateLine; // 要らないかも。要検討
	void initMainProcess();
	bool exeMainProcess();


	// テキスト表示用変数
	InputGroup proceedInput;
	struct WindowText {
	public:
		size_t index;
		double indexCT;
		String sentence;

		double time;
		Audio voice;

		WindowText() : index(0), indexCT(0.0), sentence(U""), time(0.0) {
		};
	};
	WindowText nowWindowText;
	String sentenceStorage;
	bool checkAuto;

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
		Engine* engine;
		String expr; // クリック時に動作するスクリプト
		String role; // 特殊機能

	public:
		Button() : Graphic{} {};
		Button(FilePath n, FilePath h, FilePath c, Vec2 p, Size s, double sc, double o, int32 l, String t, String e, String r, Engine* en) :
			hoverPath(h), clickPath(c), expr(e), role(r), engine(en), Graphic{n, p, s, sc, o, l, t} {};

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

	struct Process {
	public:
		Process() : mainStack({}), subList({}), graphics({}), sounds({}) {};
		Array<std::function<bool()>> mainStack; // メイン処理のキュー
		Array<std::function<bool()>> subList; // サブ処理のリスト
		Array<std::shared_ptr<Graphic>> graphics; // 画像
		Array<Audio> sounds; // 効果音

		// WindowText windowText;
	};
	Array<Process> processStack; // 全処理のスタック（update()を実行しない、描画はする）

	// ログ
	Array<String> scenarioLog;

	// データセーブ用
	FilePath savefile;
	Array<Graphic> forSaveGraphics;
	void setSaveVariable();

	// デバッグ用
	bool isShowingLog;
	bool isShowingVariable;
	String getVariableList(std::any value) const;
};
