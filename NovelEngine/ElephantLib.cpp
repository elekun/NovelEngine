#include "stdafx.h"
#include "ElephantLib.hpp"

String ElephantLib::eraseFrontChar(String s, char32 c) {
	String out = U"";
	bool flag = false;
	for (auto itr = s.begin(); itr != s.end(); ++itr) {
		if (*itr == c && !flag) {
			continue;
		}
		else {
			flag = true;
			out += *itr;
		}
	}
	return out;
}

String ElephantLib::eraseBackChar(String s, char32 c) {
	String out = U"";
	bool flag = false;
	for (auto itr = s.rbegin(); itr != s.rend(); ++itr) {
		if (*itr == c && !flag) {
			continue;
		}
		else {
			flag = true;
			out += *itr;
		}
	}
	return out.reverse();
}

String ElephantLib::eraseFrontBackChar(String s, char32 c) {
	return eraseFrontChar(eraseBackChar(s, c), c);
}

void ElephantLib::makeArchive(FilePath folder, FilePath output) {
	//アーカイブファイル作成テスト
	BinaryWriter out{ output };
	Array<FilePath> filePathList;

	//ファイル数チェック
	int32 numberOfFile = 0;
	for (const auto& a_path : FileSystem::DirectoryContents(folder, Recursive::Yes)) {
		BinaryReader in{ a_path };
		if (in.size() > 0) {
			filePathList << FileSystem::RelativePath(a_path, U"./");
			numberOfFile++;
		}
	}
	out.write(numberOfFile); //ファイル数（32bit整数）

	//各ファイルのサイズチェック
	for (const auto& path : filePathList) {
		BinaryReader in{ path };
		out.write(in.size()); //ファイルサイズ（64bit整数）
		out.write(path.length()); //パスの文字列のサイズ(64bit整数)
		out.write(path.data(), (sizeof(char32) * path.length())); //パス(8byte文字列)
	}

	//ファイル内容書き込み
	for (const auto& path : filePathList) {
		BinaryReader in{ path };
		char oneByte;
		while (in.read(oneByte)) {
			out.write(oneByte);
		}
	}
}

void ElephantLib::readArchive(FilePath archive) {
	//アーカイブファイル読み込みテスト
	BinaryReader dataReader{ archive };
	TextWriter writer{ U"data/archive_test.txt" };
	int64 headerSize = 0;

	// データの総数のチェック
	int32 numberOfFile;
	dataReader.read(numberOfFile);
	headerSize += sizeof(numberOfFile);

	// Key: FilePath, Value: pair<ReadStartPoint, FileSize>
	HashTable<FilePath, std::pair<int64, int64>> dataTable;

	int64 fileSize;
	int64 pathLength;
	FilePath filePath;
	int64 beforeAllFileSize = 0;
	for (auto i : step(numberOfFile)) {
		dataReader.read(fileSize);
		dataReader.read(pathLength);
		filePath.resize(pathLength);
		dataReader.read(filePath.data(), (sizeof(char32) * pathLength));

		headerSize += sizeof(fileSize) + sizeof(pathLength) + sizeof(filePath);
		dataTable.emplace(filePath, std::make_pair(beforeAllFileSize, fileSize));

		beforeAllFileSize += fileSize;
	}

	// いちいち計算しなくて済むように読み取り開始部分を計算
	for (auto itr = dataTable.begin(); itr != dataTable.end(); ++itr) {
		itr->second.first += headerSize;
	}

	dataReader.close();


	//テスト読み込み
	FilePath testPath = U"assets/img/room.jpg";

	//まず所定の部分にアクセスして一時ファイルを生成
	if (dataTable.contains(testPath)) {
		BinaryReader archiveReader{ testPath };
		BinaryWriter testWriter{ U"temp/img" };
		auto [pos, size] = dataTable[testPath];
		archiveReader.setPos(pos);
		char oneByte;
		for (auto i : step(size)) {
			archiveReader.read(oneByte);
			testWriter.write(oneByte);
		}
	}

	TextureAsset::Register(U"test", U"temp/img");
	TextureAsset::Load(U"test");
	FileSystem::Remove(U"temp/img", AllowUndo::No);

	// 一回消したら一時ファイルを再生成しないとダメ（当たり前）
	// TextureAsset::Release(U"test");
	// TextureAsset::Load(U"test");
}

FilePath ElephantLib::resrc(FilePath f) {
#ifdef _DEBUG
	return f;
#else
	return Resource(f);
#endif
}

