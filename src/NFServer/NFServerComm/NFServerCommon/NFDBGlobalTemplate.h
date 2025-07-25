// -------------------------------------------------------------------------
//    @FileName         :    NFDBGlobalTemplate.h
//    @Author           :    gaoyi
//    @Date             :    23-11-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFDBGlobalTemplate
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFCore/NFCommon.h>
#include <NFComm/NFPluginModule/NFStoreProtoCommon.h>

#include "NFBaseDBObj.h"
#include "NFDBObjMgr.h"

template<typename className, typename pbClass, int classType>
class NFDBGlobalTemplate : public NFObjectTemplate<className, classType, NFBaseDBObj>
{
public:
    NFDBGlobalTemplate()
    {
        if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode())
        {
            CreateInit();
        }
        else
        {
            ResumeInit();
        }
    }

    int CreateInit()
    {
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    virtual ~NFDBGlobalTemplate()
    {
    }

    virtual int Load(NF_SERVER_TYPE serverType)
    {
        NFBaseDBObj::SetServerType(serverType);
        return NFDBObjMgr::Instance()->LoadFromDB(this);
    }

    virtual int MakeLoadData(google::protobuf::Message *data)
    {
        CHECK_NULL(0, data);
        pbClass *p = dynamic_cast<pbClass *>(data);
        CHECK_NULL(0, p);

        int iRetCode = NFProtobufCommon::SetPrimarykeyFromMessage(p, NFCommon::tostr(GetDbId()));
        CHECK_EXPR(iRetCode == 0, -1, "SetPrimarykeyFromMessage Failed From {}", data->GetTypeName());

        return 0;
    }

    virtual int MakeSaveData(google::protobuf::Message *data)
    {
        if (!this->m_bDataInited)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "not inited !");
            return -1;
        }
        CHECK_NULL(0, data);

        pbClass *p = dynamic_cast<pbClass *>(data);
        CHECK_NULL(0, p);

        int iRetCode = NFProtobufCommon::SetPrimarykeyFromMessage(p, NFCommon::tostr(GetDbId()));
        CHECK_EXPR(iRetCode == 0, -1, "SetPrimarykeyFromMessage Failed From {}", data->GetTypeName());
        return SaveToDB(*p);
    }

    virtual int InitWithDBData(const google::protobuf::Message *pData)
    {
        const pbClass *pGlobal = dynamic_cast<const pbClass *>(pData);
        CHECK_NULL(0, pGlobal);

        std::string dbId;
        int iRetCode = NFProtobufCommon::GetPrimarykeyFromMessage(pGlobal, dbId);
        CHECK_EXPR(iRetCode == 0, -1, "GetPrimarykeyFromMessage Failed From {}", pData->GetTypeName());
        CHECK_EXPR(NFCommon::strto<int>(dbId) == GetDbId(), -1, "invalid data!");
        int iRet = LoadFromDB(*pGlobal);
        CHECK_EXPR(iRet == 0, -1, "parse failed!");

        this->m_bDataInited = true;
        return 0;
    }

    virtual int InitWithoutDBData()
    {
        int iRet = InitConfig();
        CHECK_EXPR(iRet == 0, -1, "InitConfig failed!");
        this->m_bDataInited = true;
        return 0;
    }

    virtual int LoadFromDB(const pbClass &dbData) = 0;

    virtual int SaveToDB(pbClass &dbData) = 0;

    virtual int InitConfig() = 0;

    virtual int GetDbId() = 0;

    virtual google::protobuf::Message *CreateTempProtobufData() { return new pbClass(); }
    virtual uint32_t GetSaveDis() { return 60; }
    virtual eDealWithLoadFailed DealWithFailed() { return EN_DW_SHUTDOWN; }
};
