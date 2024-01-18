#include "stdafx.h"
#include "Engine.hpp"
#include "ElephantLib.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"

using namespace ElephantLib;

Engine::Engine(const InitData& init) : IScene{ init } {

	Console << U"script path :" << getData().scriptPath;

	TextReader reader{ getData().scriptPath };
	if (!reader) {
		throw Error{ U"Failed to open `{}`"_fmt(getData().scriptPath) };
	}
	scriptStack = {};
	scriptStack << ScriptData{reader, 0, 0};

	readStopFlag = false;

	processStack << Process{};

	isStopNow = false;
	initMainProcess();
	fadeProcess = [&, this]() {};

	nowName = U"";
	isShowSelection = false;

	isShowingLog = false;
	isShowingVariable = false;

	savefile = U"./test_save.dat";

	proceedInput = MouseL | KeyEnter | KeySpace;
}

v_type Engine::stringToVariable(String type, String v) {
	v_type value;
	if (type == U"int") {
		value = v.isEmpty() ? 0 : Parse<int32>(v);
	}
	else if (type == U"double") {
		value = v.isEmpty() ? 0.0 : Parse<double>(v);
	}
	else if (type == U"bool") {
		value = v.isEmpty() ? false : Parse<bool>(v);
	}
	else if (type == U"point") {
		value = v.isEmpty() ? Point::Zero() : Parse<Point>(v);
	}
	else if (type == U"string") {
		value = v;
	}
	return value;
}

Array<String> Engine::splitArgs(String s) {
	if (s == U"") {
		return {};
	}

	// 各変数ごとに分割
	Array<String> args = {};
	Array<char32> bracket = {};
	char32 quote = U' '; // " or ' or `
	String tmp = U"";
	for (auto i : step(s.length())) {
		char32 c = s.at(i);

		for (auto q : { U'\'', U'"', U'`' }) {
			if (c == q) {
				if (quote == U' ') {
					quote = c;
				}
				else if (quote == q) {
					quote = U' ';
				}
			}
		}

		if (c == U'(' || c == U'{') {
			bracket << c;
		}
		if (c == U')' && U'(' == bracket.back()) {
			bracket.pop_back();
		}
		if (c == U'}' && U'{' == bracket.back()) {
			bracket.pop_back();
		}

		if (bracket.isEmpty() && quote == U' ' && c == U',') {
			args << tmp;
			tmp = U"";
		}
		else {
			tmp << c;
		}
	}
	args << tmp;

	return args;
}

template<typename T>
inline void Engine::setArgument(StringView op, StringView option, T& v, v_type arg) {
	if (auto p = std::get_if<T>(&arg)) {
		v = *p;
	}
	else {
		ThrowArgumentError(op, option, Unicode::Widen(typeid(T).name()), arg);
	}
}

template<typename T>
inline void Engine::setArgumentParse(StringView op, StringView option, T& v, v_type arg) {
	Optional<T> o = std::visit([](auto& x) {
		return ParseOpt<T>(Format(x));
	}, arg);

	if (o) {
		v = *o;
	}
	else {
		ThrowParseError(Unicode::Widen(typeid(T).name()), arg);
	}
}

template<>
inline void Engine::setArgumentParse(StringView op, StringView option, String& v, v_type arg) {
	v = std::visit([](auto& x) {
		return Format(x);
	}, arg);
}

void Engine::readScriptLine(String& s) {
	if (scriptStack.isEmpty()) {
		s = U"";
		readStopFlag = true;
		return;
	}

	if (scriptStack.back().reader.readLine(s)) {
		scriptStack.back().index++;
	}
	else {
		scriptStack.pop_back();
		readScriptLine(s);
	}
}

