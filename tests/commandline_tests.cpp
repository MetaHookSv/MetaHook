#include "ICommandLine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>

extern bool CheckLegacyCommandLineABI(void* commandLine);

static void CheckCurrentArguments(std::initializer_list<const char*> expected)
{
	const ICommandLine* commandLine = CommandLine();
	if (static_cast<int>(expected.size()) != commandLine->GetArgc())
	{
		fprintf(stderr, "Wrong argument count for: %s\n", commandLine->GetCmdLine());
		exit(1);
	}

	int index = 0;
	for (const char* argument : expected)
	{
		if (strcmp(argument, commandLine->GetArgv()[index++]) != 0)
		{
			fprintf(stderr, "Wrong argument %d for: %s\n", index - 1, commandLine->GetCmdLine());
			exit(1);
		}
	}
}

static void CheckArguments(const char* commandLine, std::initializer_list<const char*> expected)
{
	CommandLine()->CreateCmdLine(commandLine);
	CheckCurrentArguments(expected);
}

int main()
{
	CheckArguments("MetaHook.exe -insecure -game czero +exec autoexec.cfg",
		{ "MetaHook.exe", "-insecure", "-game", "czero", "+exec", "autoexec.cfg" });
	CheckArguments("\"C:\\Half Life\\MetaHook.exe\" -game \"czero\"",
		{ "C:\\Half Life\\MetaHook.exe", "-game", "czero" });
	CheckArguments("MetaHook.exe -value \"\" next", { "MetaHook.exe", "-value", "", "next" });
	CheckArguments("MetaHook.exe pre\"middle space\"post \"a\"\"b\"",
		{ "MetaHook.exe", "premiddle spacepost", "ab" });
	CheckArguments("MetaHook.exe -basedir \"C:\\Half Life\\\"",
		{ "MetaHook.exe", "-basedir", "C:\\Half Life\\" });
	CheckArguments("MetaHook.exe \"unterminated value", { "MetaHook.exe", "unterminated value" });
	CheckArguments(" \t\r\nMetaHook.exe\t-game\rczero\n", { "MetaHook.exe", "-game", "czero" });
	CheckArguments("", {});

	std::string manyArguments = "MetaHook.exe";
	for (int i = 0; i < 300; ++i)
		manyArguments += " value";
	CommandLine()->CreateCmdLine(manyArguments.c_str());
	if (256 != CommandLine()->GetArgc())
		return 1;

	// Parsing owns its strings, and a subsequent engine session replaces them.
	char temporary[] = "MetaHook.exe -game \"czero\"";
	CommandLine()->CreateCmdLine(temporary);
	memset(temporary, 'x', sizeof(temporary) - 1);
	const char* value = nullptr;
	if (!CommandLine()->CheckParm("-game", &value) || strcmp("czero", value) != 0)
		return 1;
	const char** arguments = CommandLine()->GetArgv();
	const char* game = arguments[2];
	CommandLine()->GetArgc();
	CommandLine()->GetCmdLine();
	CommandLine()->CheckParm("-game");
	if (arguments != CommandLine()->GetArgv() || game != arguments[2] || strcmp("czero", game) != 0)
		return 1;

	// Sys_InitArgv may receive the singleton's own raw command-line buffer.
	CommandLine()->CreateCmdLine(CommandLine()->GetCmdLine());
	CheckCurrentArguments({ "MetaHook.exe", "-game", "czero" });
	CommandLine()->AppendParm("-windowed", nullptr);
	CheckCurrentArguments({ "MetaHook.exe", "-game", "czero", "-windowed" });
	CommandLine()->RemoveParm("-windowed");
	CheckCurrentArguments({ "MetaHook.exe", "-game", "czero" });
	CommandLine()->SetParm("-game", "valve");
	CheckCurrentArguments({ "MetaHook.exe", "-game", "valve" });
	CommandLine()->SetParm("-width", 800);
	CheckCurrentArguments({ "MetaHook.exe", "-game", "valve", "-width", "800" });
	CommandLine()->CreateCmdLine(CommandLine()->GetArgv()[2]);
	CheckCurrentArguments({ "valve" });
	if (!CheckLegacyCommandLineABI(CommandLine()))
		return 1;
	CheckArguments("MetaHook.exe -game valve", { "MetaHook.exe", "-game", "valve" });
	puts("Command-line tests passed.");
	return 0;
}
