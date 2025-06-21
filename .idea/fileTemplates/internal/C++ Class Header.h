// -------------------------------------------------------------------------
//    @FileName         :    ${FILE_NAME}
//    @Author           :    $USER_NAME
//    @Date             :    ${DATE}
//    @Email            :    445267987@qq.com
//    @Module           :    ${NAME}
//
// -------------------------------------------------------------------------

#pragma once

#[[#include]]# "NFComm/NFCore/NFPlatform.h"
#[[#include]]# "NFComm/NFShmCore/NFShmObj.h"
#[[#include]]# "NFComm/NFShmCore/NFShmMgr.h"
#[[#include]]# "NFComm/NFShmCore/NFISharedMemModule.h"

class ${NAME} : public NFShmObjTemplate<${NAME}, EOT_ID, NFShmObj>
{
public:
    ${NAME}();
    
    virtual ~${NAME}();
    
    int CreateInit();
    int ResumeInit();
};