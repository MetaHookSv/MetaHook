#pragma once

#include "StartupCommandLine.h"

#include <array>
#include <cstddef>

typedef struct quakeparms_s
{
	char* basedir;
	char* cachedir;
	int argc;
	const char** argv;
	void* membase;
	unsigned int memsize;
} quakeparms_t;

// SvEngine removed cachedir from the engine-private startup parameters.
typedef struct quakeparms_svengine_s
{
	char* basedir;
	int argc;
	const char** argv;
	void* membase;
	unsigned int memsize;
} quakeparms_svengine_t;

static_assert(sizeof(void*) == 4, "Engine startup parameters require x86.");
static_assert(offsetof(quakeparms_t, argc) == 8);
static_assert(offsetof(quakeparms_t, argv) == 12);
static_assert(offsetof(quakeparms_svengine_t, argc) == 4);
static_assert(offsetof(quakeparms_svengine_t, argv) == 8);

class EngineStartupArguments
{
public:
	template<class HostParms>
	void Initialize(const char* commandLine, HostParms& hostParms,
		void (*initArgv)(int, const char**), const int& comArgc, const char** const& comArgv)
	{
		// Own the strings independently of the launcher's mutable CommandLine().
		// argv[0] is reserved; the launcher's executable path remains argv[1].
		m_Arguments = ParseStartupCommandLine(commandLine, MAX_NUM_ARGVS - 1);
		m_Argv.fill(nullptr);
		m_Argv[0] = "";
		for (size_t i = 0; i < m_Arguments.size(); ++i)
			m_Argv[i + 1] = m_Arguments[i].c_str();

		hostParms.argc = static_cast<int>(m_Arguments.size()) + 1;
		hostParms.argv = m_Argv.data();
		initArgv(hostParms.argc, hostParms.argv);

		// Read through references after the engine has rebuilt cmdline and applied
		// its own finalization (including -safe on engines that support it).
		hostParms.argc = comArgc;
		hostParms.argv = comArgv;
	}

	void Clear()
	{
		m_Arguments.clear();
		m_Argv.fill(nullptr);
	}

private:
	static constexpr size_t MAX_NUM_ARGVS = 50;
	std::vector<std::string> m_Arguments;
	std::array<const char*, MAX_NUM_ARGVS> m_Argv = {};
};
