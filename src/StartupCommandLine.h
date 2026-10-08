#pragma once

#include <string>
#include <vector>

// Startup arguments use literal backslashes, not Windows CRT escape rules.
// Keep this parser shared by the launcher and the engine's Sys_InitArgv hook.
inline std::vector<std::string> ParseStartupCommandLine(const char* commandLine, size_t maxArguments)
{
	std::vector<std::string> arguments;
	if (!commandLine)
		return arguments;

	const auto isSeparator = [](unsigned char c) { return c <= ' ' || c == 127; };
	const char* cursor = commandLine;
	while (*cursor && arguments.size() < maxArguments)
	{
		while (*cursor && isSeparator(static_cast<unsigned char>(*cursor)))
			++cursor;
		if (!*cursor)
			break;

		std::string argument;
		bool inQuotes = false;
		while (*cursor)
		{
			if (*cursor == '"')
				inQuotes = !inQuotes;
			else if (!inQuotes && isSeparator(static_cast<unsigned char>(*cursor)))
				break;
			else
				argument += *cursor;
			++cursor;
		}
		arguments.push_back(argument);
	}
	return arguments;
}
