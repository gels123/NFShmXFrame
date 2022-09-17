// -------------------------------------------------------------------------
//    @FileName         :    NFShmMgr.h
//    @Author           :    xxxxx
//    @Date             :   xxxx-xx-xx
//    @Email			:    xxxxxxxxx@xxx.xxx
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include "NFComm/NFCore/NFSingleton.hpp"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFTypeDefines.h"
#include "NFShmDefine.h"
#include "NFTypeDefines.h"

class NFShmObj;
class NFISharedMemModule;
class NFTransBase;
class NFShmTimerObj;

class NFShmMgr : public NFSingleton<NFShmMgr>
{
public:
	NFShmMgr();
	virtual ~NFShmMgr();
public:
	/**
	* �����ڴ洴������ģʽ
	*/
	virtual EN_OBJ_MODE	GetCreateMode();

	/**
	* �����ڴ洴������ģʽ
	*/
	virtual void	SetCreateMode(EN_OBJ_MODE mode);

    /**
    * @brief  ���ƫ�Ƶ�ַ
    */
    virtual size_t   GetAddrOffset();

    virtual void SetAddrOffset(size_t offset);
public:
    EN_OBJ_MODE m_objMode;
    /**
    * ������ϴι����ڴ��ַrecover֮���ƫ����,�����ָ�ָ�����
    */
    size_t  m_siAddrOffset;
};