void Engine::readScript() {
	String line;
	readStopFlag = false;

	auto getArgs = [&, this](StringView sv) {
		Array<std::pair<String, v_type>> args = {}; // 引き渡された変数
		for (auto i : splitArgs(String{ sv })) {
			String s = eraseFrontBackChar(i, U' '); //前後のスペースを削除
			auto r = RegExp(U"([0-9a-zA-Z_]+) *= *(.+)").match(s);
			String optionName{ r[1].value() };
			String optionArg{ r[2].value() };

			v_type value;


			if (auto mr = RegExp(U"<(.+)>").match(optionArg)) {
				std::any v = getValueFromVariable(String{ mr[1].value() });
				if (auto p = std::any_cast<int32>(&v)) {
					value = *p;
				}
				else if (auto p = std::any_cast<double>(&v)) {
					value = *p;
				}
				else if (auto p = std::any_cast<String>(&v)) {
					value = *p;
				}
				else {
					throw Error{U"不適切な変数です"};
				}
				args << std::make_pair(optionName, value);
				continue;
			}

			// string or not
			Array<char32> _q = { U'\'', U'"', U'`' };
			if (auto mr = RegExp(U"\"(.*)\"|'(.*)'|`(.*)`").match(optionArg)) {
				if (mr[1].has_value()) {
					value = String{ mr[1].value() };
				}
				else if (mr[2].has_value()) {
					value = String{ mr[2].value() };
				}
				else if (mr[3].has_value()) {
					value = String{ mr[3].value() };
				}
				args << std::make_pair(optionName, value);
				continue;
			}

			// point or not
			if (auto mr = RegExp(U"\\( *(.+), *(.+) *\\)").match(optionArg)) {
				Array<int32> tmp = {};
				for (int32 i = 1; i < mr.size(); i++) {
					String tmp_s{ mr[i].value() };

					if (auto mr = RegExp(U"<(.+)>").match(tmp_s)) {
						std::any v = getValueFromVariable(String{ mr[1].value() });
						if (auto p = std::any_cast<int32>(&v)) {
							tmp << *p;
						}
						else {
							throw Error{U"不適切な変数です"};
						}
						continue;
					}
					else {
						if (auto e = ParseOpt<int32>(tmp_s)) {
							tmp << *e;
						}
					}
				}
				if (tmp.size() == 2) {
					value = Point{ tmp[0], tmp[1] };
					args << std::make_pair(optionName, value);
				}
				else {
					throw Error{U"Value Error : point型の構成でエラーが起きました"};
				}
				continue;
			}

			// bool or not
			if (RegExp(U"true|false").fullMatch(optionArg)) {
				value = Parse<bool>(optionArg);
				args << std::make_pair(optionName, value);
				continue;
			}

			// double or not
			if (RegExp(U"[+,-]?([1-9]\\d*|0)\\.\\d+").fullMatch(optionArg)) {
				value = Parse<double>(optionArg);
				args << std::make_pair(optionName, value);
				continue;
			}

			// int or not
			if (RegExp(U"[+,-]?([1-9]\\d*|0)").fullMatch(optionArg)) {
				value = Parse<int32>(optionArg);
				args << std::make_pair(optionName, value);
				continue;
			}

			throw Error{U"{} は値として不適切です"_fmt(optionArg)};
		}

		return args;
	};

	auto setMainProcess = [&, this](std::function<bool()> f) {
		processStack.back().mainStack << f;
		//mainProcess << f;
		readStopFlag = true;
	};

	auto addSubProcess = [&, this](std::function<bool()> f) {
		processStack.back().subList.emplace_back(f);
	};

	// 命令
	Array<String> pn = {
		U"start", U"name",
		U"mes_config",
		U"setimg", U"setbtn", U"change", U"moveto", U"moveby", U"delete",
		U"music", U"sound", U"volmusic", U"volsound",
		U"wait", U"stop", U"scenechange", U"exit",
		U"choice", U"goto", U"call", U"return",
		U"if", U"elif", U"else",U"endif",
		U"sleeppr", U"awakepr",
		U"input", U"mousewheel"
	};
	String proc = U"";
	for (auto [j, p] : Indexed(pn)) {
		proc += p;
		if (j != pn.size() - 1) { proc += U"|"; }
	}

	// 文章を読み込む処理
	do {
		readScriptLine(line);

		// コード部分なら一気に読み込んで行く
		// 複数行
		if (RegExp(U" *```.*").fullMatch(line)) {
			String code = String{ RegExp(U" *```(.*)").search(line)[1].value() };
			bool isCodeLine = true;
			while (isCodeLine) {
				readScriptLine(line);
				if (RegExp(U".*``` *").fullMatch(line)) {
					code += String{ RegExp(U"(.*)``` *").search(line)[1].value() };
					isCodeLine = false;
				}
				else {
					code += line + U" ";
				}
			}
			interprete(code);
			line = U"";
		}
		// 1行
		if (RegExp(U" *`([^`]*)` *").fullMatch(line)) {
			String code = String{ RegExp(U" *`([^`]*)` *").match(line)[1].value() };
			interprete(code);
			line = U"";
		}

		// # 始まりと @ 始まりの行は特殊文として処理
		if (auto tmp_op = RegExp(U" *#(.*)").match(line)) {
			operateLine = tmp_op[1].value();
		}
		else if (auto tmp_op = RegExp(U" *@(.*)").match(line)) {
			// dst
		}
		else if(line != U"") {
			sentenceStorage += line + U"\n";
		}
		else {
			// 文章表示
			if (sentenceStorage != U"") {
				processStack.back().windowText.message = eraseBackChar(sentenceStorage, U'\n'); // 末尾の改行を削除
				processStack.back().windowText.indexCT = 0.05;

				std::function<bool()> f = [&, this]() {

					processStack.back().windowText.time += Scene::DeltaTime();

					if (getData().isAuto) {
						if (processStack.back().windowText.time >= processStack.back().windowText.indexCT * 20) {
							resetTextWindow({ processStack.back().windowText.message });
							setSaveVariable();
							return true;
						}
					}
					else {
						// 左クリックorスペースで
						if (proceedInput.down()) {
							// 最後まで文字が表示されているなら次に行く
							if (processStack.back().windowText.index >= processStack.back().windowText.message.size() - 1) {
								resetTextWindow({ processStack.back().windowText.message });
								setSaveVariable();
								return true;
							}
							// まだ最後まで文字が表示されていないなら最後まで全部出す
							else {
								processStack.back().windowText.index = processStack.back().windowText.message.size() - 1;
							}
						}
					}

					// 選択肢が用意されているときの処理
					// 文字が最後まで表示されたら
					if (!selections.isEmpty() && processStack.back().windowText.index >= processStack.back().windowText.message.size() - 1) {
						// 選択肢を表示しておく
						isShowSelection = true;
						return true;
					}

					// 1文字ずつ順番に表示する
					if (processStack.back().windowText.time >= processStack.back().windowText.indexCT) {
						if (processStack.back().windowText.index < processStack.back().windowText.message.size() - 1) {
							processStack.back().windowText.index++;
							processStack.back().windowText.time = 0;
						}
					}

					return false;
				};
				setMainProcess(f);
			}

			// 選択肢表示
			if (!selections.isEmpty()) {
				if (processStack.back().windowText.message == U"") {
					isShowSelection = true;
				}
				std::function<std::function<bool()>()> f = [&, this]() {
					bool isClicked = false;
					return [&, this]() mutable {
						for (auto itr = selections.begin(); itr != selections.end(); ++itr) {
							if (Rect{ itr->pos, itr->size }.leftReleased() && isClicked) {
								resetTextWindow({ processStack.back().windowText.message, U"choiced : " + itr->text });
								setSaveVariable();
								isShowSelection = false;

								FilePath fp = scriptStack.back().reader.path();
								auto [reader, i] = getDistination(fp, itr->dst);

								scriptStack.pop_back();
								scriptStack << ScriptData{reader, i, i};

								selections.clear();
								return true;
							}
							if (Rect{ itr->pos, itr->size }.leftClicked()) {
								isClicked = true;
							}
						}

						return false;
					};
				};
				setMainProcess(f());
			}
		}

		// 各種処理
		if (operateLine != U"") {
			if (auto operate = RegExp(U"({})\\[(.*)\\]"_fmt(proc)).match(operateLine)) {
				StringView op = operate[1].value(); // 処理のタグ
				Array<std::pair<String, v_type>> args = getArgs(operate[2].value());

				if (!pn.includes(String{ op }))
					throw Error{U"{}命令は定義されていません"_fmt(String{ op })};

				if (op == U"start") {
					// ...option
					double d = 0.0;

					for (auto [option, arg] : args) {
						if (option == U"duration") {
							setArgumentParse(op, option, d, arg);
							d = Max(d, 0.0);
						}
					}

					fadeProcess = [&, this]() {
						Rect{ Point::Zero(), Scene::Size() }.draw(ColorF{ 0.0, 0.0, 0.0, 1.0 - sceneFade });
					};

					std::function<std::function<bool()>()> f = [&, this, _d = d]() {
						double time = 0.0;

						return [&, this, time, duration = _d]() mutable {
							sceneFade = duration > 0 ? Clamp(time / duration, 0.0, 1.0) : 1.0;

							time += Scene::DeltaTime();

							if (time > duration) {
								return true;
							}

							return false;
						};
					};
					addSubProcess(f());
					// subProcesses.emplace_back(f());

					// 強制待機
					std::function<std::function<bool()>()> w = [&, this, _d = d]() {
						double time = 0.0;
						return [&, this, time, duration = _d]() mutable {
							if (time >= duration) {
								return true;
							}
							time += Scene::DeltaTime();

							return false;
						};
					};
					setMainProcess(w());
				}

				if (op == U"name") {
					// newName
					String name = U"";
					for (auto [option, arg] : args) {
						if (option == U"name") {
							setArgument(op, option, name, arg);
						}
					}

					std::function<bool()> f = [&, this, n = name]() {
						nowName = n;
						return true;
					};
					setMainProcess(f);
				}

				if (op == U"mes_config") {
					Point mp = Point::Zero(), np = Point::Zero();
					int32 ms = 0, ns = 0;
					int32 l = 0;
					for (auto [option, arg] : args) {
						if (option == U"mes_pos") {
							setArgument(op, option, mp, arg);
							messagePos = mp;
							processStack.back().windowText.messagePos = mp;
						}
						if (option == U"name_pos") {
							setArgument(op, option, np, arg);
							namePos = np;
							processStack.back().windowText.namePos = np;
						}
						if (option == U"mes_size") {
							setArgument(op, option, ms, arg);
							defaultMessageSize = ms;
							processStack.back().windowText.defaultMessageSize = ms;
						}
						if (option == U"name_size") {
							setArgument(op, option, ns, arg);
							defaultNameSize = ns;
							processStack.back().windowText.defaultNameSize = ns;
						}
						if (option == U"layer") {
							setArgument(op, option, l, arg);
							layer = l;
							processStack.back().windowText.layer = l;
						}
					}
				}

				if (op == U"setimg") {
					String n = U""; // タグの名前
					FilePath fp = U"";
					Point p = Point::Zero();
					int32 l = 0;
					Size si = Size::Zero();

					double fa = 0; // フェードインにかかる時間
					double s = 1.0;

					for (auto [option, arg] : args) {
						if (option == U"tag") {
							setArgument(op, option, n, arg);
						}
						if (option == U"path") {
							setArgument(op, option, fp, arg);
						}
						if (option == U"pos") {
							setArgument(op, option, p, arg);
						}
						if (option == U"fade") {
							setArgumentParse(op, option, fa, arg);
							fa = Max(fa, 0.0);
						}
						if (option == U"size") {
							setArgument(op, option, si, arg);
						}
						if (option == U"scale") {
							setArgumentParse(op, option, s, arg);
							s = Max(s, 0.0);
						}
						if (option == U"layer") {
							setArgument(op, option, l, arg);
						}
					}

					Image img{ fp }; //画像サイズの計算のために一時的にImageを作成
					si = si.isZero() ? img.size() : si;

					auto i = std::make_shared<Graphic>(Graphic{ fp, p, si, s, 0.0, l, n });

					std::function<std::function<bool()>()> f = [&, this, img = i, _n = n, _fa = fa]() {
						double time = 0.0;
						processStack.back().graphics.emplace_back(img);
						processStack.back().graphics.stable_sort_by([](const std::shared_ptr<Graphic>& a, const std::shared_ptr<Graphic>& b) { return a->getLayer() < b->getLayer(); });
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									(*itr)->setOpacity(t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								return true;
							}

							return false;
						};
					};

					addSubProcess(f());
				}

				if (op == U"setbtn") {
					String n = U"";
					FilePath nor = U"", hov = U"", cli = U"";
					Point p = Point::Zero();
					int32 l = 0;
					Size si = Size::Zero();
					String e = U"";
					String r = U"";
					String jt = U"";
					String li = scriptStack.back().reader.path();
					String d = U"";

					double fa = 0; // フェードインにかかる時間
					double s = 1.0;

					for (auto [option, arg] : args) {
						if (option == U"tag") {
							setArgument(op, option, n, arg);
						}
						if (option == U"path" || option == U"normal") {
							setArgument(op, option, nor, arg);
						}
						if (option == U"hover") {
							setArgument(op, option, hov, arg);
						}
						if (option == U"click") {
							setArgument(op, option, cli, arg);
						}
						if (option == U"pos") {
							setArgument(op, option, p, arg);
						}
						if (option == U"fade") {
							setArgumentParse(op, option, fa, arg);
							fa = Max(fa, 0.0);
						}
						if (option == U"size") {
							setArgument(op, option, si, arg);
							s = Max(s, 0.0);
						}
						if (option == U"scale") {
							setArgumentParse(op, option, s, arg);
						}
						if (option == U"layer") {
							setArgument(op, option, l, arg);
						}
						if (option == U"exp") {
							setArgument(op, option, e, arg);
						}
						if (option == U"role") {
							setArgument(op, option, r, arg);
						}
						if (option == U"jumptype") {
							setArgument(op, option, jt, arg);
						}
						if (option == U"link") {
							setArgument(op, option, li, arg);
						}
						if (option == U"dst") {
							setArgument(op, option, d, arg);
						}
					}

					Image img{ nor }; //画像サイズの計算のために一時的にImageを作成
					si = si.isZero() ? img.size() : si;

					auto i = std::make_shared<Button>(Button{ nor, hov, cli, p, si, s, 0.0, l, n, e, r, jt, li, d, this });

					std::function<std::function<bool()>()> f = [&, this, img = i, _n = n, _fa = fa]() {
						double time = 0.0;
						processStack.back().graphics.emplace_back(img);
						processStack.back().graphics.stable_sort_by([](const std::shared_ptr<Graphic>& a, const std::shared_ptr<Graphic>& b) { return a->getLayer() < b->getLayer(); });
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									(*itr)->setOpacity(t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								return true;
							}

							return false;
						};
					};

					addSubProcess(f());
				}

				if (op == U"change") {
					String n; // タグの名前
					FilePath fp;
					double fa = 0.0;

					for (auto [option, arg] : args) {
						if (option == U"tag") {
							setArgument(op, option, n, arg);
						}
						if (option == U"path") {
							setArgument(op, option, fp, arg);
						}
						if (option == U"fade") {
							setArgumentParse(op, option, fa, arg);
							fa = Max(fa, 0.0);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _t = n, _fp = fp, _fa = fa]() {
						double time = 0.0;
						for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
							if ((*itr)->getTag() == _t) {
								(*itr)->setNextPath(_fp);
								(*itr)->setNextTexture();
							}
						}
						return [&, this, time, tag = _t, filepath = _fp, fade = _fa]() mutable {
							for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									(*itr)->setOpacity(1.0 - t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
									if ((*itr)->getTag() == tag) {
										(*itr)->changeTexture();
										(*itr)->setOpacity(1.0);
									}
								}
								return true;
							}

							return false;
						};
					};

					addSubProcess(f());
				}

				if (op == U"moveto") {
					// x, y, duration, tag, ...option

					String n; // タグの名前
					Point d;
					double dur;
					double del = 0; // 開始時間のディレイ

					for (auto [option, arg] : args) {
						if (option == U"tag") {
							setArgument(op, option, n, arg);
						}
						if (option == U"dst") {
							setArgument(op, option, d, arg);
						}
						if (option == U"duration") {
							setArgumentParse(op, option, dur, arg);
						}
						if (option == U"delay") {
							setArgumentParse(op, option, del, arg);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _ta = n, _dst = d, _dur = dur, _del = del]() {
						double time = 0.0;
						Vec2 src, dst;

						for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
							if ((*itr)->getTag() == _ta) {
								src = (*itr)->getPosition();
								dst = _dst;
								// dst = src + _v;
							}
						}

						return [&, this, time, src, dst, duration = _dur, tag = _ta, delay = _del]() mutable {
							for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = duration > 0 ? Clamp((time - delay) / duration, 0.0, 1.0) : 1.0;
									(*itr)->setPosition(src.lerp(dst, t));

									if (proceedInput.down()) {
										(*itr)->setPosition(dst);
										return true;
									}
								}
							}

							time += Scene::DeltaTime();

							if (time < delay) {
								return false;
							}
							else if (time > delay + duration) {
								return true;
							}

							return false;
						};
					};

					addSubProcess(f());
				}

				if (op == U"moveby") {
					// x, y, duration, tag, ...option

					String n; // タグの名前
					Point by;
					double dur;
					double del = 0; // 開始時間のディレイ

					for (auto [option, arg] : args) {
						if (option == U"tag") {
							setArgument(op, option, n, arg);
						}
						if (option == U"by") {
							setArgument(op, option, by, arg);
						}
						if (option == U"duration") {
							setArgumentParse(op, option, dur, arg);
						}
						if (option == U"delay") {
							setArgumentParse(op, option, del, arg);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _ta = n, _by = by, _dur = dur, _del = del]() {
						double time = 0.0;
						Vec2 src, dst;

						for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
							if ((*itr)->getTag() == _ta) {
								src = (*itr)->getPosition();
								dst = src + _by;
							}
						}

						return [&, this, time, src, dst, duration = _dur, tag = _ta, delay = _del]() mutable {
							for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = duration > 0 ? Clamp((time - delay) / duration, 0.0, 1.0) : 1.0;
									(*itr)->setPosition(src.lerp(dst, t));

									if (proceedInput.down()) {
										(*itr)->setPosition(dst);
										return true;
									}
								}
							}

							time += Scene::DeltaTime();

							if (time < delay) {
								return false;
							}
							else if (time > delay + duration) {
								return true;
							}

							return false;
						};
					};

					addSubProcess(f());
				}

				if (op == U"delete") {
					// tag, ...option
					String n; // タグの名前
					double fa = 0; // フェードアウトにかかる時間

					for (auto [option, arg] : args) {
						if (option == U"tag") {
							setArgument(op, option, n, arg);
						}
						if (option == U"fade") {
							setArgumentParse(op, option, fa, arg);
							fa = Max(fa, 0.0);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _n = n, _fa = fa]() {
						double time = 0.0;
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									(*itr)->setOpacity(1.0 - t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ) {
									if ((*itr)->getTag() == tag) {
										itr = processStack.back().graphics.erase(itr);
									}
									else {
										++itr;
									}
								}
								return true;
							}

							return false;
						};
					};

					addSubProcess(f());
				}

				if (op == U"music") {
					//filepath, ...option
					FilePath fp;
					for (auto [option, arg] : args) {
						if (option == U"path") {
							setArgument(op, option, fp, arg);
						}
					}
					Audio a{ Audio::Stream, fp };

					std::function<std::function<bool()>()> f = [&, this, _a = a]() {
						double time = 0.0;
						return [&, this, audio = _a, time]() mutable {
							nowMusic = audio;
							nowMusic.play(MixBus0);
							return true;
						};
					};

					addSubProcess(f());
				}

				if (op == U"sound") {
					//filepath, ...option
					FilePath fp;
					for (auto [option, arg] : args) {
						if (option == U"path") {
							setArgument(op, option, fp, arg);
						}
					}
					Audio a{ Audio::Stream, fp };
					
					std::function<std::function<bool()>()> f = [&, this, _a = a]() {
						double time = 0.0;
						return [&, this, audio = _a, time]() mutable {
							processStack.back().sounds.emplace_back(audio);
							processStack.back().sounds.back().play(MixBus1);
							return true;
						};
					};

					addSubProcess(f());
				}

				if (op == U"volmusic") {
					double v = 1.0;
					for (auto [option, arg] : args) {
						if (option == U"volume") {
							setArgument(op, option, v, arg);
						}
					}
					GlobalAudio::BusSetVolume(MixBus0, v);
				}

				if (op == U"volsound") {
					double v = 1.0;
					for (auto [option, arg] : args) {
						if (option == U"volume") {
							setArgument(op, option, v, arg);
						}
					}
					GlobalAudio::BusSetVolume(MixBus1, v);
				}

				if (op == U"wait") {
					// duration
					double t = 0.0;
					for (auto [option, arg] : args) {
						if (option == U"time") {
							setArgumentParse(op, option, t, arg);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _t = t]() {
						double time = 0.0;
						return [&, this, time, duration = _t]() mutable {
							if (time >= duration) {
								return true;
							}
							time += Scene::DeltaTime();

							return false;
						};
					};

					setMainProcess(f());
				}

				if (op == U"choice") {
					String text;
					String dst;
					String type;
					Point pos = Point{ 0, 0 };
					Point size = Point{ 0, 0 };
					String normal = U"", hover = U"", click = U"";

					// 引数を処理
					for (auto [option, arg] : args) {
						if (option == U"text") {
							setArgument(op, option, text, arg);
						}
						if (option == U"dst") {
							setArgument(op, option, dst, arg);
						}
						if (option == U"type") {
							setArgument(op, option, type, arg);
						}
						if (option == U"pos") {
							setArgument(op, option, pos, arg);
						}
						if (option == U"size") {
							setArgument(op, option, size, arg);
						}
						if (option == U"normal") {
							setArgument(op, option, normal, arg);
						}
						if (option == U"hover") {
							setArgument(op, option, hover, arg);
						}
						if (option == U"click") {
							setArgument(op, option, click, arg);
						}

					}
					Choice choice;
					if (type == U"image") {
						choice = Choice{ text, dst, type, pos, size, normal, hover, click };
					}

					selections.emplace_back(choice);
				}

				if (op == U"goto") {
					// distination
					FilePath link = scriptStack.back().reader.path();
					String dst;
					for (auto [option, arg] : args) {
						if (option == U"link") {
							setArgument(op, option, link, arg);
						}
						if (option == U"dst") {
							setArgument(op, option, dst, arg);
						}
					}

					auto [reader, i] = getDistination(link, dst);
					scriptStack.pop_back();
					scriptStack << ScriptData{reader, i, i};
					if (isStopNow) {
						processStack.back().mainStack.pop_front();
						initMainProcess();
						isStopNow = false;
					}
				}

				if (op == U"stop") {
					isStopNow = true;

					std::function<bool()> f = [&, this]() { return false; };
					setMainProcess(f);
				}

				if (op == U"if") {
					ifSkipFlag = false;

					String e;
					for (auto [option, arg] : args) {
						if (option == U"exp") {
							setArgument(op, option, e, arg);
						}
					}

					if (evalExpression(e)) {
						ifSkipFlag = true;
					}
					else {
						skipLineForIf();
					}
				}

				if (op == U"elif") {
					String e;
					for (auto [option, arg] : args) {
						if (option == U"exp") {
							setArgument(op, option, e, arg);
						}
					}

					if (!ifSkipFlag && evalExpression(e)) {
						ifSkipFlag = true;
					}
					else {
						skipLineForIf();
					}
				}

				if (op == U"else") {
					if (!ifSkipFlag) {
						ifSkipFlag = true;
					}
					else {
						skipLineForIf();
					}
				}

				if (op == U"endif") {
					ifSkipFlag = false;
				}

				if (op == U"scenechange") {
					// filename
					FilePath fp;
					double d; // シーン切り替えににかかる時間
					for (auto [option, arg] : args) {
						if (option == U"link") {
							setArgument(op, option, fp, arg);
						}
						if (option == U"duration") {
							setArgumentParse(op, option, d, arg);
							d = Max(d, 0.0);
						}
					}

					std::function<bool()> f = [&, this, file = fp, dur = d]() {
						getData().scriptPath = file;
						changeScene(GameScene::Engine, dur * 1.0s);
						return true;
					};

					setMainProcess(f);
				}

				if (op == U"call") {
					// filename
					FilePath link = scriptStack.back().reader.path();
					String dst;
					for (auto [option, arg] : args) {
						if (option == U"link") {
							setArgument(op, option, link, arg);
						}
						if (option == U"dst") {
							setArgument(op, option, dst, arg);
						}
					}

					auto [reader, i] = getDistination(link, dst);
					scriptStack << ScriptData{reader, i, i};
				}

				if (op == U"return") {
					scriptStack.pop_back();
				}

				if (op == U"sleeppr") {
					bool sf = false;
					for (auto [option, arg] : args) {
						if (option == U"save") {
							setArgument(op, option, sf, arg);
						}
					}

					for (auto itr = processStack.back().sounds.begin(); itr != processStack.back().sounds.end(); ++itr) {
						itr->pause();
					}

					processStack << Process{sf};
				}

				if (op == U"awakepr") {
					processStack.pop_back();

					for (auto itr = processStack.back().sounds.begin(); itr != processStack.back().sounds.end(); ++itr) {
						itr->play();
					}
				}

				if (op == U"input") {
					String who;
					String down_exp, press_exp, up_exp;
					String jumptype = U"";
					String link = scriptStack.back().reader.path();
					String dst = U"";
					for (auto [option, arg] : args) {
						if (option == U"who") {
							setArgument(op, option, who, arg);
						}
						if (option == U"down_exp") {
							setArgument(op, option, down_exp, arg);
						}
						if (option == U"press_exp") {
							setArgument(op, option, press_exp, arg);
						}
						if (option == U"up_exp") {
							setArgument(op, option, up_exp, arg);
						}
						if (option == U"jumptype") {
							setArgument(op, option, jumptype, arg);
						}
						if (option == U"link") {
							setArgument(op, option, link, arg);
						}
						if (option == U"dst") {
							setArgument(op, option, dst, arg);
						}
					}

					std::function<bool()> f = [&, this, input = ElephantLib::stringToInput[who], down_exp, press_exp, up_exp, jumptype, link, dst]() {
						if (input.down()) {
							interprete(down_exp);

							auto [reader, i] = getDistination(link, dst);
							if (jumptype) {
								if (jumptype == U"goto") {
									scriptStack.pop_back();
								}
								scriptStack << ScriptData{reader, i, i};
								if (isStopNow) {
									processStack.back().mainStack.pop_front();
									initMainProcess();
									isStopNow = false;
								}
							}
						}
						if (input.pressed()) {
							interprete(press_exp);
						}
						if (input.up()) {
							interprete(up_exp);
						}
						return false;
					};

					addSubProcess(f);
				}

				if (op == U"get_inputduration") {
					String who;
					String var;
					for (auto [option, arg] : args) {
						if (option == U"who") {
							setArgument(op, option, who, arg);
						}
						if (option == U"var") {
							setArgument(op, option, var, arg);
						}
					}

					std::function<bool()> f = [&, this, input = ElephantLib::stringToInput[who], var]() {
						if (input.up()) {
							interprete(U"{} = {}"_fmt(var, input.pressedDuration()));
						}
						return false;
					};

					addSubProcess(f);
				}

				if (op == U"get_wheelscroll") {
					String var;
					for (auto [option, arg] : args) {
						if (option == U"var") {
							setArgument(op, option, var, arg);
						}
					}

					std::function<bool()> f = [&, this, var]() {
						interprete(U"{} = {}"_fmt(var, Mouse::Wheel()));
						return false;
					};

					addSubProcess(f);
				}

				if (op == U"exit") {
					std::function<bool()> f = [&, this]() {
						System::Exit();
						return true;
					};
					setMainProcess(f);
				}
			}

			operateLine = U"";
		}

	} while (!readStopFlag);
}

