// -------------------------------------------------------------------------
//    @FileName         :    NFIDescDbTemplate.h
//    @Author           :    gaoyi
//    @Date             :    23-10-26
//    @Email			:    445267987@qq.com
//    @Module           :    NFIDescDbTemplate
//
// -------------------------------------------------------------------------

#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStore.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmHashMultiMap.h"
#include "NFComm/NFShmStl/NFShmHashMultiSet.h"
#include "NFComm/NFShmStl/NFShmHashSet.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFComm/NFObjCommon/NFObjectTemplate.h"
#include "NFComm/NFCore/NFStringUtility.h"

template<typename className, typename className_s, int classType, int DescNum>
class NFIDescStoreTemplate : public NFShmObjGlobalTemplate<className, classType, NFIDescStore>
{
public:
    NFIDescStoreTemplate()
    {
        if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {
            CreateInit();
        }
        else {
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
public:
    virtual int GetResNum() const override { return m_astDescVec.size(); }
    NFShmVector<className_s, DescNum> &GetResDesc() { return m_astDescVec; }
    const NFShmVector<className_s, DescNum> &GetResDesc() const { return m_astDescVec; }
    const className_s* GetDescByIndex(int index) const
    {
        CHECK_EXPR(index >= 0 && index < (int)m_astDescVec.size(), NULL, "index:{} size:{}", index, m_astDescVec.size());
        return &m_astDescVec[index];
    }

    className_s* GetDescByIndex(int index)
    {
        return const_cast<className_s *>((static_cast<const className*>(this))->GetDescByIndex(index));
    }

    const className_s* GetDesc(int64_t id) const
    {
        auto iter = m_astDescMap.find(id);
        if (iter != m_astDescMap.end())
        {
            return GetDescByIndex(iter->second);
        }
        return NULL;
    }

    className_s* GetDesc(int64_t id)
    {
        return const_cast<className_s *>((static_cast<const className*>(this))->GetDesc(id));
    }
public:
    virtual int Initialize() override { return 0; }

    virtual int Load(NFResDB *pDB) override { return 0; }

    virtual int CheckWhenAllDataLoaded() override { return 0; }

    virtual int Reload(NFResDB *pDB) override
    {
        this->PrepareReload();
        int iRetCode = this->Load(pDB);
        return iRetCode;
    }

    virtual std::string GetFileName() override
    {
        std::string strSubModuleName = typeid(className).name();

#if NF_PLATFORM == NF_PLATFORM_WIN

        std::size_t position = strSubModuleName.find(' ');
        if (string::npos != position)
        {
            strSubModuleName = strSubModuleName.substr(position + 1, strSubModuleName.length());
        }
#else
        for (int i = 0; i < (int) strSubModuleName.length(); i++)
        {
            std::string s = strSubModuleName.substr(0, i + 1);
            int n = atof(s.c_str());
            if ((int) strSubModuleName.length() == i + 1 + n)
            {
                strSubModuleName = strSubModuleName.substr(i + 1, strSubModuleName.length());
                break;
            }
        }
#endif
        int pos = strSubModuleName.rfind("Desc");
        if (pos != std::string::npos)
        {
            strSubModuleName = strSubModuleName.substr(0, pos);
        }
        strSubModuleName = "E_" + strSubModuleName;
        return strSubModuleName;
    }

    virtual int CalcUseRatio() override
    {
        return m_astDescVec.size() * 100 / m_astDescVec.max_size();
    }

    virtual int SaveDescStore() override
    {
        if (!this->IsLoaded())
            return 0;
        for (auto iter = m_astDescVec.begin(); iter != m_astDescVec.end(); iter++)
        {
            if (iter->IsUrgentNeedSave())
            {
                auto pb = iter->ToPb();
                this->SaveDescStoreToDB(&pb);
                iter->ClearUrgent();
            }
        }
        return 0;
    }

    virtual int InsertDescStore(const className_s &desc) {
        auto pb = desc.ToPb();
        this->InsertDescStoreToDB(&pb);
        return 0;
    }

    virtual int DeleteDescStore(const className_s &desc)
    {
        auto pb = desc.ToPb();
        this->DeleteDescStoreToDB(&pb);
        return 0;
    }
protected:
    NFShmVector<className_s, DescNum> m_astDescVec;
    NFShmHashMap<int64_t, int, DescNum> m_astDescMap;
};