#pragma once

#include <mathlib.h>
#include <tier1/utllinkedlist.h>
#include <tier1/utlmap.h>
#include <icvar.h>

struct LayerRecord
{
    int m_sequence;
    float m_cycle;
    float m_weight;
    int m_order;
};

struct LagRecord
{
    // Did player die this frame
    int            m_fFlags;

    // Player position, orientation and bbox
    Vector          m_vecOrigin;
    QAngle          m_vecAngles;
    Vector          m_vecMins;
    Vector          m_vecMaxs;

    float          m_flSimulationTime;

    // Player animation details, so we can get the legs in the right spot.
    LayerRecord        m_layerRecords[15];
    int            m_masterSequence;
    float          m_masterCycle;

    float          m_flPoseParameters[24];
};

typedef CUtlFixedLinkedList< LagRecord > LagRecordList;

class CBaseEntity;
typedef CBaseHandle EHANDLE;

struct EntityLagData
{
    // True if lag compensation altered entity data
    bool      m_bRestoreEntity;
    // keep a list of lag records for each player
    LagRecordList  m_LagRecords;

    // Entity data before we moved him back
    LagRecord    m_RestoreData;
    // Entity data where we moved him back
    LagRecord    m_ChangeData;
};