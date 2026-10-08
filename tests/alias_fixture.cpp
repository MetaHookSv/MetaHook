#ifndef FIXTURE_VALUE
#    define FIXTURE_VALUE 1
#endif
extern "C" __declspec(dllexport) int AliasFixture()
{
    return FIXTURE_VALUE;
}
