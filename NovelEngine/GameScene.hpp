#pragma once
#include <Siv3D.hpp>
#include <regex>
#include <any>

class Variable;

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


void ThrowArgumentError(const StringView operation, const StringView name, const String type, const v_type value);

void ThrowParseError(const String type, const v_type value);


class Variable {
public:
	String name;
	std::any value;
	Variable() {};
	Variable(String n, std::any v) : name(n), value(v) {};
};

struct GameData {
	FilePath scriptPath;
	HashTable<VariableKey, v_type> grobalVariable;
	HashTable<VariableKey, v_type> variable;
	HashTable<String, std::shared_ptr<Variable>> vars;
};

using GameManager = SceneManager<GameScene, GameData>;
