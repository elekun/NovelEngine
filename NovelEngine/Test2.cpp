#include "stdafx.h"
#include "Test2.hpp"
#include "ElephantLib.hpp"

Test2::Test2(const InitData& init) : IScene{ init } {
	effectIndex = 0;
	renderTexture = MSRenderTexture{ Scene::Size() };

	cb->level = 0.1;

	TextReader reader{ getData().scriptPath };
	if (!reader) {
		throw Error{ U"Failed to open `{}`"_fmt(getData().scriptPath) };
	}
	scriptLines = reader.readLines();
	scriptIndex = 0;
	forSaveIndex = 0;

	strIndex = 0;
	indexCT = 0.05;

	mainProcess = [&, this]() { return true; };
	fadeProcess = [&, this]() {};

	nowName = U"";
	isShowSelection = false;

	isShowingLog = false;

	savefile = U"./test_save.dat";

	proceedInput = MouseL | KeyEnter | KeySpace;
}

Array<String> Test2::splitArgs(String s) {
	if (s == U"") {
		return {};
	}
	Array<String> args = {};
	Array<char32> bracket = {};
	String tmp = U"";
	for (auto i : step(s.length())) {
		char32 c = s.at(i);
		bool f = true; //変数を構成する文字かどうか
		if (c == U'(' || c == U'{') {
			bracket << c;
		}
		else if (c == U')' || c == U'}') {
			bracket.pop_back();
		}

		if (bracket.isEmpty() && c == U',') {
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

void Test2::readSetting() {
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
		const auto setting = RegExp(U"\\s*({}):\\s*(.+)"_fmt(list)).search(line);
		StringView st = setting[1].value(); // 処理のタグ
		const auto tmp = String{ setting[2].value() }.split(',');
		Array<String> args = {};
		for (auto i : tmp) {
			String t = String{ RegExp(U"\\s*(.+)\\s*").search(i)[1].value() }; //前後のスペースを削除
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

void Test2::readScriptLine(String& s) {
	if (scriptIndex >= scriptLines.size()) {
		return;
	}
	s = scriptLines.at(scriptIndex);
	scriptIndex++;
}

void Test2::readScript() {
	String line;
	bool readStopFlag = false;

	// 引数の処理
	auto getArgs = [&, this](StringView sv) {
		// const auto tmp = String{ sv }.split(',');
		// カンマ区切りのデータ型(Pointなど)が混じってても問題なく分割できるように設計
		const auto tmp = splitArgs(String{ sv });

		Array<String> args = {}; // 引き渡された変数
		for (auto i : tmp) {
			args << ElephantLib::eraseFrontBackChar(i, U' '); //前後のスペースを削除
		}
		return args;
	};

	// オプション引数の処理
	auto getOption = [](String o) {
		auto option = RegExp(U"(.+)\\s*=\\s*(.+)").search(o);
		String optionName{ option[1].value() };
		String optionArg{ option[2].value() };
		return std::make_pair(optionName, optionArg);
	};


	auto setMainProcess = [&, this](std::function<bool()> f) {
		mainProcess = f;
		readStopFlag = true;
	};

	// 文章を読み込む処理
	do {
		// 各種処理
		if (operateLine != U"") {
			Array<String> pn = { U"setting", U"start", U"name", U"setimg", U"chgimg", U"movimg", U"delimg", U"music", U"sound", U"wait", U"goto", U"scenechange", U"call", U"exit"};
			String proc = U"";
			for (auto [j, p] : Indexed(pn)) {
				proc += p;
				if (j != pn.size() - 1) { proc += U"|"; }
			}

			const auto operate = RegExp(U"({})\\[(.*)\\]"_fmt(proc)).search(operateLine);
			if (!operate.isEmpty()) {

				StringView op = operate[1].value(); // 処理のタグ
				Array<String> args = getArgs(operate[2].value());
				if (op == U"setting") {
					FilePath fp = args.at(0); //設定ファイルのパス
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

					for (auto op : args) {
						auto [option, arg] = getOption(op);
						if (option == U"duration") {
							d = Parse<double>(arg);
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
					String name = args.isEmpty() ? U"" : args.at(0);
					std::function<bool()> f = [&, this, n = name]() {
						nowName = n;
						return true;
					};
					setMainProcess(f);
				}

				if (op == U"setimg") {
					// tag, pos, filepath, ...option
					assert(args.size() >= 2);

					String n = args.at(0); // タグの名前
					FilePath fp;
					Vec2 p;
					int32 l = 0;
					int32 w = 0, h = 0;
					args.pop_front_N(1);

					double fa = 0; // フェードインにかかる時間
					double s = 1.0;
					for (auto op : args) {
						auto [option, arg] = getOption(op);
						if (option == U"pos") {
							p = Parse<Vec2>(arg);
						}
						if (option == U"path") {
							fp = arg;
						}
						if (option == U"fade") {
							fa = Max(Parse<double>(arg), 0.0);
						}
						if (option == U"width") {
							w = Max(Parse<int32>(arg), 1);
						}
						if (option == U"height") {
							h = Max(Parse<int32>(arg), 1);
						}
						if (option == U"scale") {
							s = Max(Parse<double>(arg), 0.0);
						}
						if (option == U"layer") {
							l = Parse<int32>(arg);
						}
					}

					Image img{ fp }; //画像サイズの計算のために一時的にImageを作成
					w = w > 0 ? w : img.width();
					h = h > 0 ? h : img.height();

					ImageInfo i{ fp, p, Size{ w, h }, s, 0.0, l, n };

					std::function<std::function<bool()>()> f = [&, this, img = i, _n = n, _fa = fa]() {
						double time = 0.0;
						images.emplace_back(img);
						images.stable_sort_by([](const ImageInfo& a, const ImageInfo& b) { return a.layer < b.layer; });
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = images.begin(); itr != images.end(); ++itr) {
								if (itr->tag == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									itr->opacity = t;
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

				if (op == U"chgimg") {
					// tag, filepath, ...option
					assert(args.size() >= 2);

					String n = args.at(0); // タグの名前
					FilePath fp;
					double fa = 0.0;
					args.pop_front_N(1);

					for (auto op : args) {
						auto [option, arg] = getOption(op);
						if (option == U"path") {
							fp = arg;
						}
						if (option == U"fade") {
							fa = Max(Parse<double>(arg), 0.0);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _t = n, _fp = fp, _fa = fa]() {
						double time = 0.0;
						for (auto itr = images.begin(); itr != images.end(); ++itr) {
							if (itr->tag == _t) {
								itr->nextPath = _fp;
								itr->setNextTexture();
							}
						}
						return [&, this, time, tag = _t, filepath = _fp, fade = _fa]() mutable {
							for (auto itr = images.begin(); itr != images.end(); ++itr) {
								if (itr->tag == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									itr->opacity = (1.0 - t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								for (auto itr = images.begin(); itr != images.end(); ++itr) {
									if (itr->tag == tag) {
										itr->changeTexture();
										itr->opacity = 1.0;
									}
								}
								return true;
							}

							return false;
						};
					};

					subProcesses.emplace_back(f());
				}

				if (op == U"movimg") {
					// x, y, duration, tag, ...option
					Point v{ Parse<int32>(args.at(0)), Parse<int32>(args.at(1)) };
					double dur = Parse<double>(args.at(2));
					String ta = args.at(3);

					args.pop_front_N(4);
					int32 ty = 1; // 移動タイプ ( 1(default): 移動先指定, 2: 移動量指定 )
					double del = 0; // 開始時間のディレイ
					for (auto o : args) {
						auto [option, arg] = getOption(o);

						if (option == U"type") {
							ty = Max(Parse<int32>(arg), 1);
						}
						if (option == U"delay") {
							double _ = Parse<double>(arg);
							del = _ > 0 ? _ : 0;
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _v = v, _dur = dur, _ta = ta, _ty = ty, _del = del]() {
						double time = 0.0;
						Vec2 src, dst;

						for (auto img : images) {
							if (img.tag == _ta) {
								src = img.position;
								if (_ty == 1) {
									dst = _v;
								}
								if (_ty == 2) {
									dst = src + _v;
								}
							}
						}

						return [&, this, time, src, dst, duration = _dur, tag = _ta, type = _ty, delay = _del]() mutable {
							for (auto itr = images.begin(); itr != images.end(); ++itr) {
								if (itr->tag == tag) {
									double t = duration > 0 ? Clamp((time - delay) / duration, 0.0, 1.0) : 1.0;
									itr->position = src.lerp(dst, t);

									if (proceedInput.down()) {
										itr->position = dst;
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

				if (op == U"delimg") {
					// tag, ...option
					String n = args.at(0); // タグの名前
					args.pop_front_N(1);

					double fa = 0; // フェードアウトにかかる時間
					for (auto op : args) {
						auto [option, arg] = getOption(op);
						if (option == U"fade") {
							fa = Max(Parse<double>(arg), 0.0);
						}
					}

					std::function<std::function<bool()>()> f = [&, this, _n = n, _fa = fa]() {
						double time = 0.0;
						return [&, this, time, tag = _n, fade = _fa]() mutable {
							for (auto itr = images.begin(); itr != images.end(); ++itr) {
								if (itr->tag == tag) {
									double t = fade > 0 ? Clamp(time / fade, 0.0, 1.0) : 1.0;
									itr->opacity = (1.0 - t);
								}
							}

							time += Scene::DeltaTime();

							if (time >= fade) {
								for (auto itr = images.begin(); itr != images.end(); ) {
									if (itr->tag == tag) {
										itr = images.erase(itr);
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
					Audio a{ Audio::Stream, args.at(0) };
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
					Audio a{ Audio::Stream, args.at(0) };
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
					double d = Parse<double>(args.at(0));
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
					String d = args.at(0);
					std::function<bool()> f = [&, this, dst = d]() {
						for (auto [i, l] : Indexed(scriptLines)) {
							const auto skip = RegExp(U"@(.+)").search(l);
							if (!skip.isEmpty()) {
								if (skip[1].value() == dst) {
									scriptIndex = i + 1;

								}
							}
						}
						return true;
					};


					setMainProcess(f);
				}

				if (op == U"scenechange") {
					// filename
					String fi = args.at(0);
					args.pop_front_N(1);

					double d = 1.0; // シーン切り替えににかかる時間
					for (auto op : args) {
						auto [option, arg] = getOption(op);
						if (option == U"duration") {
							d = Max(Parse<double>(arg), 0.0);
						}
					}

					std::function<bool()> f = [&, this, file = fi, dur = d]() {
						getData().scriptPath = U"assets/script/{}.es"_fmt(file);
						changeScene(GameScene::Test2, dur * 1.0s);
						return true;
					};

					setMainProcess(f);
				}

				/* マクロ挿入用、readScript()の処理を変える必要があるので後回し
				* 1. struct{scriptIndex, scriptLines} の配列を作成
				* 2. スタック（First in Last out）でscriptLinesにアクセス、処理を実行
				* 3. 最後まで行ったらスタックから出す
				*/
				if (op == U"call") {
					// filename
					String fi = args.at(0);
					args.pop_front_N(1);

					for (auto op : args) {
						auto [option, arg] = getOption(op);
					}

					std::function<bool()> f = [&, this, file = fi]() {
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
		if (RegExp(U"\\s*\\$\\$.*").fullMatch(line)) {
			String code = String{ RegExp(U"\\s*\\$\\$(.*)").search(line)[1].value() };
			bool isCodeLine = true;
			while (isCodeLine) {
				readScriptLine(line);
				if (RegExp(U".*\\$\\$\\s*").fullMatch(line)) {
					code += String{ RegExp(U"(.*)\\$\\$\\s*").search(line)[1].value() };
					isCodeLine = false;
				}
				else {
					code += line;
				}
			}
			analyzeCode(code);
			// Print << code;
			line = U"";
		}

		// # 始まりと @ 始まりの行は特殊文として処理
		auto tmp = RegExp(U"\\s*[#@](.*)").search(line);

		// 処理が挟まるか空行ならひとかたまりの文章として処理
		if (!tmp.isEmpty() || line == U"") {
			if (!tmp.isEmpty()) {
				operateLine = tmp[1].value();
			}

			if (nowSentence != U"") {
				if (nowSentence.back() == '\n') {
					nowSentence = nowSentence.substr(0, nowSentence.size() - 1); // 改行削除
				}

				std::function<std::function<bool()>()> f = [&, this]() {
					double time = 0.0;
					return [&, this, time]() mutable {

						time += Scene::DeltaTime();

						// 左クリックorスペースで
						if (proceedInput.down()) {
							// 最後まで文字が表示されているなら次に行く
							if (strIndex >= nowSentence.size() - 1) {
								resetTextWindow({ nowSentence });
								setSaveVariable();
								return true;
							}
							// まだ最後まで文字が表示されていないなら最後まで全部出す
							else {
								strIndex = nowSentence.size() - 1;
							}
						}

						// 選択肢が用意されているときの処理
						// 文字が最後まで表示されたら
						if (!selections.isEmpty() && strIndex >= nowSentence.size() - 1) {
							// 選択肢を表示しておく
							isShowSelection = true;
							return true;
						}

						// 1文字ずつ順番に表示する
						if (time >= indexCT) {
							if (strIndex < nowSentence.size() - 1) {
								strIndex++;
							}
							time = 0;
						}

						return false;
					};
				};
				setMainProcess(f());
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
			const auto select = RegExp(U"\\s*\\*(.*)\\[(.*)\\]").search(line);
			if (!select.isEmpty()) {
				// 選択肢を読み込んだら選択肢リストに追加
				String text{ select[1].value() };
				Array<String> args = getArgs(select[2].value());
				String dst{ args.at(0) };
				String type{ args.at(1) };
				Point pos = Point{ 0, 0 };
				Point size = Point{ 0, 0 };
				String normal = U"", hover = U"", click = U"";
				args.pop_front_N(2);

				// 引数を処理
				for (auto op : args) {
					auto [option, arg] = getOption(op);
					if (type == U"image") {
						if (option == U"pos") {
							pos = Parse<Point>(arg);
						}
						if (option == U"size") {
							size = Parse<Point>(arg);
						}
						if (option == U"normal") {
							normal = arg;
						}
						if (option == U"hover") {
							hover = arg;
						}
						if (option == U"click") {
							click = arg;
						}
					}
				}

				selections.emplace_back(Choice{ text, dst, type, pos, size, normal, hover, click });

				isShowSelection = false;
			}
			else {
				// 選択肢でないならウィンドウに表示する（つまり普通の）文字列として処理
				nowSentence += line + U"\n";
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
						resetTextWindow({ nowSentence, U"choiced : " + itr->text });
						setSaveVariable();
						isShowSelection = false;

						// 指定箇所までスキップ
						for (auto [i, l] : Indexed(scriptLines)) {
							const auto skip = RegExp(U"@(.+)").search(l);
							if (!skip.isEmpty()) {
								if (skip[1].value() == itr->dst) {
									scriptIndex = i + 1;
								}
							}
						}

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

void Test2::analyzeCode(String code){
	Array<String> exps = code.split(';');
	for (auto i: step(exps.size())) {
		String exp = exps.at(i);
		if (RegExp(U"(int|double|string)\\s(.+)").fullMatch(exp)) {
			String type = String{ RegExp(U"(int|double|string)\\s(.+)=(.+)").search(exp)[1].value()};
			String var = String{ RegExp(U"(int|double|string)\\s(.+)=(.+)").search(exp)[2].value() };
			String value = String{ RegExp(U"(int|double|string)\\s(.+)=(.+)").search(exp)[3].value() };

			// ElephantLib::eraseFrontBackChar(s, U' ');
		}
	}
}

void Test2::resetTextWindow(Array<String> strs) {
	for (auto itr = strs.begin(); itr != strs.end(); ++itr) {
		scenarioLog << *itr;
	}
	nowSentence = U"";
	strIndex = 0;
}

void Test2::setSaveVariable() {
	forSaveIndex = scriptIndex;
	forSaveImages = images;
}

void Test2::update() {

	isShowingLog = KeyF5.down() ? !isShowingLog : isShowingLog;

	// ログ見せの場合は何も動かさない
	if (isShowingLog) {
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

		Print << U"Saved!";
		writer(forSaveIndex);
		writer(operateLine);
		writer(nowName);

		writer(forSaveImages); //画像をシリアライズして保存はできないので、画像そのものではなく画像情報を保存
	}

	// クイックロード
	if (KeyL.down()) {
		Deserializer<BinaryReader> reader{ savefile };
		if (!reader) throw Error{ U"Failed to open SaveFile" };

		Print << U"Loaded!";
		reader(scriptIndex);
		reader(operateLine);
		reader(nowName);

		reader(images); //画像情報をロード
		for (auto itr = images.begin(); itr != images.end(); ++itr) {
			itr->setTexture(); //Textureを設定
		}

		// 実行中の各種処理を初期化
		resetTextWindow({});
		subProcesses.clear();

		// 改めて読み込み開始
		readScript();
	}

	/*
	if (KeyE.down()) {
		effectIndex = (effectIndex + 1) % 4;
	}
	*/

	if (KeyR.down()) {
		changeScene(GameScene::Test2);
	}
}

void Test2::draw() const {
	if (isShowingLog) {
		ClearPrint();
		for (auto itr = scenarioLog.begin(); itr != scenarioLog.end(); ++itr) {
			Print << *itr;
		}
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


	for (auto img : images) {
		img.draw();
	}

	FontAsset(U"Medium{}"_fmt(nameSize))(nowName).draw(namePos, Palette::White);

	FontAsset(U"Medium{}"_fmt(textSize))(nowSentence.substr(0, strIndex + 1)).draw(textPos, Palette::White);

	if (isShowSelection) {
		for (auto itr = selections.begin(); itr != selections.end(); ++itr) {
			itr->draw();
		}
	}

	// フェードインアウトの描画
	fadeProcess();
}
