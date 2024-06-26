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

	std::function<std::function<bool()>()> test_closure = []() {
		int32 count = 0;
		return [&, count]() mutable {
			Console << U"count: " << ++count;
			return true;
		};
	};
	std::function<bool()> tc;

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
	void initMainProcess(bool saveflag, bool isOverlap);
	bool exeMainProcess();


	// テキスト表示用変数
	InputGroup proceedInput;

	class WindowText {
	private:
		Point windowPos;
		Point messageOffset;
		Point nameOffset;
		int32 defaultMessageSize;
		int32 defaultNameSize;
		int32 layer;
		int32 lineBreakNumber;
		FilePath backgroundPath;
		Texture background;
		FilePath fontpath;
		Font defaultNameFont;
		Font defaultMessageFont;
		HashTable<String, Font> fonts;
		Texture waitIcon;
		Size waitIconSize;
	public:
		size_t index;
		double indexCT;
		String message;
		String name;

		Array<String> linked_tag;

		double time;
		Audio voice;

		WindowText() : index(0), indexCT(0.0), message(U""), name(U""), time(0.0), linked_tag({}) {};
		void setWindowConfig(Point p, Point mo, Point no, int32 ms, int32 ns, int32 l, int32 lb, FilePath fp, FilePath bgp) {
			windowPos = p; messageOffset = mo; nameOffset = no; defaultMessageSize = ms; defaultNameSize = ns; layer = l; lineBreakNumber = lb;
			fontpath = fp;
			backgroundPath = bgp;
			background = Texture{ bgp };
			defaultMessageFont = Font{ defaultMessageSize, fontpath };
			defaultNameFont = Font{ defaultNameSize, fontpath };
		}
		void setWaitIcon(FilePath fp, Size s) {
			waitIcon = Texture{ fp };
			waitIconSize = s;
		};
		void resetWindow() {
			index = 0; indexCT = 0.0; message = U""; time = 0.0;
		}
		int32 getLayer() const { return layer; }
		void draw() const;

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(windowPos, messageOffset, nameOffset, defaultMessageSize, defaultNameSize, layer, fontpath, backgroundPath, index, indexCT, message, name); };
	};

	String sentenceStorage;
	bool allowTextSkip;
	bool isClosed; //UI表示非表示

	void resetTextWindow();

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
		virtual void draw() const;
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
		bool isOverlap;

	public:
		Button() : Graphic{} {};
		Button(FilePath n, FilePath h, FilePath c, Vec2 p, Size s, double sc, double o, int32 l, String t, String e, String r, String jt, String li, String d, bool ol, Engine* en) :
			hover(Texture{ h }), hoverPath(h), click(Texture{ c }), clickPath(c), expr(e), role(r), engine(en), jumptype(jt), link(li), dst(d), isOverlap(ol), Graphic{n, p, s, sc, o, l, t} {};

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
		void draw() const override;
	};
	class Slider : public Graphic {
	protected:
		Texture back;
		FilePath backPath;
		Texture backSub;
		FilePath backSubPath;
		Size backSize;
		String expr; // クリック時に動作するスクリプト
		Engine* engine;
		int32 max;
		int32 min;
		int32 now;
	public:
		Slider() : Graphic{} {};
		Slider(FilePath k, FilePath b, FilePath bs, Vec2 p, Size s, Size bsi, int32 l, String t, int32 ma, int32 mi, int32 de, String e, Engine* en) :
			back(Texture{ b }), backPath(b), backSub(Texture{ bs }), backSubPath(bs), backSize(bsi), max(ma), min(mi), now(de), expr(e), engine(en), Graphic{k, p, s, 1.0, 1.0, l, t} {};

		void setPath(String kp, String bp, String bsp) { path = kp; backPath = bp; backSubPath = bsp; };
		void setTexture() override { texture = Texture{ path }; back = Texture{ backPath }; backSub = Texture{ backSubPath }; };
		void setEnginePointer(Engine* en) { engine = en; };

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(path, position, size, scale, opacity, layer, tag, backPath, backSubPath, backSize, expr, max, min, now); };

		void update() override;
		void draw() const override;
	};
	class Character : public Graphic {
	protected:
		Array<Texture> textures;
		FilePath folderPath;
		Array<Texture> nextTextures;
		FilePath nextFolderPath;
		int32 nowImageIndex;
		String name;
	public:
		Character() : Graphic{} {};
		Character(FilePath fp, Vec2 p, Size s, double sc, double o, int32 l, String t, String n) : folderPath(fp), nextFolderPath(U""), nextTextures({}), name(n) {
			position = p; size = s; scale = sc; opacity = o; layer = l; tag = t;
			nowImageIndex = 0;
			setTexture();

		}
		String getName() { return name;  };
		void setPath(String p) { folderPath = p; };
		virtual void setTexture() {
			Array<FilePath> pathList = FileSystem::DirectoryContents(folderPath, Recursive::No);
			for (auto itr = pathList.begin(); itr != pathList.end(); ++itr) {
				if (!FileSystem::Extension(*itr)) continue;
				textures << Texture{ *itr };
			}
		};
		void setNextPath(String p) { nextFolderPath = p; };
		virtual void setNextTexture() {
			Array<FilePath> pathList = FileSystem::DirectoryContents(nextFolderPath, Recursive::No);
			for (auto itr = pathList.begin(); itr != pathList.end(); ++itr) {
				if (!FileSystem::Extension(*itr)) continue;
				nextTextures << Texture{ *itr };
			}
		};
		virtual void changeTexture() { folderPath = nextFolderPath; textures = nextTextures; nextFolderPath = U""; nextTextures = {}; nowImageIndex = 0; };

		template <class Archive>
		void SIV3D_SERIALIZE(Archive& archive) { archive(folderPath, position, size, scale, opacity, layer, tag, nextFolderPath, nowImageIndex, name); };

		void update() override;
		void draw() const override;
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
	HashTable<String, Audio> musics;
	Audio nowAmbient; // 環境音楽
	FilePath ambientPath;

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
		bool isOverlap;

		InputProcess() {};
		InputProcess(String w, String de, String pe, String ue, String jt, String l, String d, bool ol)
			: who(w), down_exp(de), press_exp(pe), up_exp(ue), jumptype(jt), link(l), dst(d), isOverlap(ol){};
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
		Process() : mainStack({}), subList({}), graphics({}), sounds({}), saveFlag(false), windowText(WindowText{}) {};
		Process(bool sf, bool ol) : mainStack({}), subList({}), graphics({}), sounds({}), saveFlag(sf), isOverlap(ol), windowText(WindowText{}) {};
		Array<std::shared_ptr<std::function<bool()>>> mainStack; // メイン処理のキュー
		// Array<std::function<bool()>> subList; // サブ処理のリスト
		Array<std::shared_ptr<std::function<bool()>>> subList; // サブ処理のリスト
		Array<std::shared_ptr<ContinueProcess>> continueList; // 継続処理のリスト
		Array<std::shared_ptr<Graphic>> graphics; // 画像
		Array<Audio> sounds; // 効果音

		WindowText windowText;

		Array<Choice> selections;
		bool isShowSelection;

		bool saveFlag;

		bool isOverlap;

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