std::pair<TextReader, size_t> Engine::getDistination(FilePath path, String dst) {
	TextReader reader{ path };
	if (!reader) throw Error{U"Failed to open `{}`"_fmt(path)};

	size_t i = 0;
	if (dst) {
		String line;
		bool flag = false;
		while (reader.readLine(line)) {
			i++;
			const auto skip = RegExp(U" *@(.+) *").match(line);
			if (!skip.isEmpty()) {
				if (skip[1].value() == dst) {
					flag = true;
					break;
				}
			}
		}
		if (!flag) throw Error{U"Not exist `@{}`"_fmt(dst)};
	}

	return std::make_pair(reader, i);
}

void Engine::interprete(String code) {
	auto tokens = Lexer().init(code).tokenize();
	auto blk = Parser().init(tokens).block();
	auto[v, sv, gv] = Interpreter().init(blk, getData().tmp_vars, getData().savedata_vars, getData().system_vars).run();
	getData().tmp_vars = v;
	getData().savedata_vars = sv;
	getData().system_vars = gv;
}

std::any Engine::getValueFromVariable(String var) {
	auto tokens = Lexer().init(var).tokenize();
	auto blk = Parser().init(tokens).block();
	return Interpreter().init(blk, getData().tmp_vars, getData().savedata_vars, getData().system_vars).getValue();
}

