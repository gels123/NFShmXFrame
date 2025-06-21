// -------------------------------------------------------------------------
//    @FileName         :    NFObjectTemplate.h
//    @Author           :    gaoyi
//    @Date             :    23-10-26
//    @Email			:    445267987@qq.com
//    @Module           :    NFObjectTemplate
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFPluginModule/NFMemTracker.h>

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFObjCommon/NFObjectIterator.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"

template <typename ClassName, int ClassType, typename ParentClassName>
class NFObjectTemplate : public ParentClassName
{
	static_assert(!std::is_same<ParentClassName, ClassName>::value, "parentClassName is equal to the className");
public:
	typedef NFObjectIterator<ClassName, ClassName&, ClassName*, NFIMemMngModule> iterator;
	typedef NFObjectIterator<ClassName, const ClassName&, const ClassName*, NFIMemMngModule> const_iterator;
	typedef std::reverse_iterator<const_iterator>                                                 const_reverse_iterator;
	typedef std::reverse_iterator<iterator>                                                       reverse_iterator;
public:
	NFObjectTemplate()
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

	virtual ~NFObjectTemplate()
	{
	}

	static int GetStaticItemCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetItemCount(ClassType);
	}

	static int GetStaticUsedCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetUsedCount(ClassType);
	}

	static int GetStaticFreeCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetFreeCount(ClassType);
	}

	static std::string GetStaticClassName()
	{
		return typeid(ClassName).name();
	}

	static bool IsEnd(int iIndex)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IsEnd(ClassType, iIndex);
	}

	static int GetStaticClassType()
	{
		return ClassType;
	}

	static iterator Begin()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IterBegin(ClassType);
	}

	static iterator End()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IterEnd(ClassType);
	}

	static reverse_iterator RBegin()
	{
		return reverse_iterator(End());
	}

	static reverse_iterator REnd()
	{
		return reverse_iterator(Begin());
	}

	static iterator Erase(iterator iter)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->Erase(iter);
	}

	static bool IsValid(iterator iter)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IsValid(iter);
	}

	static ClassName* CreateObj()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateObj(ClassType));
	}

	static ClassName* CreateObjByHashKey(NFObjectHashKey hashKey)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateObjByHashKey(ClassType, hashKey));
	}

	static ClassName* CreateTrans()
	{
		static_assert(std::is_base_of<NFTransBase, ClassName>::value, "className is not inherit to NFTransBase");
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateTrans(ClassType));
	}

	static ClassName* GetTrans(uint64_t ullTransId)
	{
		static_assert(std::is_base_of<NFTransBase, ClassName>::value, "className is not inherit to NFTransBase");
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetTrans(ullTransId));
	}

	static void DestroyObj(ClassName* pObj)
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->DestroyObj(pObj);
	}

	static void ClearAllObj()
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->ClearAllObj(ClassType);
	}

	static ClassName* GetObjByObjId(int iObjId)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByObjId(ClassType, iObjId));
	}

	static ClassName* GetObjByGlobalId(int iGlobalId, bool withChildrenType = false)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByGlobalId(ClassType, iGlobalId, withChildrenType));
	}

	static ClassName* GetObjByHashKey(NFObjectHashKey hashKey)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByHashKey(ClassType, hashKey));
	}

	static int DestroyObjAutoErase(int maxNum = INVALID_ID, const DESTROY_OBJECT_AUTO_ERASE_FUNCTION& func = nullptr)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->DestroyObjAutoErase(ClassType, maxNum, func);
	}

	static ClassName* Instance()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetHeadObj(ClassType));
	}

	static ClassName* GetInstance()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetHeadObj(ClassType));
	}

	static int RegisterClassToObjSeg(int bType, size_t siObjSize, int iObjCount, const std::string& strClassName, bool useHash, bool singleton = false)
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->RegisterClassToObjSeg(bType, siObjSize, iObjCount, ClassName::ResumeObjectRegisterFunction,
		                                                                                                           ClassName::CreateObjectRegisterFunction, ClassName::DestroyObjectRegisterFunction, ParentClassName::GetStaticClassType(),
		                                                                                                           strClassName, useHash, singleton);
		return 0;
	}

	void* operator new(size_t, void* pBuffer) throw()
	{
		return pBuffer;
	}

	static NFObject* CreateObjectRegisterFunction()
	{
		ClassName* pTmp = nullptr;
		void* pVoid = NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->AllocMemForObject(ClassType);
		if (!pVoid)
		{
			NFLogError(NF_LOG_DEFAULT, 0, "ERROR: class:{}, Item:{}, Used:{}, Free:{}", GetStaticClassName(), GetStaticItemCount(), GetStaticUsedCount(), GetStaticFreeCount());
			return nullptr;
		}
		pTmp = new(pVoid) ClassName();
		return pTmp;
	}

	static NFObject* ResumeObjectRegisterFunction(void* pVoid)
	{
		ClassName* pTmp = nullptr;
		pTmp = new(pVoid) ClassName();
		return pTmp;
	}

	static void DestroyObjectRegisterFunction(NFObject* pObj)
	{
		ClassName* pTmp = nullptr;
		pTmp = dynamic_cast<ClassName*>(pObj);
		(*pTmp).~ClassName();
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->FreeMemForObject(ClassType, pTmp);
	}

