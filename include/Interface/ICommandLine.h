#ifndef ICOMMANDLINE_H
#define ICOMMANDLINE_H

#ifdef _WIN32
#    pragma once
#endif

class ICommandLine
{
public:
    virtual void        CreateCmdLine(const char* commandline)                       = 0;
    virtual const char* GetCmdLine(void) const                                       = 0;
    virtual const char* CheckParm(const char* psz, const char** ppszValue = 0) const = 0;
    virtual void        RemoveParm(const char* parm)                                 = 0;
    virtual void        AppendParm(const char* pszParm, const char* pszValues)       = 0;
    virtual void        SetParm(const char* pszParm, const char* pszValues)          = 0;
    virtual void        SetParm(const char* pszParm, int iValue)                     = 0;

    // Append new virtual methods here to preserve existing plugin ABI slots.
    // Quotes are removed; argv[0] is the first command-line token. The existing
    // parser accepts up to 256 arguments; consumers apply their own lower limit.
    // The read-only view (NULL when empty) stays valid until the next mutation
    // through CreateCmdLine, RemoveParm, AppendParm or SetParm.
    virtual int          GetArgc(void) const = 0;
    virtual const char** GetArgv(void) const = 0;
};

ICommandLine* CommandLine(void);

#endif
