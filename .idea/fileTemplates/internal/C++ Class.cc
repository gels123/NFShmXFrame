// -------------------------------------------------------------------------
//    @FileName         :    ${FILE_NAME}
//    @Author           :    $USER_NAME
//    @Date             :    ${DATE}
//    @Email            :    445267987@qq.com
//    @Module           :    ${NAME}
//
// -------------------------------------------------------------------------

#[[#include]]# "${HEADER_FILENAME}"

${NAME}::${NAME}()
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

${NAME}::~${NAME}()
{
}

int ${NAME}::CreateInit()
{
    return 0;
}
    
int ${NAME}::ResumeInit()
{
	return 0;
}