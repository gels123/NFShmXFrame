// -------------------------------------------------------------------------
//    @FileName         :    NFDBObjMgr.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFDBObjMgr.h
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFShmStl/NFShmHashSet.h>
#include <NFComm/NFShmStl/NFShmList.h>

#include "NFComm/NFObjCommon/NFObject.h"

class NFBaseDBObj;
class NFDBObjTrans;
class NFDBObjMgr : public NFObjectTemplate<NFDBObjMgr, EOT_TRANS_DB_OBJ_MGR, NFObject>
{
public:
    NFDBObjMgr();
    virtual ~NFDBObjMgr();

    //非继承函数, 不要加virtual
    int CreateInit();
    //非继承函数, 不要加virtual
    int ResumeInit();

    //must be virtual
    virtual int OnTimer(int timeId, int callcount) override;
public:
    NFBaseDBObj* GetObj(int iObjID);
    int LoadFromDB(NFBaseDBObj* pObj);
    int OnDataLoaded(int iObjID, int32_t err_code, const google::protobuf::Message* pData);
    int OnDataInserted(NFDBObjTrans* trans, bool success);
    int OnDataSaved(NFDBObjTrans* trans, bool success);
    int SaveToDB(NFBaseDBObj* pObj);
    int Tick();
    int CheckWhenAllDataLoaded();
public:
    /*
     * 停服之前，检查服务器是否满足停服条件
     * */
    virtual bool CheckStopServer();

    /*
     * 停服之前，做一些操作，满足停服条件
     * */
    virtual bool StopServer();
private:
    int m_iLastSavingObjIndex;
    uint32_t m_iLastTickTime;
    int m_iTransMngObjID;
    int m_iTimer;
    NFShmList<int, 1024> m_runningObjList;
    NFShmList<int, 1024> m_failedObjList;
    NFShmHashSet<int, 1024> m_loadDBList;
    NFShmHashSet<int, 1024> m_loadDBFinishList;
};
