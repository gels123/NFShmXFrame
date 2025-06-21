// -------------------------------------------------------------------------
//    @FileName         :    NFObjDefine.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------


#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include <functional>

#define OBJECT_MAGIC_CHECK_NUMBER 1234567890
const int INVALID_ID = -1;
const int MAX_PATH_LEN = 192;
const uint64_t ULL_MAX = 0xffffffff;

const int C_I_MAX_OBJSEG_COUNT = 255;

typedef enum
{
	EN_OBJ_MODE_INIT = 1,
	EN_OBJ_MODE_RECOVER = 2,
} EN_OBJ_MODE;

inline void debug_assert()
{
#ifdef NF_DEBUG_MODE
	assert(0);
#endif
}

class NFObject;
typedef std::function<bool(NFObject* pObj)> DESTROY_OBJECT_AUTO_ERASE_FUNCTION;

typedef uint64_t NFObjectHashKey;