bool Engine::boolCheck(std::any value) {
	if (auto p = std::any_cast<int32>(&value)) {
		return *p != 0;
	}
	else if (auto p = std::any_cast<bool>(&value)) {
		return *p;
	}
	else {
		throw Error{U"boolCheck Error"};
	}
}

bool Engine::evalExpression(String expr) {
	auto tokens = Lexer().init(expr).tokenize();
	auto blk = Parser().init(tokens).block();
	return Interpreter().init(blk, getData().tmp_vars, getData().savedata_vars, getData().system_vars).evalExpression();
}

void Engine::skipLineForIf() {
	TextReader reader{ scriptStack.back().reader.path() };
	String line;
	size_t i = 0;
	for (size_t j = 0; reader.readLine(line); j++) {
		if (j < scriptStack.back().index) continue;

		auto skip = RegExp(U" *#(elif|else|endif).+").match(line);
		if (!skip.isEmpty()) {
			break;
		}
		i++;
	}

	String skipLine;
	for (; i > 0; i--) {
		readScriptLine(skipLine);
	}
}

void Engine::initMainProcess() {
	processStack.back().mainStack << [&, this]() { return true; };
}

bool Engine::exeMainProcess() {
	if ((processStack.back().mainStack.front())()) {
		processStack.back().mainStack.pop_front();
	}
	return processStack.back().mainStack.isEmpty();
}

