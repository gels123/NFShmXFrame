// -------------------------------------------------------------------------
//    @FileName         :    NFCZdbDriver.h
//    @Author           :    eliteYang
//    @Date             :   2022-11-09
//    @Module           :    NFCZdbDriver
//    @Desc             :
// -------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <set>
#include "NFComm/NFKernelMessage/storesvr_sqldata.pb.h"
#include <string>
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "zdb/zdb.h"

#define  NF_ZDB_TRY_BEGIN  TRY


#define NF_ZDB_TRY_END(msg)\
        ELSE\
        {\
            NFLogError(NF_LOG_SYSTEMLOG, 0, "{} {}: {} raised in {} at {}:{}", msg,\
            Exception_frame.exception->name,\
            Exception_frame.message,\
            Exception_frame.func,\
            Exception_frame.file,\
            Exception_frame.line);\
        }\
        END_TRY;\


class NFCZdbDriver
{
public:
    NFCZdbDriver();
    virtual ~NFCZdbDriver();
public:
    /**
     * @brief �������ݿ�
     *
     * @param  strDBType ���ݿ����ͣ�����mysql,sqlite3
     * @param  strDBName ���ݿ�����
     * @param  strDBHost ���ݿ�IP
     * @param  nDBPort   ���ݿ�˿�
     * @param  strDBUser ���ݿ��û���
     * @param  strDBPwd  ���ݿ�����
     * @return int
     */
    virtual int Connect(const std::string& strDBType, const std::string &strDBName, const std::string &strDBHost, int nDBPort, const std::string &strDBUser,
                const std::string &strDBPwd);

    /**
     * @brief ����
     *
     * @return int
     */
    virtual int Connect();

    /**
     * @brief ���zdb������
     *
     * @return Connection_T
     */
    Connection_T GetConnection();
private:
    /**
     * @brief ���ݿ����ͣ�����mysql,sqlite3
     */
    std::string m_dbType;

    /**
     * @brief ���ݿ�����
     */
    std::string m_dbName;

    /**
     * @brief ���ݿ�IP
     */
    std::string m_dbHost;

    /**
     * @brief ���ݿ�˿�
     */
    int m_dbPort;

    /**
     * @brief ���ݿ��û���
     */
    std::string m_dbUser;

    /**
     * @brief ���ݿ��û�����
     */
    std::string m_dbPwd;

    /**
     * @brief url����
     */
    URL_T m_url;

    /**
     * @brief zdb���ӳ�
     */
    ConnectionPool_T m_connectionpool;
};


