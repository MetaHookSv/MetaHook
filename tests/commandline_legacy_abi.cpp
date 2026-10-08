#include <cstring>

// Compile the pre-extension plugin contract in its own translation unit.
// This must not include the current ICommandLine declaration: these virtual
// calls exercise the seven original slots as an already-built plugin would.
class LegacyCommandLine
{
public:
	virtual void CreateCmdLine(const char*) = 0;
	virtual const char* GetCmdLine() const = 0;
	virtual const char* CheckParm(const char*, const char** = nullptr) const = 0;
	virtual void RemoveParm(const char*) = 0;
	virtual void AppendParm(const char*, const char*) = 0;
	virtual void SetParm(const char*, const char*) = 0;
	virtual void SetParm(const char*, int) = 0;
};

bool CheckLegacyCommandLineABI(void* instance)
{
	auto* commandLine = static_cast<LegacyCommandLine*>(instance);
	commandLine->CreateCmdLine("MetaHook.exe -game czero");
	if (strcmp("MetaHook.exe -game czero", commandLine->GetCmdLine()) != 0)
		return false;
	const char* value = nullptr;
	if (!commandLine->CheckParm("-game", &value) || strcmp("czero", value) != 0)
		return false;
	commandLine->AppendParm("-width", "640");
	if (!commandLine->CheckParm("-width", &value) || strcmp("640", value) != 0)
		return false;
	commandLine->SetParm("-width", 800);
	if (!commandLine->CheckParm("-width", &value) || strcmp("800", value) != 0)
		return false;
	commandLine->SetParm("-game", "valve");
	if (!commandLine->CheckParm("-game", &value) || strcmp("valve", value) != 0)
		return false;
	commandLine->RemoveParm("-width");
	return commandLine->CheckParm("-width") == nullptr;
}
