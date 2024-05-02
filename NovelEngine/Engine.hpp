#pragma once
#include <Siv3D.hpp>
#include "GameScene.hpp"

#include "ElephantLib.hpp"

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
	v_type stringToVariable(String type = U"string", String v = U"");

	// 汎用関数
	Array<String> splitArgs(String s);
	template<typename T>
	void setArgument(StringView op, StringView option, T& v, v_type arg);
	template<typename T>
	void setArgumentParse(StringView op, StringView option, T& v, v_type arg);
	template <>
	void setArgumentParse(StringView op, StringView option, String& v, v_type arg);

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
	// String operateLine; // 要らないかも。要検討
	bool isStopNow;
	void initMainProcess();
	bool exeMainProcess();


	// テキスト表示用変数
	InputGroup proceedInput;

	Point messagePos;
	Point namePos;
	int32 defaultMessageSize;
	int32 defaultNameSize;
	int32 layer;
	struct WindowText {
	public:
		Point messagePos;
		Point namePos;
		int32 defaultMessageSize;
		int32 defaultNameSize;
		int32 layer;

		size_t index;
		double indexCT;
		String message;
		String name;

		double time;
		Audio voice;

		WindowText() : index(0), indexCT(0.0), message(U""), name(U""), time(0.0) {};
		WindowText(Point mp, Point np, int32 ms, int32 ns, int32 l)
			: messagePos(mp), namePos(np), defaultMessageSize(ms), defaultNameSize(ns), layer(l), index(0), indexCT(0.0), message(U""), name(U""), time(0.0) {};
		void draw() const;

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(messagePos, namePos, defaultMessageSize, defaultNameSize, layer, index, indexCT, message, name); };
	};

	String sentenceStorage;
	bool checkAuto;

	void resetTextWindow(Array<String> strs);

	// 画面全体処理
	double sceneFade; // シーンのフェードの進行度 range: 0.0 - 1.0
	std::function<void()> fadeProcess;

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

		String jumptype; // goto or call
		String link; // jump先のスクリプト
		String dst; // スクリプト内のjump先

	public:
		Button() : Graphic{} {};
		Button(FilePath n, FilePath h, FilePath c, Vec2 p, Size s, double sc, double o, int32 l, String t, String e, String r, String jt, String li, String d, Engine* en) :
			hoverPath(h), clickPath(c), expr(e), role(r), engine(en), jumptype(jt), link(li), dst(d), Graphic{n, p, s, sc, o, l, t} {};

		void setPath(String p, String h, String c) { path = p; hoverPath = h; clickPath = c; };
		void setTexture() override { texture = Texture{ path }; hover = Texture{ hoverPath }; click = Texture{ clickPath }; };
		void setNextPath(String p, String h, String c) { nextPath = p; nextHoverPath = h; nextClickPath = c; };
		void setNextTexture() { nextTexture = Texture{ nextPath }; nextHover = Texture{ nextHoverPath }; nextClick = Texture{ nextClickPath }; };
		void setEnginePointer(Engine* en) { engine = en; };

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(path, position, size, scale, opacity, layer, tag, hoverPath, clickPath, expr, role, jumptype, link, dst); };

		void changeTexture() override {
			path = nextPath; texture = nextTexture; nextPath = U""; nextTexture = Texture{ U"" };
			hoverPath = nextHoverPath; hover = nextHover; nextHoverPath = U""; nextHover = Texture{ U"" };
			clickPath = nextClickPath; click = nextClick; nextClickPath = U""; nextClick = Texture{ U"" };
		};

		void update() override;
		void draw() const;
	};

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


	Audio nowMusic; // BGM
	FilePath musicPath;

	// 継続する処理（入力受付処理など）
	struct ContinueProcess {
	public:
		ContinueProcess() {};
		Engine* engine;
		virtual void invoke() {};
	};

	struct InputProcess : public ContinueProcess {
	public:
		String who;
		String down_exp, press_exp, up_exp;
		String jumptype;
		String link;
		String dst;
		Input input;

		InputProcess() {};
		InputProcess(String w, String de, String pe, String ue, String jt, String l, String d)
			: who(w), down_exp(de), press_exp(pe), up_exp(ue), jumptype(jt), link(l), dst(d){};
		void init(Engine* e);
		void invoke() override;
	};

	struct GetDurationProcess : public ContinueProcess {
	public:
		String who;
		String var;
		Input input;

		GetDurationProcess() {};
		GetDurationProcess(String w, String v) : who(w), var(v) {};
		void init(Engine* e);
		void invoke() override;
	};

	struct MouseWheelScrollProcess : public ContinueProcess {
	public:
		String var;
		String exp;
		Input input;

		MouseWheelScrollProcess() {};
		MouseWheelScrollProcess(String v, String e) : var(v), exp(e) {};
		void init(Engine* e);
		void invoke() override;
	};


	struct Process {
	public:
		Process() : mainStack({}), subList({}), graphics({}), sounds({}), saveFlag(false) {};
		Process(bool sf) : mainStack({}), subList({}), graphics({}), sounds({}), saveFlag(sf) {};
		Array<std::function<bool()>> mainStack; // メイン処理のキュー
		Array<std::function<bool()>> subList; // サブ処理のリスト
		Array<std::shared_ptr<ContinueProcess>> continueList; // 継続処理のリスト
		Array<std::shared_ptr<Graphic>> graphics; // 画像
		Array<Audio> sounds; // 効果音

		WindowText windowText;

		bool saveFlag;

		// template <class Archive>
		// void SIV3D_SERIALIZE(Archive& archive) { archive(graphics, windowText); };
	};
	Array<Process> processStack; // 全処理のスタック（update()を実行しない、描画はする）

	Array<String> scenarioLog; // ログ

	// データセーブ用
	FilePath savefile;
	Array<Process> saveProcessStack;
	void setSaveVariable();
	void dataSave();
	void dataLoad();

	// デバッグ用
	bool isShowingLog;
	bool isShowingVariable;
	String getVariableList(std::any value) const;
};