void Engine::resetTextWindow(Array<String> strs) {
	for (auto itr = strs.begin(); itr != strs.end(); ++itr) {
		scenarioLog << *itr;
	}

	sentenceStorage = U"";
	processStack.back().windowText = WindowText{ messagePos, namePos, defaultMessageSize, defaultNameSize, layer };
}

void Engine::setSaveVariable() {
	for (auto itr = scriptStack.begin(); itr != scriptStack.end(); itr++) {
		itr->saveIndex = itr->index;
	}

	Array<Graphic> saveGraphics = {};
	for (auto g : processStack.back().graphics) {
		if (typeid((*g)) == typeid(Graphic)) {
			saveGraphics << *g;
		}
	}
	forSaveGraphics = saveGraphics;
}

void Engine::update() {
	// Printをクリア
	ClearPrint();

	if (KeyF5.down()) {
		isShowingLog = !isShowingLog;
		isShowingVariable = false;
	}
	else if (KeyF6.down()) {
		isShowingVariable = !isShowingVariable;
		isShowingLog = false;
	}

	// ログ見せの場合は何も動かさない
	if (isShowingLog || isShowingVariable) {
		return;
	}


	// サブ処理(並列可能処理)実行
	for (auto itr = processStack.back().subList.begin(); itr != processStack.back().subList.end();) {
		if ((*itr)()) {
			itr = processStack.back().subList.erase(itr);
		}
		else {
			++itr;
		}
	}

	// 画像系の逐次処理
	for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
		(*itr)->update();
	}

	// 音声が停止したら配列から削除
	for (auto itr = processStack.back().sounds.begin(); itr != processStack.back().sounds.end();) {
		if (!itr->isPlaying()) {
			itr = processStack.back().sounds.erase(itr);
		}
		else {
			++itr;
		}
	}

	// メイン処理実行
	if (exeMainProcess()) {
		// 処理が終わったら次の処理を読み込む
		readScript();
	}




	// クイックセーブ
	if (KeyS.down()) {
		Serializer<BinaryWriter> writer{ savefile };
		if (!writer) throw Error{U"Failed to open SaveFile"};

		// script
		Array<FilePath> scriptPathList;
		Array<size_t> scriptSaveIndexList;
		for (auto s : scriptStack) {
			scriptPathList << s.reader.path();
			scriptSaveIndexList << s.saveIndex;
		}
		writer(scriptPathList);
		writer(scriptSaveIndexList);
		Console << scriptSaveIndexList;

		writer(operateLine);
		writer(nowName);

		writer(forSaveGraphics); //画像をシリアライズして保存はできないので、画像そのものではなく画像情報を保存

		// 変数書き込み
		/*
		Array<VariableKey> k_tmp;
		Array<v_type> v_tmp;
		Array<String> t_tmp;
		for (auto [k, v] : variable) {
			k_tmp << k;
			v_tmp << v;
			if (auto* p = std::get_if<int32>(&v)) {
				t_tmp << U"int";
			}
			else if (auto* p = std::get_if<String>(&v)) {
				t_tmp << U"string";
			}
			else if (auto* p = std::get_if<double>(&v)) {
				t_tmp << U"double";
			}
			else if (auto* p = std::get_if<bool>(&v)) {
				t_tmp << U"bool";
			}
			else if (auto* p = std::get_if<Point>(&v)) {
				t_tmp << U"point";
			}
		}

		writer(t_tmp); //保存する変数の型のリスト
		for (auto i : step(k_tmp.size())) {
			writer(k_tmp[i]); // 変数名
			std::visit([&writer](auto& x) {
				writer(x); // 値
			}, v_tmp[i]);
		}
		// -------
		*/

		Print << U"Saved!";
	}

	// クイックロード
	if (KeyL.down()) {
		Deserializer<BinaryReader> reader{ savefile };
		if (!reader) throw Error{ U"Failed to open SaveFile" };

		scriptStack = {};
		Array<FilePath> scriptPathList;
		Array<size_t> scriptSaveIndexList;
		reader(scriptPathList);
		reader(scriptSaveIndexList);
		Console << scriptSaveIndexList;
		for (auto i : step(scriptPathList.size())) {
			TextReader tr{ scriptPathList[i] };
			size_t index = scriptSaveIndexList[i];
			if (!tr) throw Error{ U"Failed to open `{}`"_fmt(scriptPathList[i]) };

			for (auto j : step(index)) {
				tr.readLine();
			}

			scriptStack << ScriptData{tr, index, index};
		}

		reader(operateLine);
		reader(nowName);

		reader(forSaveGraphics); //画像情報をロード
		processStack.back().graphics.clear();
		// graphics.clear();
		for (auto itr = forSaveGraphics.begin(); itr != forSaveGraphics.end(); ++itr) {
			processStack.back().graphics << std::make_shared<Graphic>(*itr);
		}

		for (auto itr = processStack.back().graphics.begin(); itr != processStack.back().graphics.end(); ++itr) {
			(*itr)->setTexture();
		}

		/*
		variable.clear();
		Array<String> typeList;
		reader(typeList); //変数リスト
		for (String t : typeList) {
			VariableKey k;
			v_type v;

			reader(k);
			if (t == U"int") {
				int32 _;
				reader(_);
				v = _;
			}
			else if (t == U"string") {
				String _;
				reader(_);
				v = _;
			}
			else if (t == U"bool") {
				bool _;
				reader(_);
				v = _;
			}
			else if (t == U"double") {
				double _;
				reader(_);
				v = _;
			}
			else if (t == U"point") {
				Point _;
				reader(_);
				v = _;
			}

			variable[k] = v;
		}
		*/

		// 実行中の各種処理を初期化
		resetTextWindow({});

		processStack.back().subList.clear();
		// subProcesses.clear();

		Print << U"Loaded!";

		// 改めて読み込み開始
		readScript();
	}

	if (KeyR.down()) {
		changeScene(GameScene::Engine);
	}
}

