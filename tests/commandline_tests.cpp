#include "../src/commandline.cpp"

#include <initializer_list>
#include <string>

static void CheckArguments(const char* commandLine, std::initializer_list<const char*> expected)
{
	g_CmdLine.CreateCmdLine(commandLine);
	if (static_cast<int>(expected.size()) != g_CmdLine.ParmCount())
	{
		fprintf(stderr, "Wrong argument count for: %s\n", commandLine);
		exit(1);
	}

	int index = 0;
	for (const char* argument : expected)
	{
		if (strcmp(argument, g_CmdLine.GetParm(index++)) != 0)
		{
			fprintf(stderr, "Wrong argument %d for: %s\n", index - 1, commandLine);
			exit(1);
		}
	}
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
	g_CmdLine.CreateCmdLine(manyArguments.c_str());
	if (256 != g_CmdLine.ParmCount())
		return 1;

	// Parsing owns its strings, and a subsequent engine session replaces them.
	char temporary[] = "MetaHook.exe -game \"czero\"";
	g_CmdLine.CreateCmdLine(temporary);
	memset(temporary, 'x', sizeof(temporary) - 1);
	const char* value = nullptr;
	if (!g_CmdLine.CheckParm("-game", &value) || strcmp("czero", value) != 0)
		return 1;
	CheckArguments("MetaHook.exe -game valve", { "MetaHook.exe", "-game", "valve" });
	puts("Command-line tests passed.");
	return 0;
}
