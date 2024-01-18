#pragma once
#include <Siv3D.hpp>
#include <regex>
#include <any>

class Variable;

enum class GameScene {
	Initialize,
	Engine,
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
		return format_to(ctx.out(), U"empty");
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
	HashTable<String, std::shared_ptr<Variable>> tmp_vars;
	HashTable<String, std::shared_ptr<Variable>> savedata_vars;
	HashTable<String, std::shared_ptr<Variable>> system_vars;

	bool isAuto = false;
	double grobalVolume = 1.0;
	double musicVolume = 1.0; // MixBus0
	double soundVolume = 1.0; // MixBus1
	double voiceVolume = 1.0; // MixBus2
};

using GameManager = SceneManager<GameScene, GameData>;
