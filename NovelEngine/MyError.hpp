#pragma once
#include "GameScene.hpp"

class ArgumentError final : public Error {
public:

	using Error::Error;

	[[nodiscard]]
	StringView type() const noexcept override;
};

class ArgumentParseError final : public Error {
public:

	using Error::Error;

	[[nodiscard]]
	StringView type() const noexcept override;
};

void ThrowArgumentError(const StringView operation, const StringView name, const String type, const v_type value);

void ThrowParseError(const String type, const v_type value);
