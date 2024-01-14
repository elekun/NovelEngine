#pragma once
#include <Siv3D.hpp>

namespace ElephantLib {
	String eraseFrontChar(String s, char32 c);
	String eraseBackChar(String s, char32 c);
	String eraseFrontBackChar(String s, char32 c);

	void makeArchive(FilePath folder, FilePath output);
	void readArchive(FilePath archive);

	FilePath resrc(FilePath f);
}
