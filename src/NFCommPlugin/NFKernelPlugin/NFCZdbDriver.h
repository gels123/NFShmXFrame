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
#include <unordered_map>
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "zdb/zdb.h"

#define  NF_ZDB_TRY_BEGIN  TRY {


#define NF_ZDB_TRY_END(outCode, outError, msg) }\
        CATCH(SQLException)\
        {\
            outCode = -1;\
            outError = NF_FORMAT("{} {}: {} raised in {} at {}:{}", msg,\
            Exception_frame.exception->name,\
            Exception_frame.message,\
            Exception_frame.func,\
            Exception_frame.file,\
            Exception_frame.line);\
            NFLogError(NF_LOG_SYSTEMLOG, 0, "{}", outError); \
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
public:
    /**
     * @brief ִ��sql���
     *
     * @param  qstr sql���
     * @param  valueVec ��������
     * @return int =0ִ�гɹ�, != 0ʧ��
     */
    int ExecuteOne(const std::string &qstr, std::unordered_map<std::string, std::string> &valueVec, std::string &errormsg);

    /**
     * @brief ִ��sql���
     *
     * @param  qstr sql���
     * @return int =0ִ�гɹ�, != 0ʧ��
     */
    int Execute(const std::string &qstr, std::string &errormsg);
private:
    /**
     * @brief ͨ��sql����ѯ����
     *
     * @param  qstr sql���
     * @param  queryResult ��ѯ���
     * @return bool ��ѯ�ɹ���ʧ��
     */
    int Query(const std::string &qstr, ResultSet_T& resultSet, std::string &errormsg);
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