void Engine::draw() const {
	if (isShowingLog) {
		Print << U"Log";
		for (auto itr = scenarioLog.begin(); itr != scenarioLog.end(); ++itr) {
			Print << *itr;
		}
		return;
	}

	if (isShowingVariable) {
		Print << U"-------------";
		Print << U"Temporary Variable";
		for (auto [k, v] : getData().tmp_vars) {
			Print << k << U" : " << getVariableList(v->value);
		}
		Print << U"";
		Print << U"Savedata Variable";
		for (auto [k, v] : getData().savedata_vars) {
			Print << k << U" : " << getVariableList(v->value);
		}
		Print << U"";
		Print << U"System Variable";
		for (auto [k, v] : getData().system_vars) {
			Print << k << U" : " << getVariableList(v->value);
		}
		Print << U"-------------";
		return;
	}

	
	bool messageDrawFlag = false;
	int32 meslayer = processStack.back().windowText.layer;
	for (auto i : step(processStack.size())) {
		int32 nowlayer = INT32_MIN, prevlayer = INT32_MIN;
		for (auto itr = processStack.at(i).graphics.begin(); itr != processStack.at(i).graphics.end(); ++itr) {
			prevlayer = nowlayer;
			nowlayer = (*itr)->getLayer();
			if (!messageDrawFlag && i == processStack.size() - 1) {
				if (prevlayer <= meslayer && meslayer < nowlayer) {
					Console << U"message";
					processStack.back().windowText.draw();
					messageDrawFlag = true;
				}
			}

			(*itr)->draw();
		}
	}
	if (!messageDrawFlag) {
		processStack.back().windowText.draw();
	}


	if (isShowSelection) {
		for (auto itr = selections.begin(); itr != selections.end(); ++itr) {
			itr->draw();
		}
	}

	// フェードインアウトの描画
	fadeProcess();
}

