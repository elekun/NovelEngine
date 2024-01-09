﻿#include "stdafx.h"
#include "Engine.hpp"
#include "ElephantLib.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"

using namespace ElephantLib;


Engine::Engine(const InitData& init) : IScene{ init } {

	Console << U"script path :" << getData().scriptPath;

	effectIndex = 0;
	renderTexture = MSRenderTexture{ Scene::Size() };

	cb->level = 0.1;

	TextReader reader{ getData().scriptPath };
	if (!reader) {
		throw Error{ U"Failed to open `{}`"_fmt(getData().scriptPath) };
	}
	scriptStack = {};
	scriptStack << ScriptData{reader, 0};

	readStopFlag = false;


	mainProcess = [&, this]() { return true; };
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

void Engine::readSetting() {
	TextReader reader{ settingfile };
	if (!reader) {
		throw Error{ U"Failed to open setting file." };
	}
	Array<String> si = { U"textSize", U"textPosition", U"textRect", U"nameSize", U"namePosition" };
	String list = U"";
	for (auto [j, p] : Indexed(si)) {
		list += p;
		if (j != si.size() - 1) { list += U"|"; }
	}

	String line;
	while (reader.readLine(line)) {
		// 設定ファイルの項目とその引数の読み込み
		const auto setting = RegExp(U" *({}): *(.+)"_fmt(list)).search(line);
		StringView st = setting[1].value(); // 処理のタグ
		const auto tmp = String{ setting[2].value() }.split(',');
		Array<String> args = {};
		for (auto i : tmp) {
			String t = String{ RegExp(U" *(.+) *").search(i)[1].value() }; //前後のスペースを削除
			if (t != U"") {
				args << t;
			}
		}

		if (st == U"textSize") {
			textSize = Max(Parse<int32>(args[0]), 1);
		}
		if (st == U"nameSize") {
			nameSize = Max(Parse<int32>(args[0]), 1);
		}
		if (st == U"textPosition") {
			textPos = Point{ Parse<int32>(args[0]), Parse<int32>(args[1]) };
		}
		if (st == U"namePosition") {
			namePos = Point{ Parse<int32>(args[0]), Parse<int32>(args[1]) };
		}
	}
}

void Engine::readScriptLine(String& s) {
	if (scriptStack.back().reader.readLine(s)) {
		scriptStack.back().index++;
	}
	else {
		s = U"";
		readStopFlag = true;
		return;
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
				value = String{ mr[1].value() };
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
		mainProcess = f;
		readStopFlag = true;
	};

	// 命令
	Array<String> pn = {
		U"setting", U"start", U"name",
		U"setimg", U"setbtn", U"change", U"moveto", U"moveby", U"delete",
		U"music", U"sound",
		U"wait", U"goto", U"scenechange", U"exit",
		U"inculude", U"call"
	};
	String proc = U"";
	for (auto [j, p] : Indexed(pn)) {
		proc += p;
		if (j != pn.size() - 1) { proc += U"|"; }
	}

	// 文章を読み込む処理
	do {
		// 各種処理
		if (operateLine != U"") {

			const auto operate = RegExp(U"({})\\[(.*)\\]"_fmt(proc)).search(operateLine);
			if (!operate.isEmpty()) {

				StringView op =	operate[1].value(); // 処理のタグ
				Array<std::pair<String, v_type>> args = getArgs(operate[2].value());

				if (op == U"setting") {
					FilePath fp = U""; //設定ファイルのパス
					for (auto [option, arg] : args) {
						if (option == U"path") {
							setArgument(op, option, fp, arg);
						}
					}

					std::function<bool()> f = [&, this, p = fp]() {
						settingfile = p;
						readSetting();
						return true;
					};
					setMainProcess(f);
				}

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
					subProcesses.emplace_back(f());

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
						graphics.emplace_back(img);
						graphics.stable_sort_by([](const std::shared_ptr<Graphic>& a, const std::shared_ptr<Graphic>& b) { return a->getLayer() < b->getLayer(); });
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
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

					subProcesses.emplace_back(f());
				}

				if (op == U"setbtn") {
					String n = U"";
					FilePath nor = U"", hov = U"", cli = U"";
					Point p = Point::Zero();
					int32 l = 0;
					Size si = Size::Zero();
					String e = U"";
					String r = U"";

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
					}

					Image img{ nor }; //画像サイズの計算のために一時的にImageを作成
					si = si.isZero() ? img.size() : si;

					auto i = std::make_shared<Button>(Button{ nor, hov, cli, p, si, s, 0.0, l, n, e, r, this });

					std::function<std::function<bool()>()> f = [&, this, img = i, _n = n, _fa = fa]() {
						double time = 0.0;
						graphics.emplace_back(img);
						graphics.stable_sort_by([](const std::shared_ptr<Graphic>& a, const std::shared_ptr<Graphic>& b) { return a->getLayer() < b->getLayer(); });
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
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

					subProcesses.emplace_back(f());
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
						for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
							if ((*itr)->getTag() == _t) {
								(*itr)->setNextPath(_fp);
								(*itr)->setNextTexture();
							}
						}
						return [&, this, time, tag = _t, filepath = _fp, fade = _fa]() mutable {
							for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									(*itr)->setOpacity(1.0 - t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
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

					subProcesses.emplace_back(f());
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

						for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
							if ((*itr)->getTag() == _ta) {
								src = (*itr)->getPosition();
								dst = _dst;
								// dst = src + _v;
							}
						}

						return [&, this, time, src, dst, duration = _dur, tag = _ta, delay = _del]() mutable {
							for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
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
					subProcesses.emplace_back(f());
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

						for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
							if ((*itr)->getTag() == _ta) {
								src = (*itr)->getPosition();
								dst = src + _by;
							}
						}

						return [&, this, time, src, dst, duration = _dur, tag = _ta, delay = _del]() mutable {
							for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
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
					subProcesses.emplace_back(f());
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
							for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
								if ((*itr)->getTag() == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									(*itr)->setOpacity(1.0 - t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								for (auto itr = graphics.begin(); itr != graphics.end(); ) {
									if ((*itr)->getTag() == tag) {
										itr = graphics.erase(itr);
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

					subProcesses.emplace_back(f());

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
							nowMusic.play();
							return true;
						};
					};
					subProcesses.emplace_back(f());
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
							sounds.emplace_back(audio);
							sounds.back().play();
							return true;
						};
					};
					subProcesses.emplace_back(f());
				}

				if (op == U"wait") {
					// duration
					double d = 0.0;
					for (auto [option, arg] : args) {
						if (option == U"duration") {
							setArgumentParse(op, option, d, arg);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _d = d]() {
						double time = 0.0;
						return [&, this, time, duration = _d]() mutable {
							if (time >= duration) {
								return true;
							}
							time += Scene::DeltaTime();

							return false;
						};
					};

					setMainProcess(f());
				}

				if (op == U"goto") {
					// distination
					String d;
					for (auto [option, arg] : args) {
						if (option == U"dst") {
							setArgument(op, option, d, arg);
						}
					}
					FilePath fp = scriptStack.back().reader.path();

					std::function<bool()> f = [&, this, dst = d, path = fp]() {
						TextReader reader{ path };
						if (!reader) {
							throw Error{U"Failed to open `{}`"_fmt(path)};
						}

						String line;
						size_t i = 0;
						bool flag = false;
						while (reader.readLine(line)) {
							i++;
							const auto skip = RegExp(U" *@(.+) *").match(line);
							if (!skip.isEmpty()) {
								if (skip[1].value() == dst) {
									scriptStack.pop_back();
									scriptStack << ScriptData{reader, i};
									flag = true;
								}
							}
						}

						if (!flag) throw Error{U"Not exist `@{}`"_fmt(dst)};

						return true;
					};

					setMainProcess(f);
				}

				if (op == U"scenechange") {
					// filename
					FilePath fp;
					double d; // シーン切り替えににかかる時間
					for (auto [option, arg] : args) {
						if (option == U"path") {
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

				if (op == U"include") {

				}

				/* マクロ挿入用、readScript()の処理を変える必要があるので後回し
				* 1. struct{scriptIndex, scriptLines} の配列を作成
				* 2. スタック（First in Last out）でscriptLinesにアクセス、処理を実行
				* 3. 最後まで行ったらスタックから出す
				*/
				if (op == U"call") {
					// filename
					FilePath fp;
					for (auto [option, arg] : args) {
						if (option == U"path") {
							setArgument(op, option, fp, arg);
						}
					}

					std::function<bool()> f = [&, this, file = fp]() {
						readScript();
						return true;
					};

					setMainProcess(f);
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
		auto tmp = RegExp(U" *[#@](.*)").search(line);

		// 処理が挟まるか空行ならひとかたまりの文章として処理
		if (!tmp.isEmpty() || line == U"") {
			if (!tmp.isEmpty()) {
				operateLine = tmp[1].value();
			}

			if (sentenceStorage != U"") {
				if (sentenceStorage.back() == '\n') {
					nowWindowText.sentence = sentenceStorage.substr(0, sentenceStorage.size() - 1); // 改行削除
				}
				nowWindowText.indexCT = 0.05;

				std::function<bool()> f = [&, this]() {

					nowWindowText.time += Scene::DeltaTime();

					if (getData().isAuto) {
						if (nowWindowText.time >= nowWindowText.indexCT * 20) {
							resetTextWindow({ nowWindowText.sentence });
							setSaveVariable();
							return true;
						}
					}
					else {
						// 左クリックorスペースで
						if (proceedInput.down()) {
							// 最後まで文字が表示されているなら次に行く
							if (nowWindowText.index >= nowWindowText.sentence.size() - 1) {
								resetTextWindow({ nowWindowText.sentence });
								setSaveVariable();
								return true;
							}
							// まだ最後まで文字が表示されていないなら最後まで全部出す
							else {
								nowWindowText.index = nowWindowText.sentence.size() - 1;
							}
						}
					}

					// 選択肢が用意されているときの処理
					// 文字が最後まで表示されたら
					if (!selections.isEmpty() && nowWindowText.index >= nowWindowText.sentence.size() - 1) {
						// 選択肢を表示しておく
						isShowSelection = true;
						return true;
					}

					// 1文字ずつ順番に表示する
					if (nowWindowText.time >= nowWindowText.indexCT) {
						if (nowWindowText.index < nowWindowText.sentence.size() - 1) {
							nowWindowText.index++;
							nowWindowText.time = 0;
						}
					}

					return false;
				};
				setMainProcess(f);
			}
			else {
				// nowSentence に何も文字列が入ってないとき
				if (!selections.isEmpty()) {
					std::function<bool()> f = [&, this]() {
						// 選択肢を表示
						isShowSelection = true;
						return true;
					};
					setMainProcess(f);
				}
			}
		}
		else {
			const auto select = RegExp(U" *\\*(.*)\\[(.*)\\]").search(line);
			if (!select.isEmpty()) {
				// 選択肢を読み込んだら選択肢リストに追加
				String text{ select[1].value() };
				Array<std::pair<String, v_type>> args = getArgs(select[2].value());

				String dst;
				String type;
				Point pos = Point{ 0, 0 };
				Point size = Point{ 0, 0 };
				String normal = U"", hover = U"", click = U"";

				// 引数を処理
				for (auto [option, arg] : args) {
					if (option == U"dst") {
						setArgument(U"choice", option, dst, arg);
					}
					if (option == U"type") {
						setArgument(U"choice", option, type, arg);
					}
					if (option == U"pos") {
						setArgument(U"choice", option, pos, arg);
					}
					if (option == U"size") {
						setArgument(U"choice", option, size, arg);
					}
					if (option == U"normal") {
						setArgument(U"choice", option, normal, arg);
					}
					if (option == U"hover") {
						setArgument(U"choice", option, hover, arg);
					}
					if (option == U"click") {
						setArgument(U"choice", option, click, arg);
					}

				}

				if (type == U"image") {
					selections.emplace_back(Choice{ text, dst, type, pos, size, normal, hover, click });
				}

				isShowSelection = false;
			}
			else {
				// 選択肢でないならウィンドウに表示する（つまり普通の）文字列として処理
				sentenceStorage += line + U"\n";
			}
		}

	} while (!readStopFlag);


	// 選択肢処理用(うまくまとめられなかったので例外的な処理)
	if (isShowSelection) {

		std::function<std::function<bool()>()> f = [&, this]() {
			double _time = 0.0;
			bool isClicked = false;
			return [&, this, _time]() mutable {
				_time += Scene::DeltaTime();

				for (auto itr = selections.begin(); itr != selections.end(); ++itr) {
					if (Rect{ itr->pos, itr->size }.leftReleased() && isClicked) {
						resetTextWindow({ nowWindowText.sentence, U"choiced : " + itr->text });
						setSaveVariable();
						isShowSelection = false;

						FilePath fp = scriptStack.back().reader.path();
						TextReader reader{ fp };
						if (!reader) throw Error{U"Failed to open `{}`"_fmt(fp)};

						String line;
						size_t i = 0;
						bool flag = false;
						while (reader.readLine(line)) {
							i++;
							const auto skip = RegExp(U" *@(.+) *").match(line);
							if (!skip.isEmpty()) {
								if (skip[1].value() == itr->dst) {
									scriptStack.pop_back();
									scriptStack << ScriptData{reader, i};
									flag = true;
								}
							}
						}

						if(!flag) throw Error{U"Not exist `@{}`"_fmt(itr->dst)};

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

void Engine::interprete(String code) {
	auto tokens = Lexer().init(code).tokenize();
	auto blk = Parser().init(tokens).block();
	auto[v, sv] = Interpreter().init(blk, this->vars, getData().systemVars).run();
	this->vars = v;
	getData().systemVars = sv;
}

std::any Engine::getValueFromVariable(String var) {
	auto tokens = Lexer().init(var).tokenize();
	auto blk = Parser().init(tokens).block();
	return Interpreter().init(blk, this->vars, getData().systemVars).getValue();
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

void Engine::resetTextWindow(Array<String> strs) {
	for (auto itr = strs.begin(); itr != strs.end(); ++itr) {
		scenarioLog << *itr;
	}

	sentenceStorage = U"";
	nowWindowText = WindowText{};
}

void Engine::setSaveVariable() {
	Array<Graphic> saveGraphics = {};
	for (auto g : graphics) {
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
	for (auto itr = subProcesses.begin(); itr != subProcesses.end();) {
		if ((*itr)()) {
			itr = subProcesses.erase(itr);
		}
		else {
			++itr;
		}
	}


	// メイン処理実行
	if (mainProcess()) {
		// 処理が終わったら次の処理を読み込む
		readScript();
	}

	// 画像系の逐次処理
	for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
		(*itr)->update();
	}

	// 音声が停止したら配列から削除
	for (auto itr = sounds.begin(); itr != sounds.end();) {
		if (!itr->isPlaying()) {
			itr = sounds.erase(itr);
		}
		else {
			++itr;
		}
	}

	// クイックセーブ
	if (KeyS.down()) {
		Serializer<BinaryWriter> writer{ savefile };
		if (!writer) throw Error{U"Failed to open SaveFile"};

		// script
		// writer(forSaveIndex);
		Array<FilePath> scriptPathList;
		Array<size_t> scriptIndexList;
		for (auto s : scriptStack) {
			scriptPathList << s.reader.path();
			scriptIndexList << s.index;
		}
		writer(scriptPathList);
		writer(scriptIndexList);

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
		Array<size_t> scriptIndexList;
		reader(scriptPathList);
		reader(scriptIndexList);
		for (auto i : step(scriptPathList.size())) {
			TextReader tr{ scriptPathList[i] };
			size_t index = scriptIndexList[i];
			if (!tr) throw Error{ U"Failed to open `{}`"_fmt(scriptPathList[i]) };

			for (auto j : step(index)) {
				tr.readLine();
			}

			scriptStack << ScriptData{tr, index};
		}

		reader(operateLine);
		reader(nowName);

		reader(forSaveGraphics); //画像情報をロード
		graphics.clear();
		for (auto itr = forSaveGraphics.begin(); itr != forSaveGraphics.end(); ++itr) {
			graphics << std::make_shared<Graphic>(*itr);
		}

		for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
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
		subProcesses.clear();

		Print << U"Loaded!";

		// 改めて読み込み開始
		readScript();
	}

	/*
	if (KeyE.down()) {
		effectIndex = (effectIndex + 1) % 4;
	}
	*/

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
		Print << U"Variable";
		for (auto [k, v] : vars) {
			Print << k << U" : " << getVariableList(v->value);
		}
		Print << U"";
		Print << U"System Variable";
		for (auto [k, v] : getData().systemVars) {
			Print << k << U" : " << getVariableList(v->value);
		}
		Print << U"-------------";
		return;
	}

	/*
	renderTexture.clear(Palette::Black);
	{
		const ScopedRenderTarget2D target{ renderTexture };
	}

	Graphics2D::Flush();
	renderTexture.resolve();

	Graphics2D::SetPSConstantBuffer(1, cb);

	if (effectIndex == 0) {
		renderTexture.draw();
	}
	else if (effectIndex == 1) {
		const ScopedCustomShader2D shader{ PixelShaderAsset(U"OnlyRed") };
		renderTexture.draw();
	}
	else if (effectIndex == 2) {
		const ScopedCustomShader2D shader{ PixelShaderAsset(U"OnlyGreen") };
		renderTexture.draw();
	}
	else if (effectIndex == 3) {
		const ScopedCustomShader2D shader{ PixelShaderAsset(U"OnlyBlue") };
		renderTexture.draw();
	}
	*/

	for (auto itr = graphics.begin(); itr != graphics.end(); ++itr) {
		(*itr)->draw();
	}

	FontAsset(U"Medium{}"_fmt(nameSize))(nowName).draw(namePos, Palette::White);

	FontAsset(U"Medium{}"_fmt(textSize))(nowWindowText.sentence.substr(0, nowWindowText.index + 1)).draw(textPos, Palette::White);

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