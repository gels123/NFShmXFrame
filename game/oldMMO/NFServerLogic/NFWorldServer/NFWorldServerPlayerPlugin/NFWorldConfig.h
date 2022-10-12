// -------------------------------------------------------------------------
//    @FileName         :    NFWorldConfig.h
//    @Author           :    gaoyi
//    @Date             :    2022/10/12
//    @Email			:    445267987@qq.com
//    @Module           :    NFWorldConfig
//
// -------------------------------------------------------------------------

#pragma once


#include <common_logic_s.h>
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFLogicCommon/NFServerFrameTypeDefines.h"
#include "NFLogicCommon/NFLogicCommonDefine.h"

class NFWorldConfig : public NFShmObj
{
public:
    NFWorldConfig(NFIPluginManager *pPluginManager);

    virtual ~NFWorldConfig();

    int CreateInit();

    int ResumeInit();

public:
    /**
     * @brief ����Server/WorldServer.lua�е�WolrdServer���ã�
     *        ͨ��common_logic.proto�е�protobuf�ṹWorldServerConfig��������ע��fieldҪ��lua���Զ���Сд��ȫһ��
     *        lua:                                   proto:
     *        WorldServer = {                        message WorldExternalConfig {
                    TokenTimeCheck = True;   ====>      optional bool TokenTimeCheck = 1[(yd_fieldoptions.field_use_stl)=true];
              };                                    };
     *
     * @param luaMgr
     * @return
     */
    int LoadConfig(NFILuaModule luaMgr);

    /**
     * @brief ��ȡ����
     * @return
     */
    const proto_ff_s::WorldExternalConfig_s *GetConfig() const;

private:
    /**
     * @brief �����������������
     */
    proto_ff_s::WorldExternalConfig_s m_config;
DECLARE_IDCREATE(NFWorldConfig)
};