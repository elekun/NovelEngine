#include "stdafx.h"
#include "Test.hpp"

Test::Test(const InitData& init) : IScene{ init } {
	effectIndex = 0;
	renderTexture = MSRenderTexture{ Scene::Size() };

	cb->level = 0.1;

	room = Texture{ U"assets/img/room.jpg" };

	reader = TextReader{ U"assets/script/test.txt" };
	if (!reader){
		throw Error{ U"Failed to open `test.txt`" };
	}

	strIndex = 0;
	indexCT = 0.05;
	prevTimeUpdateIndex = 0;

	processEndFlag = false;
	processIndex = 0;

	readScript();
	nowName = U"";
}

void Test::readScript() {
	Array<String> script = reader.readLines();
	size_t i = 0;
	String sentTmp = U"";
	bool readProcessFlag = false;

	while (true) {
		if (i >= script.size()) {
			break;
		}
		String line = script.at(i);


		if (sentTmp != U"" && (line == U"" || readProcessFlag)) {
			sentTmp = sentTmp.substr(0, sentTmp.size() - 1); // 改行削除

			// 文字送りをするための関数
			std::function<void()> f = [&, this, sentence=sentTmp]() {
				nowSentence = sentence;

				// 左クリックで
				if (MouseL.down()) {
					// 最後まで文字が表示されているなら次に行く
					if (strIndex >= nowSentence.size() - 1) {
						strIndex = 0;
						processEndFlag = true;
					}
					// まだ最後まで文字が表示されていないなら最後まで全部出す
					else {
						strIndex = nowSentence.size() - 1;
					}
				}

				// 1文字ずつ順番に表示する
				if (prevTimeUpdateIndex >= indexCT) {
					if (strIndex < nowSentence.size() - 1) {
						strIndex++;
					}
					prevTimeUpdateIndex = 0;
				}
			};

			// 処理リストに追加
			mainProcesses.emplace_back(f);

			sentTmp = U"";
			readProcessFlag = false;
		}

		// "#"から始まるものは本文から弾く（実質コメントアウト）
		const auto exp = RegExp(U"\\s*#(.*)").search(line);
		if (!exp.isEmpty()) {

			Array<String> processName = {U"name", U"image" ,U"music", U"sound", U"move"};
			String proc = U"";
			for (auto [j, p] : Indexed(processName)) {
				proc += p;
				if (j != processName.size() - 1) {
					proc += U"|";
				}
			}

			// 定義済処理
			const auto match = RegExp(U"({})\\[(.*)\\]"_fmt(proc)).search(exp[1].value());
			if (!match.isEmpty()) {
				readProcessFlag = true;

				StringView op = match[1].value(); // 処理のタグ
				const auto tmp = String{ match[2].value() }.split(',');
				Array<String> args = {}; // 引き渡された変数
				for (auto i : tmp) {
					args << String{ RegExp(U"\\s*(.+)\\s*").search(i)[1].value() };
				}

				auto getOption = [](String o) {
					auto option = RegExp(U"(.+)\\s*=\\s*(.+)").search(o);
					String optionName{ option[1].value() };
					String optionArg{ option[2].value() };
					return std::make_pair(optionName, optionArg);
				};

				std::function<void()> f;

				if (op == U"name") {
					// newName
					String name = args.isEmpty() ? U"" : args.at(0);
					f = [&, this, n=name]() {
						nowName = n;
						processEndFlag = true;
					};
				}
				if (op == U"image") {
					// x, y, filepath, ...option
					assert(args.size() >= 2);

					Texture t{ args.at(2) };
					Point p{ Parse<int32>(args.at(0)), Parse<int32>(args.at(1)) };
					int32 l = 0;

					args.pop_front_N(3);
					String n = U""; // タグの名前
					for (auto o : args) {
						auto [option, arg] = getOption(o);

						if (option == U"tag") {
							n = arg;
						}
					}

					ImageInfo i{ t, p, l, n };
					f = [&, this, img=i]() {
						images.emplace_back(img);
						images.stable_sort_by([](const ImageInfo& a, const ImageInfo& b) { return a.layer < b.layer; });
						processEndFlag = true;
					};
				}
				if (op == U"music") {
					//filepath, ...option
					Audio a{ Audio::Stream, args.at(0) };
					f = [&, this, audio=a]() {
						nowMusic = audio;
						nowMusic.play();
						processEndFlag = true;
					};
				}
				if (op == U"sound") {
					//filepath, ...option
					Audio a{ Audio::Stream, args.at(0) };
					f = [&, this, audio = a]() {
						audio.playOneShot();
						processEndFlag = true;
					};
				}
				if (op == U"move") {
					Point dst{ Parse<int32>(args.at(0)), Parse<int32>(args.at(1)) };
					double dur = Parse<double>(args.at(2));
					String n = args.at(3);

					args.pop_front_N(4);
					String name = U""; // タグの名前
					int32 t = 1; // 移動タイプ ( 1(default): 移動先指定, 2: 移動量指定 )
					double del = 0; // 開始時間のディレイ
					for (auto o : args) {
						auto [option, arg] = getOption(o);
						
						if (option == U"type") {
							t = Clamp(Parse<int32>(arg), 1, 2);
						}
						if (option == U"delay") {
							double _ = Parse<double>(arg);
							del = _ > 0 ? _ : 0;
						}
					}

					f = [&, this, distination = dst, duration = dur, tag = name, delay = del]() {
						if (delay > 0) {
							return;
						}
						for (auto img : images) {
							if (img.tag == tag) {

							}
						}
					};
				}
				
				
				if (f != nullptr) {
					mainProcesses.emplace_back(f);
				}
			}
		}
		else {
			if (line != U"") {
				sentTmp += line + U"\n";
			}

			readProcessFlag = false;
		}

		i++;
	}
}

void Test::update() {
	mainProcesses.at(processIndex)();
	if (processEndFlag) {
		processIndex++;
		processEndFlag = false;
	}
	if (mainProcesses.size() == processIndex) {
		nowSentence = U"";
		strIndex = 0;
		changeScene(GameScene::Test);
	}







	if (KeyS.down()) {
		effectIndex = (effectIndex + 1) % 4;
	}

	if (KeyR.down()) {
		changeScene(GameScene::Test);
	}

	prevTimeUpdateIndex += Scene::DeltaTime();
}

void Test::draw() const {

	TextureRegion bg = room.scaled(1.6);
	renderTexture.clear(Palette::Black);
	{
		const ScopedRenderTarget2D target{ renderTexture };
		for (auto img : images) {
			img.texture.draw(img.position);
		}
		//bg.draw(0, 0);
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


	//FontAsset(U"Medium50")(testStrings.at(sentIndex).substr(0, strIndex + 1)).draw(200, 720, Palette::Black);
	FontAsset(U"Medium30")(nowName).draw(200, 650, Palette::White);
	FontAsset(U"Medium40")(nowSentence.substr(0, strIndex + 1)).draw(200, 720, Palette::White);
}
