#ifndef ICAREERUI_H
#define ICAREERUI_H

#ifdef _WIN32
#    pragma once
#endif

#include <interface.h>
#include <vector>

class ICareerTask;
struct MapInfo;
enum CareerDifficultyType : int;

//Public CareerUI001 interface.
class ICareerUI : public IBaseInterface
{
public:
    virtual bool                       IsPlayingMatch(void)                   = 0;
    virtual std::vector<ICareerTask*>* GetCurrentTaskVec(void)                = 0;
    virtual bool                       PlayAsCT(void)                         = 0;
    virtual int                        GetReputationGained(void)              = 0;
    virtual int                        GetNumMapsUnlocked(void)               = 0;
    virtual bool                       DoesWinUnlockAll(void)                 = 0;
    virtual int                        GetRoundTimeLength(void)               = 0;
    virtual int                        GetWinfastLength(void)                 = 0;
    virtual CareerDifficultyType       GetDifficulty(void) const              = 0;
    virtual int                        GetCurrentMapTriplet(MapInfo* maps)    = 0;
    virtual void                       OnRoundEndMenuOpen(bool didWin)        = 0;
    virtual void                       OnMatchEndMenuOpen(bool didWin)        = 0;
    virtual void                       OnRoundEndMenuClose(bool stillPlaying) = 0;
    virtual void                       OnMatchEndMenuClose(bool stillPlaying) = 0;
};

#define CAREERUI_INTERFACE_VERSION "CareerUI001"

#endif