public:
	virtual int GetClassType() const
	{
		return ClassType;
	}
};

template <typename ClassName, int ClassType>
class NFObjectTemplateNoParent
{
public:
	typedef NFObjectIterator<ClassName, ClassName&, ClassName*, NFIMemMngModule> iterator;
	typedef NFObjectIterator<ClassName, const ClassName&, const ClassName*, NFIMemMngModule> const_iterator;
	typedef std::reverse_iterator<const_iterator>                                                 const_reverse_iterator;
	typedef std::reverse_iterator<iterator>                                                       reverse_iterator;
public:
	NFObjectTemplateNoParent()
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

	virtual ~NFObjectTemplateNoParent()
	{
	}

public:
	static int GetStaticItemCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetItemCount(ClassType);
	}

	static int GetStaticUsedCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetUsedCount(ClassType);
	}

	static int GetStaticFreeCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetFreeCount(ClassType);
	}

	static bool IsEnd(int iIndex)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IsEnd(ClassType, iIndex);
	}

	static std::string GetStaticClassName()
	{
		return typeid(ClassName).name();
	}

	static int GetStaticClassType()
	{
		return ClassType;
	}

	static iterator Begin()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IterBegin(ClassType);
	}

	static iterator End()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IterEnd(ClassType);
	}

	static reverse_iterator RBegin()
	{
		return reverse_iterator(End());
	}

	static reverse_iterator REnd()
	{
		return reverse_iterator(Begin());
	}

	static iterator Erase(iterator iter)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->Erase(iter);
	}

	static bool IsValid(iterator iter)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IsValid(iter);
	}

	static ClassName* CreateObj()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateObj(ClassType));
	}

	static ClassName* CreateObjByHashKey(NFObjectHashKey hashKey)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateObjByHashKey(ClassType, hashKey));
	}

	static void DestroyObj(ClassName* pObj)
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->DestroyObj(pObj);
	}

	static void ClearAllObj()
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->ClearAllObj(ClassType);
	}

	static ClassName* GetObjByObjId(int iObjId)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByObjId(ClassType, iObjId));
	}

	static ClassName* GetObjByGlobalId(int iGlobalId, bool withChildrenType = false)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByGlobalId(ClassType, iGlobalId, withChildrenType));
	}

	static ClassName* GetObjByHashKey(NFObjectHashKey hashKey)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByHashKey(ClassType, hashKey));
	}

	static ClassName* CreateTrans()
	{
		static_assert(std::is_base_of<NFTransBase, ClassName>::value, "className is not inherit to NFTransBase");
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateTrans(ClassType));
	}

	static ClassName* GetTrans(uint64_t ullTransId)
	{
		static_assert(std::is_base_of<NFTransBase, ClassName>::value, "className is not inherit to NFTransBase");
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetTrans(ullTransId));
	}

	static int DestroyObjAutoErase(int maxNum = INVALID_ID, const DESTROY_OBJECT_AUTO_ERASE_FUNCTION& func = nullptr)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->DestroyObjAutoErase(ClassType, maxNum, func);
	}

	static ClassName* Instance()
	{
		return dynamic_cast<ClassName *>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetHeadObj(ClassType));
	}

	static ClassName* GetInstance()
	{
		return dynamic_cast<ClassName *>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetHeadObj(ClassType));
	}

	static int RegisterClassToObjSeg(int bType, size_t siObjSize, int iObjCount, const std::string& strClassName, bool useHash, bool singleton = false)
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->RegisterClassToObjSeg(bType, siObjSize, iObjCount, ClassName::ResumeObjectRegisterFunction,
		                                                                                                           ClassName::CreateObjectRegisterFunction, ClassName::DestroyObjectRegisterFunction, -1,
		                                                                                                           strClassName, useHash, singleton);
		return 0;
	}

	void* operator new(size_t, void* pBuffer) throw()
	{
		return pBuffer;
	}

	static NFObject* CreateObjectRegisterFunction()
	{
		ClassName* pTmp  = nullptr;
		void* pVoid = NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->AllocMemForObject(ClassType);
		if (!pVoid)
		{
			NFLogError(NF_LOG_DEFAULT, 0, "ERROR: class:{}, Item:{}, Used:{}, Free:{}", GetStaticClassName(), GetStaticItemCount(), GetStaticUsedCount(), GetStaticFreeCount());
			return nullptr;
		}
		pTmp = new(pVoid) ClassName();
		return pTmp;
	}

	static NFObject* ResumeObjectRegisterFunction(void* pVoid)
	{
		ClassName* pTmp = nullptr;
		pTmp = new(pVoid) ClassName();
		return pTmp;
	}

	static void DestroyObjectRegisterFunction(NFObject* pObj)
	{
		ClassName* pTmp = nullptr;
		pTmp = dynamic_cast<ClassName*>(pObj);
		(*pTmp).~ClassName();
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->FreeMemForObject(ClassType, pTmp);
	}

