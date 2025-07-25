// -------------------------------------------------------------------------
//    @FileName         :    NFTypeDefines.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once
#include "NFComm/NFCore/NFPlatform.h"

typedef enum
{
	EOT_OBJECT = 0,//0-100的是基础引擎共享内存ID
	EOT_GLOBAL_ID = 1, //位置不可修改
    EOT_TYPE_TIMER_OBJ = 2, //位置不可修改
    EOT_TYPE_TIMER_MNG = 3, //位置不可修改
    EOT_TYPE_SUBSCRIBEINFO_OBJ = 4, //位置不可修改
    EOT_TYPE_EVENT_MGR = 5, //位置不可修改
    EOT_TRANS_BASE = 6,
    EOT_TRANS_MNG = 7,
    EOT_BASE_DB_OBJ = 8,
    EOT_TRANS_DB_OBJ = 9,
    EOT_TRANS_DB_OBJ_MGR = 10,
    EOT_RPC_TRANS_ID = 11,

	EOT_SERVER_FRAME_BEGIN_ID = 100,//100-999的是服务器架构共享内存ID
	EOT_SERVER_FRAME_END_ID = 999,

	EOT_DESC_STORE_BEGIN_ID = 1000,
    EOT_DESC_STORE_END_ID = 2000,

	EOT_MAX_TYPE = 2000,
} EN_FRAME_SHMOBJ_TYPE;

#define REGISTER_SHM_OBJ(ClassName, ObjNum ) do{\
        ClassName::RegisterClassToObjSeg(ClassName::GetStaticClassType(), sizeof(ClassName), (ObjNum), std::string(#ClassName), false);\
    }while(0)

#define REGISTER_SINGLETON_SHM_OBJ(ClassName) do{\
        ClassName::RegisterClassToObjSeg(ClassName::GetStaticClassType(), sizeof(ClassName), 1, std::string(#ClassName), false, true);\
    }while(0)

#define REGISTER_SHM_OBJ_WITH_HASH( ClassName, ObjNum ) do{\
        ClassName::RegisterClassToObjSeg(ClassName::GetStaticClassType(), sizeof(ClassName), (ObjNum), std::string(#ClassName), true);\
    }while(0)

#define REGISTER_SHM_OBJ_GLOBAL(ClassName, ObjNum ) do{\
        ClassName::RegisterClassToObjSeg(ClassName::GetStaticClassType(), sizeof(ClassName), (ObjNum), std::string(#ClassName), false);\
    }while(0)

#define REGISTER_SINGLETON_SHM_OBJ_GLOBAL(ClassName) do{\
        ClassName::RegisterClassToObjSeg(ClassName::GetStaticClassType(), sizeof(ClassName), 1, std::string(#ClassName), false, true);\
    }while(0)

/**
 * @brief 反注册共享内存对象宏
 * @param ClassName 类名
 * @param ObjNum 对象数量（当前实现中未使用）
 * @note 用于从共享内存管理器中移除对象类型的注册信息
 * @warning 必须在确保没有该类型对象被使用时调用
 */
#define UNREGISTER_SHM_OBJ(ClassName, ObjNum ) do{\
        ClassName::UnRegisterClassToObjSeg(ClassName::GetStaticClassType());\
    }while(0)