String Engine::getVariableList(std::any arg) const {
	if (auto p = std::any_cast<int32>(&arg)) {
		return Format(*p);
	}
	else if (auto p = std::any_cast<double>(&arg)) {
		return Format(*p);
	}
	else if (auto p = std::any_cast<String>(&arg)) {
		return *p;
	}
	else if (auto p = std::any_cast<Array<std::any>>(&arg)) {
		String s = U"{";
		for (int32 i = 0; i < p->size(); i++) {
			s += getVariableList(p->at(i));
			if (i < p->size() - 1) {
				s += U",";
			}
		}
		s += U"}";
		return s;
	}
	else {
		return U"Function";
	}
}

void Engine::WindowText::draw() const {
	FontAsset(U"Medium{}"_fmt(defaultNameSize))(name).draw(namePos, Palette::White);
	FontAsset(U"Medium{}"_fmt(defaultMessageSize))(message.substr(0, index + 1)).draw(messagePos, Palette::White);
}

void Engine::Graphic::update() {}

void Engine::Graphic::draw() const {
	if (nextTexture) {
		texture.resized(size).scaled(scale).draw(position, ColorF{ 1.0, opacity });
		nextTexture.resized(size).scaled(scale).draw(position, ColorF{ 1.0, 1.0 - opacity });
	}
	else {
		texture.resized(size).scaled(scale).draw(position, ColorF{ 1.0, opacity });
	}
}

void Engine::Button::update() {
	if (Rect{ position.asPoint(), size }.mouseOver()) {
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	if (Rect{ position.asPoint(), size }.leftClicked()) {
		if (role) {
			if (role == U"auto") {
				engine->getData().isAuto = !engine->getData().isAuto;
			}
			else {
				throw Error{U"role error"};
			}
		}
		else {
			engine->interprete(expr);

			auto [reader, i] = engine->getDistination(link, dst);
			if (jumptype) {
				if (jumptype == U"goto") {
					engine->scriptStack.pop_back();
				}
				engine->scriptStack << ScriptData{reader, i, i};
				if (engine->isStopNow) {
					engine->processStack.back().mainStack.pop_front();
					engine->initMainProcess();
					engine->isStopNow = false;
				}
			}
		}
	}
}

void Engine::Button::draw() const {
	TextureRegion tr;
	if (click && region().leftPressed()) {
		tr = click.resized(size);
	}
	else if (hover && region().mouseOver()) {
		tr = hover.resized(size);
	}
	else {
		tr = texture.resized(size);
	}

	if (texture || hover || click) tr.draw(position);
}
