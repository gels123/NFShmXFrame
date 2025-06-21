// -------------------------------------------------------------------------
//    @FileName         :    NFResDb.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFResDb.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFPluginModule/NFBaseObj.h"

#define CHECK_DESC_RELOADING(DESCSTORENAME) if (DESCSTORENAME::Instance(m_pObjPluginManager)->IsReloading()) return true;

#define IMPL_RES_HASH_DESC(class_name, DESCCLASSNAME, DESCSTORENAME, DESCNUM) \
    private:\
    NFShmVector<DESCCLASSNAME, DESCNUM> m_astDesc;\
    NFShmHashMap<uint64_t, int, DESCNUM> m_astDescMap;\
    public:\
    virtual int GetResNum() const override { return m_astDesc.size();}\
    NFShmVector<DESCCLASSNAME, DESCNUM>& GetResDesc() { return m_astDesc; }\
    const NFShmVector<DESCCLASSNAME, DESCNUM>& GetResDesc() const { return m_astDesc; } \
    NFShmVector<DESCCLASSNAME, DESCNUM>* GetResDescPtr() { return &m_astDesc; }\
    const NFShmVector<DESCCLASSNAME, DESCNUM>* GetResDescPtr() const { return &m_astDesc; }\
    virtual int Initialize() override\
    {\
        return 0;\
    }\
    virtual int Reload(NFResDB *pDB) override\
    {\
        PrepareReload();\
        int iRetCode = Load( pDB );\
        return iRetCode;\
    }\
    virtual std::string GetFileName() override\
    {\
        return std::string(#DESCSTORENAME);\
    }\
    virtual int Load(NFResDB* pDB) override;\
    virtual int CheckWhenAllDataLoaded() override;\
    virtual int CalcUseRatio() override\
    {\
        return m_astDesc.size() * 100 / m_astDesc.max_size();\
    }\
    virtual int SaveDescStore() override\
    {\
        if (!IsLoaded()) return 0;\
        for(int i = 0; i < (int)m_astDesc.size(); i++)\
        {\
            if (m_astDesc[i].IsUrgentNeedSave())\
            {\
                auto pb = DESCCLASSNAME::make_pbmsg();\
                m_astDesc[i].write_to_pbmsg(pb);\
                SaveDescStoreToDB(&pb);\
                m_astDesc[i].ClearUrgent();\
            }\
        }\
        return 0;\
    }\
    int InsertDescStore(const DESCCLASSNAME& desc)\
    {\
        auto pb = DESCCLASSNAME::make_pbmsg();\
        desc.write_to_pbmsg(pb);\
        InsertDescStoreToDB(&pb);\
        return 0;\
    }                                                        \
    int DeleteDescStore(const DESCCLASSNAME& desc)\
    {\
        auto pb = DESCCLASSNAME::make_pbmsg();\
        desc.write_to_pbmsg(pb);\
        DeleteDescStoreToDB(&pb);\
        return 0;\
    }\


#define IMPL_RES_SELF_CREATE_DESC(class_name, DESCCLASSNAME, DESCSTORENAME, DESCNUM) \
    public:\
    virtual int GetResNum() const override { return 0;}\
    virtual int Initialize() override\
    {\
        return 0;\
    }\
    virtual int Reload(NFResDB *pDB) override\
    {\
        PrepareReload();\
        int iRetCode = Load( pDB );\
        return iRetCode;\
    }\
    virtual std::string GetFileName() override\
    {\
        return std::string(#DESCSTORENAME);\
    }\
    virtual int Load(NFResDB* pDB) override;\
    virtual int CheckWhenAllDataLoaded() override;\
    virtual int CalcUseRatio() override\
    {\
        return 0;\
    }\
    virtual int SaveDescStore() override\
    {\
        return 0;\
    }\
    int InsertDescStore(const DESCCLASSNAME& desc)\
    {\
        auto pb = DESCCLASSNAME::make_pbmsg();\
        desc.write_to_pbmsg(pb);\
        InsertDescStoreToDB(&pb);\
        return 0;\
    }                                                        \
    int DeleteDescStore(const DESCCLASSNAME& desc)\
    {\
        auto pb = DESCCLASSNAME::make_pbmsg();\
        desc.write_to_pbmsg(pb);\
        DeleteDescStoreToDB(&pb);\
        return 0;\
    }\

#define IMPL_RES_DESC_EX(DESCSTORENAME) \
    public:\
    virtual int Reload() override\
    {\
        PrepareReload();\
        int iRetCode = Load();\
        return iRetCode;\
    }\
    virtual int PrepareReload() override;\
    virtual int Load() override;\
    virtual int CheckWhenAllDataLoaded() override;\


#define MAX_MD5_STR_LEN  33
#define MAX_DESC_FILE_PATH_STR_LEN  512
constexpr int MAX_DESC_NAME_LEN = 64;

enum
{
	DESC_CHECK_FATAL = -1000,
};

class NFResTable : public NFBaseObj
{
public:
    explicit NFResTable(NFIPluginManager* p): NFBaseObj(p)
    {
    }

    ~NFResTable() override
    {
    }

    virtual int FindAllRecord(const std::string &serverId, google::protobuf::Message *pMessage) = 0;
    virtual int FindOneRecord(const std::string &serverId, google::protobuf::Message *pMessage) = 0;
    virtual int SaveOneRecord(const std::string &serverId, const google::protobuf::Message *pMessage) = 0;
    virtual int InsertOneRecord(const std::string &serverId, const google::protobuf::Message *pMessage) = 0;
    virtual int DeleteOneRecord(const std::string &serverId, const google::protobuf::Message* pMessage) = 0;
};

class NFResDb : public NFBaseObj
{
public:
    explicit NFResDb(NFIPluginManager* p): NFBaseObj(p)
    {
    }

    ~NFResDb() override
    {
    }

    /**从数据库里取得一个资源表
    *@param name 资源表的名字， 必须和资源结构说明文件里面的结构名字， 以及数据库中table名字一致
    *@retval ==NULL 没有这个资源表
    *@retval !=NULL 资源表的指针
    */
	virtual NFResTable *GetTable(const std::string& name) = 0;
};

