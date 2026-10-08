#include "../src/EngineStartupArguments.h"

#include <cassert>
#include <cstring>
#include <string>

static int comArgc;
static const char** comArgv;
static const char* finalizedArgv[58];
static int inputCount;
static const char** inputArgv;
static int calls;

static void InitArgv(int argc, const char** argv)
{
	++calls;
	assert(inputCount == argc);
	assert(inputArgv == argv);
	assert(0 == strcmp("", argv[0]));
	comArgc = argc;
	for (int i = 0; i < argc; ++i)
		finalizedArgv[i] = argv[i];
	// The engine owns finalization and may append arguments or replace the array.
	finalizedArgv[comArgc++] = "engine-added";
	finalizedArgv[comArgc] = " ";
	comArgv = finalizedArgv;
}

template<class HostParms>
static void CheckLayout()
{
	HostParms host = {};
	char baseDirectory[] = "base";
	int memory = 0;
	host.basedir = baseDirectory;
	host.membase = &memory;
	host.memsize = sizeof(memory);
	EngineStartupArguments arguments;
	static HostParms* currentHost;
	currentHost = &host;
	auto finalize = [](int argc, const char** argv)
	{
		inputCount = currentHost->argc;
		inputArgv = currentHost->argv;
		InitArgv(argc, argv);
	};

	char commandLine[] = "\"C:\\Half Life\\MetaHook.exe\" -game \"czero\" +exec \"my config.cfg\" \"\" pre\"mid dle\"post";
	const int previousCalls = calls;
	arguments.Initialize(commandLine, host, finalize, comArgc, comArgv);
	memset(commandLine, 'x', sizeof(commandLine) - 1);
	assert(previousCalls + 1 == calls);
	assert(9 == host.argc);
	assert(finalizedArgv == host.argv);
	assert(0 == strcmp("C:\\Half Life\\MetaHook.exe", host.argv[1]));
	assert(0 == strcmp("czero", host.argv[3]));
	assert(0 == strcmp("+exec", host.argv[4]));
	assert(0 == strcmp("my config.cfg", host.argv[5]));
	assert(0 == strcmp("", host.argv[6]));
	assert(0 == strcmp("premid dlepost", host.argv[7]));
	assert(0 == strcmp("engine-added", host.argv[8]));
	assert(0 == strcmp(" ", host.argv[9]));
	assert(baseDirectory == host.basedir);
	assert(&memory == host.membase);
	assert(sizeof(memory) == host.memsize);

	std::string manyArguments;
	for (int i = 0; i < 80; ++i)
		manyArguments += " value";
	arguments.Initialize(manyArguments.c_str(), host, finalize, comArgc, comArgv);
	assert(50 == inputCount);
	assert(51 == host.argc);
	assert(0 == strcmp("value", host.argv[49]));
	assert(0 == strcmp("engine-added", host.argv[50]));

	// There is no original tokenizer buffer to overflow; COM_InitArgv retains
	// responsibility for truncating its own reconstructed cmdline buffer.
	const std::string longArgument(4096, 'a');
	arguments.Initialize(longArgument.c_str(), host, finalize, comArgc, comArgv);
	assert(3 == host.argc);
	assert(longArgument == host.argv[1]);
	arguments.Initialize("", host, finalize, comArgc, comArgv);
	assert(1 == inputCount);
	assert(2 == host.argc);
	arguments.Clear();
	arguments.Initialize("-game valve", host, finalize, comArgc, comArgv);
	assert(4 == host.argc);
	assert(0 == strcmp("valve", host.argv[2]));
}

int main()
{
	CheckLayout<quakeparms_t>();
	CheckLayout<quakeparms_svengine_t>();
	return 0;
}
