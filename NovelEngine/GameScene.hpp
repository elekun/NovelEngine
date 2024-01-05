#pragma once
#include <Siv3D.hpp>
#include <regex>

enum class GameScene {
	Initialize,
	Test,
	Test2,
};

struct VariableKey {
	String name;
	int32 index;

	VariableKey() : name(U""), index(0) {};
	VariableKey(String n, int32 i) : name(n), index(i){};
	template <class Archive>
	void SIV3D_SERIALIZE(Archive& archive) { archive(name, index); };

	friend bool operator==(const VariableKey& a, const VariableKey& b) noexcept{
		return (a.name == b.name && a.index == b.index);
	}
};
template<>
struct std::hash<VariableKey> {
	size_t operator()(const VariableKey& value) const noexcept {
		size_t h1 = std::hash<String>()(value.name);
		size_t h2 = std::hash<int32>()(value.index);
		return h1 ^ h2;
	}
};

struct mono {
	friend void Formatter(FormatData& formatData, const mono& m){
		formatData.string += U"empty";
	}
};

template<>
struct SIV3D_HIDDEN fmt::formatter<mono, s3d::char32> {
	std::u32string tag;
	auto parse(basic_format_parse_context<s3d::char32>& ctx){
		return s3d::detail::GetFormatTag(tag, ctx);
	}
	template <class FormatContext>
	auto format(const mono& value, FormatContext& ctx) {
		return format_to(ctx.out(), U"monostate");
	}
};

using v_type = std::variant<mono, int32, double, String, bool, Point>;
using TYPE = std::variant<mono, int32, String, double>;

struct GameData {
	FilePath scriptPath;
	HashTable<VariableKey, v_type> grobalVariable;
	HashTable<VariableKey, v_type> variable;
};

using GameManager = SceneManager<GameScene, GameData>;

#include "Test.hpp"
#include "Test2.hpp"
#include "Initialize.hpp"
#include "MyError.hpp"