public:
	virtual int GetClassType() const
	{
		return ClassType;
	}
};

template <typename ClassName, int ClassType, typename ParentClassName>
class NFShmObjGlobalTemplate : public ParentClassName
{
public:
	typedef NFObjectIterator<ClassName, ClassName &, ClassName*, NFIMemMngModule> iterator;
	typedef NFObjectIterator<ClassName, const ClassName&, const ClassName*, NFIMemMngModule> const_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef std::reverse_iterator<iterator>                                                       reverse_iterator;
public:
	NFShmObjGlobalTemplate()
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

	virtual ~NFShmObjGlobalTemplate()
	{
	}

public:
	static int GetStaticItemCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetItemCount(ClassType);
	}

	static int GetStaticUsedCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetUsedCount(ClassType);
	}

	static int GetStaticFreeCount()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetFreeCount(ClassType);
	}

	static bool IsEnd(int iIndex)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IsEnd(ClassType,iIndex);
	}

	static std::string GetStaticClassName()
	{
		return typeid(ClassName).name();
	}

	static int GetStaticClassType()
	{
		return ClassType;
	}

	static iterator Begin()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IterBegin(ClassType);
	}

	static iterator End()
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IterEnd(ClassType);
	}

	static reverse_iterator RBegin()
	{
		return reverse_iterator(End());
	}

	static reverse_iterator REnd()
	{
		return reverse_iterator(Begin());
	}

	static iterator Erase(iterator iter)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->Erase(iter);
	}

	static bool IsValid(iterator iter)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->IsValid(iter);
	}

	static ClassName* CreateObj()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateObj(ClassType));
	}

	static ClassName* CreateObjByHashKey(NFObjectHashKey hashKey)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateObjByHashKey(ClassType, hashKey));
	}

	static ClassName* CreateTrans()
	{
		static_assert(std::is_base_of<NFTransBase, ClassName>::value, "className is not inherit to NFTransBase");
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->CreateTrans(ClassType));
	}

	static ClassName* GetTrans(uint64_t ullTransId)
	{
		static_assert(std::is_base_of<NFTransBase, ClassName>::value, "className is not inherit to NFTransBase");
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetTrans(ullTransId));
	}

	static void DestroyObj(ClassName* pObj)
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->DestroyObj(pObj);
	}

	static void ClearAllObj()
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->ClearAllObj(ClassType);
	}

	static ClassName* GetObjByObjId(int iObjId)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByObjId(ClassType, iObjId));
	}

	static ClassName* GetObjByGlobalId(int iGlobalId, bool withChildrenType = false)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByGlobalId(ClassType, iGlobalId, withChildrenType));
	}

	static ClassName* GetObjByHashKey(NFObjectHashKey hashKey)
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetObjByHashKey(ClassType, hashKey));
	}

	static int DestroyObjAutoErase(int maxNum = INVALID_ID, const DESTROY_OBJECT_AUTO_ERASE_FUNCTION& func = nullptr)
	{
		return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->DestroyObjAutoErase(ClassType, maxNum, func);
	}

	static ClassName* Instance()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetHeadObj(ClassType));
	}

	static ClassName* GetInstance()
	{
		return dynamic_cast<ClassName*>(NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->GetHeadObj(ClassType));
	}

	static int RegisterClassToObjSeg(int bType, size_t siObjSize, int iObjCount, const std::string& strClassName, bool useHash, bool singleton = false)
	{
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->RegisterClassToObjSeg(bType, siObjSize, iObjCount, ClassName::ResumeObjectRegisterFunction,
		                                                                                                           ClassName::CreateObjectRegisterFunction, ClassName::DestroyObjectRegisterFunction, ParentClassName::GetStaticClassType(),
		                                                                                                           strClassName, useHash, singleton);
		return 0;
	}

	void* operator new(size_t, void* pBuffer) throw()
	{
		return pBuffer;
	}

	static NFObject* CreateObjectRegisterFunction()
	{
		ClassName* pTmp = nullptr;
		void* pVoid = NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->AllocMemForObject(ClassType);
		if (!pVoid)
		{
			NFLogError(NF_LOG_DEFAULT, 0, "ERROR: class:{}, Item:{}, Used:{}, Free:{}", GetStaticClassName(), GetStaticItemCount(), GetStaticUsedCount(), GetStaticFreeCount());
			return nullptr;
		}
		pTmp = new(pVoid) ClassName();
		return pTmp;
	}

	static NFObject* ResumeObjectRegisterFunction(void* pVoid)
	{
		ClassName* pTmp = nullptr;
		pTmp = new(pVoid) ClassName();
		return pTmp;
	}

	static void DestroyObjectRegisterFunction(NFObject* pObj)
	{
		ClassName* pTmp = nullptr;
		pTmp = dynamic_cast<ClassName*>(pObj);
		(*pTmp).~ClassName();
		NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<NFIMemMngModule>()->FreeMemForObject(ClassType, pTmp);
	}

public:
	virtual int GetClassType() const
	{
		return ClassType;
	}
};
