// -------------------------------------------------------------------------
//    @FileName         :    NFIAsyModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFIAsyModule
//
//
//                    .::::.
//                  .::::::::.
//                 :::::::::::  FUCK YOU
//             ..:::::::::::'
//           '::::::::::::'
//             .::::::::::
//        '::::::::::::::..
//             ..::::::::::::.
//           ``::::::::::::::::
//            ::::``:::::::::'        .:::.
//           ::::'   ':::::'       .::::::::.
//         .::::'      ::::     .:::::::'::::.
//        .:::'       :::::  .:::::::::' ':::::.
//       .::'        :::::.:::::::::'      ':::::.
//      .::'         ::::::::::::::'         ``::::.
//  ...:::           ::::::::::::'              ``::.
// ```` ':.          ':::::::::'                  ::::..
//                    '.:::::'                    ':'````..
//
// -------------------------------------------------------------------------


#pragma once

#include "NFIModule.h"
#include <vector>
#include "NFITaskModule.h"
#include "NFTask.h"

#include "google/protobuf/message.h"
#include "NFComm/NFKernelMessage/FrameSqlData.pb.h"
#include "NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

using SelectByCondCb = std::function<void(int iRet, NFrame::storesvr_sel_res& stSelectRes)>;

using SelectObjCb = std::function<void(int iRet, NFrame::storesvr_selobj_res& stSelectRes)>;

using DeleteByCondCb = std::function<void(int iRet, NFrame::storesvr_del_res& stSelectRes)>;

using DeleteObjCb = std::function<void(int iRet, NFrame::storesvr_delobj_res& stSelectRes)>;

using InsertObjCb = std::function<void(int iRet, NFrame::storesvr_insertobj_res& stSelectRes)>;

using ModifyByCondCb = std::function<void(int iRet, NFrame::storesvr_mod_res& stSelectRes)>;

using ModifyObjCb = std::function<void(int iRet, NFrame::storesvr_modobj_res& stSelectRes)>;

using UpdateByCondCb = std::function<void(int iRet, NFrame::storesvr_update_res& stSelectRes)>;
using UpdateObjCb = std::function<void(int iRet, NFrame::storesvr_updateobj_res& stSelectRes)>;

using ExecuteCb = std::function<void(int iRet, NFrame::storesvr_execute_res& stSelectRes)>;
using ExecuteMoreCb = std::function<void(int iRet, NFrame::storesvr_execute_more_res& stSelectRes)>;

class NFIAsyModule : public NFIModule {
public:
	explicit NFIAsyModule(NFIPluginManager* pPluginManager): NFIModule(pPluginManager)
	{
		m_bInitActor = false;
	}

	~NFIAsyModule() override = default;