HashTable<String, Input> ElephantLib::stringToInput = {
	{U"KeyCancel", KeyCancel},
	{U"KeyBackspace", KeyBackspace},
	{U"KeyTab", KeyTab},
	{U"KeyClear", KeyClear},
	{U"KeyEnter", KeyEnter},
	{U"KeyShift", KeyShift},
	{U"KeyControl", KeyControl},
	{U"KeyAlt", KeyAlt},
	{U"KeyPause", KeyPause},
	{U"KeyEscape", KeyEscape},
	{U"KeySpace", KeySpace},
	{U"KeyPageUp", KeyPageUp},
	{U"KeyPageDown", KeyPageDown},
	{U"KeyEnd", KeyEnd},
	{U"KeyHome", KeyHome},
	{U"KeyLeft", KeyLeft},
	{U"KeyUp", KeyUp},
	{U"KeyRight", KeyRight},
	{U"KeyDown", KeyDown},
	{U"KeyPrintScreen", KeyPrintScreen},
	{U"KeyInsert", KeyInsert},
	{U"KeyDelete", KeyDelete},

	{U"Key0", Key0},
	{U"Key1", Key1},
	{U"Key2", Key2},
	{U"Key3", Key3},
	{U"Key4", Key4},
	{U"Key5", Key5},
	{U"Key6", Key6},
	{U"Key7", Key7},
	{U"Key8", Key8},
	{U"Key9", Key9},

	{U"KeyA", KeyA},
	{U"KeyB", KeyB},
	{U"KeyC", KeyC},
	{U"KeyD", KeyD},
	{U"KeyE", KeyE},
	{U"KeyF", KeyF},
	{U"KeyG", KeyG},
	{U"KeyH", KeyH},
	{U"KeyI", KeyI},
	{U"KeyJ", KeyJ},
	{U"KeyK", KeyK},
	{U"KeyL", KeyL},
	{U"KeyM", KeyM},
	{U"KeyN", KeyN},
	{U"KeyO", KeyO},
	{U"KeyP", KeyP},
	{U"KeyQ", KeyQ},
	{U"KeyR", KeyR},
	{U"KeyS", KeyS},
	{U"KeyT", KeyT},
	{U"KeyU", KeyU},
	{U"KeyV", KeyV},
	{U"KeyW", KeyW},
	{U"KeyX", KeyX},
	{U"KeyY", KeyY},
	{U"KeyZ", KeyZ},

	{U"KeyNum0", KeyNum0},
	{U"KeyNum1", KeyNum1},
	{U"KeyNum2", KeyNum2},
	{U"KeyNum3", KeyNum3},
	{U"KeyNum4", KeyNum4},
	{U"KeyNum5", KeyNum5},
	{U"KeyNum6", KeyNum6},
	{U"KeyNum7", KeyNum7},
	{U"KeyNum8", KeyNum8},
	{U"KeyNum9", KeyNum9},
	{U"KeyNumMultiply", KeyNumMultiply},
	{U"KeyNumAdd", KeyNumAdd},
	{U"KeyNumEnter", KeyNumEnter},
	{U"KeyNumSubtract", KeyNumSubtract},
	{U"KeyNumDecimal", KeyNumDecimal},
	{U"KeyNumDivide", KeyNumDivide},

	{U"KeyF1", KeyF1},
	{U"KeyF2", KeyF2},
	{U"KeyF3", KeyF3},
	{U"KeyF4", KeyF4},
	{U"KeyF5", KeyF5},
	{U"KeyF6", KeyF6},
	{U"KeyF7", KeyF7},
	{U"KeyF8", KeyF8},
	{U"KeyF9", KeyF9},
	{U"KeyF10", KeyF10},
	{U"KeyF11", KeyF11},
	{U"KeyF12", KeyF12},
	{U"KeyF13", KeyF13},
	{U"KeyF14", KeyF14},
	{U"KeyF15", KeyF15},
	{U"KeyF16", KeyF16},
	{U"KeyF17", KeyF17},
	{U"KeyF18", KeyF18},
	{U"KeyF19", KeyF19},
	{U"KeyF20", KeyF20},
	{U"KeyF21", KeyF21},
	{U"KeyF22", KeyF22},
	{U"KeyF23", KeyF23},
	{U"KeyF24", KeyF24},

	{U"KeyNumLock", KeyNumLock},
	{U"KeyLShift", KeyLShift },
	{ U"KeyRShift", KeyRShift },
	{ U"KeyLControl", KeyLControl },
	{ U"KeyRControl", KeyRControl },
	{ U"KeyLAlt", KeyLAlt },
	{ U"KeyRAlt", KeyRAlt },
	{ U"KeyNextTrack", KeyNextTrack },
	{ U"KeyPreviousTrack", KeyPreviousTrack },
	{ U"KeyStopMedia", KeyStopMedia },
	{ U"KeyPlayPauseMedia", KeyPlayPauseMedia },
	{ U"KeyColon_JIS", KeyColon_JIS },
	{ U"KeySemicolon_US", KeySemicolon_US },
	{ U"KeySemicolon_JIS", KeySemicolon_JIS },
	{ U"KeyEqual_US", KeyEqual_US },
	{ U"KeyComma", KeyComma },
	{ U"KeyMinus", KeyMinus },
	{ U"KeyPeriod", KeyPeriod },
	{ U"KeySlash", KeySlash },
	{ U"KeyGraveAccent", KeyGraveAccent },
	{ U"KeyCommand", KeyCommand },
	{ U"KeyLeftCommand", KeyLeftCommand },
	{ U"KeyRightCommand", KeyRightCommand },
	{ U"KeyLBracket", KeyLBracket },
	{ U"KeyYen_JIS", KeyYen_JIS },
	{ U"KeyBackslash_US", KeyBackslash_US },
	{ U"KeyRBracket", KeyRBracket },
	{ U"KeyCaret_JIS", KeyCaret_JIS },
	{ U"KeyApostrophe_US", KeyApostrophe_US },
	{ U"KeyUnderscore_JIS", KeyUnderscore_JIS },

	{ U"MouseL", MouseL },
	{ U"MouseR", MouseR },
	{ U"MouseM", MouseM },
};
