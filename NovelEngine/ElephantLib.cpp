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
	for (const auto& a_path : FileSystem::DirectoryContents(folder)) {
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