	/**
	 * @brief 初始化Actor池
	 *
	 * 该函数用于初始化Actor池，设置任务组和Actor的最大数量。通过该函数，可以确保Actor池在后续操作中能够正确处理任务和Actor的分配。
	 *
	 * @param iMaxTaskGroup 最大任务组数量，用于限制任务组的并发数量。
	 * @param iMaxActorNum 最大Actor数量，用于限制Actor池中Actor的并发数量。
	 *
	 * @return bool 返回初始化是否成功，true表示成功，false表示失败。
	 */
	virtual bool InitActorPool(int iMaxTaskGroup, int iMaxActorNum)
	{
		if (!m_bInitActor)
		{
			int iMaxThread = 1;
			/*
			启动多线程任务系统
			*/
			if (m_pObjPluginManager->IsLoadAllServer())
			{
				iMaxThread = 1;
			} else {
				NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_NONE);
				NF_ASSERT(pConfig);

				iMaxThread = (int)pConfig->WorkThreadNum;
			}

			FindModule<NFITaskModule>()->InitActorThread(iMaxTaskGroup, iMaxThread);

			if (iMaxActorNum <= 0)
			{
				iMaxActorNum = iMaxThread * 2;
			}

			m_stVecActorGroupPool.resize(iMaxTaskGroup);

			for (int iTaskGroup = 0; iTaskGroup < (int)m_stVecActorGroupPool.size(); iTaskGroup++)
			{
				for (int i = 0; i < iMaxActorNum; i++)
				{
					int iActorId = FindModule<NFITaskModule>()->RequireActor(iTaskGroup);
					if (iActorId <= 0) {
						return false;
					}

					m_stVecActorGroupPool[iTaskGroup].push_back(iActorId);
				}
			}

			m_bInitActor = true;
		}
		return true;
	}

	/**
	 * @brief 检查指定任务组和actorId是否存在
	 *
	 * 该函数用于验证给定的任务组和actorId是否存在于系统中。通常用于任务分配或角色管理场景。
	 *
	 * @param iTaskGroup 任务组的标识符，用于指定要检查的任务组
	 * @param iActorId 角色的标识符，用于指定要检查的角色
	 * @return bool 返回一个布尔值，表示任务组和actorId是否存在。true表示存在，false表示不存在
	 */
	virtual bool Exist(int iTaskGroup, int iActorId)
	{
		CHECK_EXPR(iTaskGroup >= 0 && iTaskGroup < (int)m_stVecActorGroupPool.size(), false, "taskGroup:{} error", iTaskGroup);
		for (size_t i = 0; i < m_stVecActorGroupPool[iTaskGroup].size(); i++)
		{
			if (m_stVecActorGroupPool[iTaskGroup][i] == iActorId) {
				return true;
			}
		}
		return false;
	}

	/**
	* @brief 添加一个Actor组件
	*
	* @return
	*/
	virtual int AddActorComponent(int iTaskGroup, int iActorId, NFITaskComponent* pComponent)
	{
		return FindModule<NFITaskModule>()->AddActorComponent(iTaskGroup, iActorId, pComponent);
	}

	virtual NFITaskComponent* GetActorComponent(int iTaskGroup, int iActorId)
	{
		return FindModule<NFITaskModule>()->GetTaskComponent(iTaskGroup, iActorId);
	}

	/**
	* @brief 通过任务的动态均衡id，获得actor
	*		 为了防止数据库错乱，防止同时对数据库表中的一条数据，读取写入，
	*		 使用动态均衡id, 使得在某个时候只有一条线程对表中的一条数据，读取或写入
	* @param iTaskGroup
	* @param ullBalanceId 动态均衡id
	* @return	一个actor索引
	*/
	int GetBalanceActor(int iTaskGroup, uint64_t ullBalanceId)
	{
		CHECK_EXPR(iTaskGroup >= 0 && iTaskGroup < (int)m_stVecActorGroupPool.size(), -1, "taskGroup:{} error", iTaskGroup);
		if (ullBalanceId == 0)
		{
			return GetRandActor(iTaskGroup);
		}
		if (m_stVecActorGroupPool[iTaskGroup].empty())
		{
			return -1;
		}
		m_ullSuitIndex = ullBalanceId % m_stVecActorGroupPool[iTaskGroup].size();
		return m_stVecActorGroupPool[iTaskGroup][m_ullSuitIndex];
	}

	/**
	* @brief 随机获得一个actor
	*
	* @return actor索引
	*/
	int GetRandActor(int iTaskGroup)
	{
		CHECK_EXPR(iTaskGroup >= 0 && iTaskGroup < (int)m_stVecActorGroupPool.size(), -1, "taskGroup:{} error", iTaskGroup);
		if (m_stVecActorGroupPool[iTaskGroup].empty())
		{
			return -1;
		}

		m_ullSuitIndex++;
		m_ullSuitIndex = m_ullSuitIndex % m_stVecActorGroupPool[iTaskGroup].size();

		return m_stVecActorGroupPool[iTaskGroup][m_ullSuitIndex];
	}

	/**
	* @brief 通过平衡ID添加要异步处理的task
	*
	* @param iTaskGroup
	* @param pTask 要异步处理的task
	* @return
	*/
	int AddTask(int iTaskGroup, NFTask* pTask)
	{
		if (pTask)
		{
			int iActorId = GetBalanceActor(iTaskGroup, pTask->GetBalanceId());
			if (iActorId > 0)
			{
				return FindModule<NFITaskModule>()->AddTask(iTaskGroup, iActorId, pTask);
			}
		}

		return 0;
	}

protected:
	/**
	* @brief actor索引数组
	*/
	std::vector<std::vector<int> > m_stVecActorGroupPool;

	/**
	* @brief 用来平衡随机获得actor
	*/
	size_t m_ullSuitIndex = 0;

	bool m_bInitActor;
};
