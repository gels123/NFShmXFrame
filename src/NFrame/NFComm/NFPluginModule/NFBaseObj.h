// -------------------------------------------------------------------------
//    @FileName         :    NFBaseObj.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFIPluginManager.h"
#include <stddef.h>
#include <string.h>

class NFBaseObj
{
public:
    NFBaseObj(NFIPluginManager *p): m_pObjPluginManager(p)
    {
        //NF_ASSERT_MSG(m_pObjPluginManager != NULL, "m_pObjPluginManager == nullptr")
    }

    virtual ~NFBaseObj()
    {

    }

    template <typename T>
    T* FindModule() const
    {
        if (m_pObjPluginManager)
        {
            return m_pObjPluginManager->FindModule<T>();
        }
        return nullptr;
    }
public:
    NFIPluginManager* m_pObjPluginManager;
};