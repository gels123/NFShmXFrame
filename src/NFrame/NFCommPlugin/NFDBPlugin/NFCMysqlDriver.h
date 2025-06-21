// -------------------------------------------------------------------------
//    @FileName         :    NFCMysqlDriver.h
//    @Author           :    eliteYang
//    @Date             :   2022-09-18
//    @Module           :    NFCMysqlDriver
//    @Desc             :
// -------------------------------------------------------------------------

#pragma once

//#define MS_HIREDIS
#ifdef _MSC_VER
#include <windows.h>
#include <stdint.h>

#else

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#endif

#include <string>
#include <vector>
#include "mysqlpp/mysql++.h"
#include "NFComm/NFKernelMessage/FrameSqlData.pb.h"
#include "NFComm/NFCore/NFQueue.hpp"
#include "NFComm/NFCore/NFMutex.h"
#include "mysqlpp/connection.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"

#define  NFMYSQLTRYBEGIN try {

#define  NFMYSQLTRYEND(msg) }\
    catch (mysqlpp::BadQuery er) \
    { \
        errorMsg = er.what();\
        NFLogError(NF_LOG_DEFAULT, 0, "BadQuery [{}] Error:{}", msg, er.what());\
        return -1; \
    } \
    catch (const mysqlpp::BadConversion& er)  \
    { \
        errorMsg = er.what();\
        NFLogError(NF_LOG_DEFAULT, 0, "BadConversion [{}] Error:{} retrieved data size:{}, actual size:{}", msg, er.what(), er.retrieved, er.actual_size);\
        return -1; \
    } \
    catch (const mysqlpp::Exception& er) \
    { \
        errorMsg = er.what();\
        NFLogError(NF_LOG_DEFAULT, 0, "mysqlpp::Exception [{}] Error:{}", msg, er.what());\
        return -1; \
    }\
    catch ( ... ) \
    { \
        errorMsg = "UnKnown Error";\
        NFLogError(NF_LOG_DEFAULT, 0, "std::exception [{}] Error:Error:Unknown", msg);\
        return -1; \
    }

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/**
* @brief mysql驱动， 里面有一个myql连接
** 实现了通过protobuf的反射来存取数据，使用方法如下：
*****************************保存数据到db方案*******************************/
class NFCMysqlDriver final
{
public:
    /**
     * @brief NFCMysqlDriver 构造函数，用于初始化 MySQL 驱动对象。
     *
     * 该构造函数允许指定重连时间和重连次数，用于控制数据库连接失败时的重连行为。
     *
     * @param nReconnectTime 重连时间间隔（单位：秒），默认为 3 秒。
     * @param nReconnectCount 重连次数，默认为 -1 表示无限重连。
     */
    explicit NFCMysqlDriver(int nReconnectTime = 3, int nReconnectCount = -1);

    /**
     * @brief NFCMysqlDriver 构造函数，用于初始化 MySQL 驱动对象。
     *
     * 该构造函数允许指定数据库名称、主机地址、端口、用户名和密码，用于直接连接到指定的数据库。
     *
     * @param strDbName 数据库名称。
     * @param strDbHost 数据库主机地址。
     * @param iDbPort 数据库端口号。
     * @param strDbUser 数据库用户名。
     * @param strDbPwd 数据库密码。
     */
    NFCMysqlDriver(const std::string& strDbName, const std::string& strDbHost, int iDbPort, const std::string& strDbUser, const std::string& strDbPwd);

    /**
     * @brief NFCMysqlDriver 析构函数。
     *
     * 该析构函数用于释放 MySQL 驱动对象所占用的资源，确保在对象销毁时正确关闭数据库连接。
     */
    ~NFCMysqlDriver();

    /**
     * @brief 连接到数据库
     *
     * 该函数用于建立与指定数据库的连接。
     *
     * @param strDbName 数据库名称
     * @param strDbHost 数据库主机地址
     * @param iDbPort 数据库端口号
     * @param strDbUser 数据库用户名
     * @param strDbPwd 数据库密码
     * @return int 返回连接结果，0表示成功，非0表示失败
     */
    int Connect(const std::string& strDbName, const std::string& strDbHost, int iDbPort, const std::string& strDbUser, const std::string& strDbPwd);

    /**
     * @brief 检查数据库连接状态
     *
     * 该函数用于检查当前数据库连接是否有效。
     *
     * @return int 返回连接状态，0表示连接正常，非0表示连接异常
     */
    int CheckConnect();

    /**
     * @brief 执行数据库查询
     *
     * 该函数用于执行SQL查询语句，并将结果存储在queryResult中。
     *
     * @param qstr SQL查询语句
     * @param queryResult 查询结果集
     * @param errorMsg 错误信息，如果执行失败，将返回错误描述
     * @return int 返回执行结果，0表示成功，非0表示失败
     */
    int Query(const std::string& qstr, mysqlpp::StoreQueryResult& queryResult, std::string& errorMsg);

    /**
     * @brief 执行单条SQL语句并返回结果
     *
     * 该函数用于执行单条SQL语句，并将结果存储在valueVec中。
     *
     * @param qstr SQL语句
     * @param valueVec 存储结果的键值对映射
     * @param errorMsg 错误信息，如果执行失败，将返回错误描述
     * @return int 返回执行结果，0表示成功，非0表示失败
     */
    int ExecuteOne(const std::string& qstr, std::map<std::string, std::string>& valueVec, std::string& errorMsg);

    /**
     * @brief 执行多条SQL语句并返回结果
     *
     * 该函数用于执行多条SQL语句，并将结果存储在valueVec中。
     *
     * @param qstr SQL语句
     * @param valueVec 存储结果的键值对映射列表
     * @param errorMsg 错误信息，如果执行失败，将返回错误描述
     * @return int 返回执行结果，0表示成功，非0表示失败
     */
    int ExecuteMore(const std::string& qstr, std::vector<std::map<std::string, std::string>>& valueVec, std::string& errorMsg);

    /**
     * @brief 执行存储过程或查询
     *
     * 该函数用于执行存储过程或查询，并将结果存储在select_res中。
     *
     * @param select 存储过程或查询的输入参数
     * @param selectRes 存储过程或查询的输出结果
     * @return int 返回执行结果，0表示成功，非0表示失败
     */
    int Execute(const NFrame::storesvr_execute& select, NFrame::storesvr_execute_res& selectRes);

    /**
     * 执行更多的存储服务器执行请求
     *
     * @param select 包含执行更多操作的请求信息
     * @param selectRes 将要填充的响应信息，包含执行结果
     * @return 返回执行操作的状态码
     *
     * 此函数用于处理多个执行请求，它接收一个包含请求信息的参数select，
     * 并将执行结果填充到selectRes中返回。通过这个函数，可以批量处理
     * 执行操作，提高效率和性能。
     */
    int ExecuteMore(const NFrame::storesvr_execute_more& select, NFrame::storesvr_execute_more_res& selectRes);

    /**
     * 执行更多的存储服务器操作
     *
     * @param select 包含执行更多操作的请求信息
     * @param vecSelectRes 将要填充的执行结果列表
     * @return 完成操作的返回状态码
     */
    int ExecuteMore(const NFrame::storesvr_execute_more& select, google::protobuf::RepeatedPtrField<NFrame::storesvr_execute_more_res>& vecSelectRes);

    /**
     * 根据条件选择数据
     *
     * @param select 包含选择条件的请求信息
     * @param selectRes 将要填充的选择结果
     * @return 完成操作的返回状态码
     */
    int SelectByCond(const NFrame::storesvr_sel& select, NFrame::storesvr_sel_res& selectRes);

    /**
     * 根据条件选择数据，返回多个结果
     *
     * @param select 包含选择条件的请求信息
     * @param selectRes 将要填充的选择结果列表
     * @return 完成操作的返回状态码
     */
    int SelectByCond(const NFrame::storesvr_sel& select, google::protobuf::RepeatedPtrField<NFrame::storesvr_sel_res>& selectRes);

    /**
     * 获取私有密钥
     *
     * @param packageName 请求的包名
     * @param className 请求的类名
     * @param privateKey 将要填充的私有密钥
     * @return 完成操作的返回状态码
     */
    int GetPrivateKey(const std::string& packageName, const std::string& className, std::string& privateKey);

    /**
     * 获取私有密钥和SQL查询结果
     *
     * @param select 包含选择条件的请求信息
     * @param privateKey 将要填充的私有密钥
     * @param selectRes 将要填充的查询结果
     * @return 完成操作的返回状态码
     */
    int GetPrivateKeySql(const NFrame::storesvr_sel& select, std::string& privateKey, std::string& selectRes);

    /**
     * @brief 根据条件查询数据并获取私有密钥
     * @param select 查询条件
     * @param privateKey 返回的私有密钥
     * @param fields 返回的字段集合
     * @param privateKeySet 返回的私有密钥集合
     * @return 执行结果
     */
    int SelectByCond(const NFrame::storesvr_sel& select, std::string& privateKey, std::unordered_set<std::string>& fields, std::unordered_set<std::string>& privateKeySet);

    /**
     * @brief 根据条件查询数据
     * @param packageName 包名
     * @param tableName 表名
     * @param className 类名
     * @param privateKey 私有密钥
     * @param leftPrivateKeySet 左连接私有密钥集合
     * @param recordsMap 返回的记录映射
     * @return 执行结果
     */
    int SelectByCond(const std::string& packageName, const std::string& tableName, const std::string& className, const std::string& privateKey, const std::unordered_set<std::string>& leftPrivateKeySet, std::map<std::string, std::string>& recordsMap);

    /**
     * @brief 创建SQL查询语句
     * @param tableName 表名
     * @param privateKey 私有密钥
     * @param leftPrivateKeySet 左连接私有密钥集合
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int CreateSql(const std::string& tableName, const std::string& privateKey, const std::unordered_set<std::string>& leftPrivateKeySet, std::string& selectRes);

    /**
     * @brief 根据查询条件创建SQL语句
     * @param select 查询条件
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_sel& select, std::string& selectRes);

    /**
     * @brief 查询对象数据
     * @param tbName 表名
     * @param pMessage 返回的protobuf消息
     * @param errMsg 错误信息
     * @return 执行结果
     */
    int SelectObj(const std::string& tbName, google::protobuf::Message* pMessage, std::string& errMsg);

    /**
     * @brief 根据条件查询对象数据
     * @param select 查询条件
     * @param selectRes 返回的查询结果
     * @return 执行结果
     */
    int SelectObj(const NFrame::storesvr_selobj& select, NFrame::storesvr_selobj_res& selectRes);

    /**
     * @brief 根据条件查询对象数据
     * @param packageName 包名
     * @param tbName 表名
     * @param className 类名
     * @param privateKey 私有密钥
     * @param privateKeyValue 私有密钥值
     * @param record 返回的记录
     * @return 执行结果
     */
    int SelectObj(const std::string& packageName, const std::string& tbName, const std::string& className, const std::string& privateKey, const std::string& privateKeyValue, std::string& record);

    /**
     * @brief 根据查询条件创建SQL语句
     * @param select 查询条件
     * @param keyMap 返回的键值映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_selobj& select, std::map<std::string, std::string>& keyMap);

    /**
     * @brief 根据条件删除数据
     * @param select 删除条件
     * @param selectRes 返回的删除结果
     * @return 执行结果
     */
    int DeleteByCond(const NFrame::storesvr_del& select, NFrame::storesvr_del_res& selectRes);

    /**
     * @brief 根据条件删除数据并获取私有密钥
     * @param select 删除条件
     * @param privateKey 返回的私有密钥
     * @param privateKeySet 返回的私有密钥集合
     * @return 执行结果
     */
    int DeleteByCond(const NFrame::storesvr_del& select, std::string& privateKey, std::unordered_set<std::string>& privateKeySet);

    /**
     * @brief 根据条件和私有密钥删除数据
     * @param select 删除条件
     * @param privateKey 私有密钥
     * @param privateKeySet 私有密钥集合
     * @param selectRes 返回的删除结果
     * @return 执行结果
     */
    int DeleteByCond(const NFrame::storesvr_del& select, const std::string& privateKey, const std::unordered_set<std::string>& privateKeySet, NFrame::storesvr_del_res& selectRes);

    /**
     * @brief 获取私有密钥并生成SQL删除语句
     * @param select 删除条件
     * @param privateKey 返回的私有密钥
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int GetPrivateKeySql(const NFrame::storesvr_del& select, std::string& privateKey, std::string& selectRes);

    /**
     * @brief 根据删除条件创建SQL语句
     * @param select 删除条件
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_del& select, std::string& selectRes);

    /**
     * @brief 根据删除条件和私有密钥创建SQL语句
     * @param select 删除条件
     * @param privateKey 私有密钥
     * @param privateKeySet 私有密钥集合
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_del& select, const std::string& privateKey, const std::unordered_set<std::string>& privateKeySet, std::string& selectRes);

    /**
     * @brief 根据修改条件创建SQL语句
     * @param select 修改条件
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_mod& select, std::string& selectRes);

    /**
     * @brief 根据更新条件创建SQL语句
     * @param select 更新条件
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_update& select, std::string& selectRes);

    /**
     * @brief 删除对象数据
     * @param select 删除条件
     * @param selectRes 返回的删除结果
     * @return 执行结果
     */
    int DeleteObj(const NFrame::storesvr_delobj& select, NFrame::storesvr_delobj_res& selectRes);

    /**
     * @brief 根据删除对象条件创建SQL语句
     * @param select 删除条件
     * @param keyMap 返回的键值映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_delobj& select, std::map<std::string, std::string>& keyMap);

    /**
     * @brief 插入对象数据
     * @param tbName 表名
     * @param pMessage 要插入的protobuf消息
     * @param errMsg 错误信息
     * @return 执行结果
     */
    int InsertObj(const std::string& tbName, const google::protobuf::Message* pMessage, std::string& errMsg);

    /**
     * @brief 根据条件插入对象数据
     * @param select 插入条件
     * @param selectRes 返回的插入结果
     * @return 执行结果
     */
    int InsertObj(const NFrame::storesvr_insertobj& select, NFrame::storesvr_insertobj_res& selectRes);

    /**
     * @brief 根据插入条件创建SQL语句
     * @param select 插入条件
     * @param resultMap 返回的结果映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_insertobj& select, std::map<std::string, std::string>& resultMap);

    /**
     * @brief 修改对象数据
     * @param tbName 表名
     * @param pMessage 要修改的protobuf消息
     * @param errMsg 错误信息
     * @return 执行结果
     */
    int ModifyObj(const std::string& tbName, const google::protobuf::Message* pMessage, std::string& errMsg);

    /**
     * @brief 根据条件修改数据
     * @param select 修改条件
     * @param selectRes 返回的修改结果
     * @return 执行结果
     */
    int ModifyByCond(const NFrame::storesvr_mod& select, NFrame::storesvr_mod_res& selectRes);

    /**
     * @brief 根据条件修改数据并获取私有密钥
     * @param select 修改条件
     * @param privateKey 返回的私有密钥
     * @param privateKeySet 返回的私有密钥集合
     * @return 执行结果
     */
    int ModifyByCond(const NFrame::storesvr_mod& select, std::string& privateKey, std::unordered_set<std::string>& privateKeySet);

    /**
     * @brief 获取私有密钥并生成SQL修改语句
     * @param select 修改条件
     * @param privateKey 返回的私有密钥
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int GetPrivateKeySql(const NFrame::storesvr_mod& select, std::string& privateKey, std::string& selectRes);

    /**
     * @brief 修改对象数据
     * @param select 修改条件
     * @param selectRes 返回的修改结果
     * @return 执行结果
     */
    int ModifyObj(const NFrame::storesvr_modobj& select, NFrame::storesvr_modobj_res& selectRes);

    /**
     * @brief 根据修改条件创建SQL语句
     * @param select 修改条件
     * @param keyMap 返回的键值映射
     * @param kevValueMap 返回的键值对映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_mod& select, std::map<std::string, std::string>& keyMap, std::map<std::string, std::string>& kevValueMap);

    /**
     * @brief 根据更新条件创建SQL语句
     * @param select 更新条件
     * @param keyMap 返回的键值映射
     * @param kevValueMap 返回的键值对映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_update& select, std::map<std::string, std::string>& keyMap, std::map<std::string, std::string>& kevValueMap);

    /**
     * @brief 根据修改对象条件创建SQL语句
     * @param select 修改对象条件
     * @param keyMap 返回的键值映射
     * @param kevValueMap 返回的键值对映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_modobj& select, std::map<std::string, std::string>& keyMap, std::map<std::string, std::string>& kevValueMap);

    /**
     * @brief 更新对象数据
     * @param tbName 表名
     * @param pMessage 要更新的protobuf消息
     * @param errMsg 错误信息
     * @return 执行结果
     */
    int UpdateObj(const std::string& tbName, const google::protobuf::Message* pMessage, std::string& errMsg);

    /**
     * @brief 根据条件更新数据
     * @param select 更新条件
     * @param selectRes 返回的更新结果
     * @return 执行结果
     */
    int UpdateByCond(const NFrame::storesvr_update& select, NFrame::storesvr_update_res& selectRes);

    /**
     * @brief 根据条件更新数据并获取私有密钥
     * @param select 更新条件
     * @param privateKey 返回的私有密钥
     * @param privateKeySet 返回的私有密钥集合
     * @return 执行结果
     */
    int UpdateByCond(const NFrame::storesvr_update& select, std::string& privateKey, std::unordered_set<std::string>& privateKeySet);

    /**
     * @brief 获取私有密钥并生成SQL更新语句
     * @param select 更新条件
     * @param privateKey 返回的私有密钥
     * @param selectRes 返回的SQL语句
     * @return 执行结果
     */
    int GetPrivateKeySql(const NFrame::storesvr_update& select, std::string& privateKey, std::string& selectRes);

    /**
     * @brief 更新对象数据
     * @param select 更新条件
     * @param selectRes 返回的更新结果
     * @return 执行结果
     */
    int UpdateObj(const NFrame::storesvr_updateobj& select, NFrame::storesvr_updateobj_res& selectRes);

    /**
     * @brief 根据更新对象条件创建SQL语句
     * @param select 更新对象条件
     * @param keyMap 返回的键值映射
     * @param kevValueMap 返回的键值对映射
     * @return 执行结果
     */
    int CreateSql(const NFrame::storesvr_updateobj& select, std::map<std::string, std::string>& keyMap, std::map<std::string, std::string>& kevValueMap);

    /**
     * @brief 查询描述存储数据
     * @param table 表名
     * @param pMessage 返回的protobuf消息指针
     * @return 执行结果
     */
    int QueryDescStore(const std::string& table, google::protobuf::Message** pMessage);

    /**
     * @brief 查询描述存储数据
     * @param table 表名
     * @param pMessage 返回的protobuf消息
     * @return 执行结果
     */
    int QueryDescStore(const std::string& table, google::protobuf::Message* pMessage);

    /**
     * @brief 将数据库表行数据转换为protobuf消息
     * @param result 数据库表行数据
     * @param packageName 包名
     * @param className 类名
     * @param pMessage 返回的protobuf消息指针
     * @return 执行结果
     */
    int TransTableRowToMessage(const std::map<std::string, std::string>& result, const std::string& packageName, const std::string& className, google::protobuf::Message** pMessage) const;

    /**
     * @brief 获取MySQL连接对象
     * @return 返回MySQL连接对象指针
     */
    mysqlpp::Connection* GetConnection();

    /**
     * @brief 关闭数据库连接
     */
    void CloseConnection();

    /**
     * @brief 检查数据库连接是否可用
     * @return 返回连接状态，true表示可用，false表示不可用
     */
    bool Enable();

    /**
     * @brief 检查是否可以重连
     * @return 返回是否可以重连，true表示可以，false表示不可以
     */
    bool CanReconnect();

    /**
     * @brief 重新连接数据库
     * @return 返回连接结果，0表示成功，非0表示失败
     */
    int Reconnect();

    /**
     * @brief 更新数据库表数据
     * @param strTableName 表名
     * @param keyMap 主键映射
     * @param keyValueMap 要更新的键值对映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Update(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, const std::map<std::string, std::string>& keyValueMap, std::string& errorMsg);

    /**
     * @brief 根据条件修改数据库表数据
     * @param strTableName 表名
     * @param where 条件语句
     * @param keyValueMap 要修改的键值对映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Modify(const std::string& strTableName, const std::string& where, const std::map<std::string, std::string>& keyValueMap, std::string& errorMsg);

    /**
     * @brief 根据主键修改数据库表数据
     * @param strTableName 表名
     * @param keyMap 主键映射
     * @param keyValueMap 要修改的键值对映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Modify(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, const std::map<std::string, std::string>& keyValueMap, std::string& errorMsg);

    /**
     * @brief 插入数据到数据库表
     * @param strTableName 表名
     * @param keyValueMap 要插入的键值对映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Insert(const std::string& strTableName, const std::map<std::string, std::string>& keyValueMap, std::string& errorMsg);

    /**
     * @brief 查询单条数据（指定字段）
     * @param strTableName 表名
     * @param keyMap 查询条件映射
     * @param fieldVec 要查询的字段列表
     * @param valueVec 返回的查询结果映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int QueryOne(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, const std::vector<std::string>& fieldVec, std::map<std::string, std::string>& valueVec, std::string& errorMsg);

    /**
     * @brief 查询单条数据（所有字段）
     * @param strTableName 表名
     * @param keyMap 查询条件映射
     * @param valueVec 返回的查询结果映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int QueryOne(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, std::map<std::string, std::string>& valueVec, std::string& errorMsg);

    /**
     * @brief 查询多条数据
     * @param strTableName 表名
     * @param keyMap 查询条件映射
     * @param fieldVec 要查询的字段列表
     * @param valueVec 返回的查询结果列表
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int QueryMore(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, const std::vector<std::string>& fieldVec, std::vector<std::map<std::string, std::string>>& valueVec, std::string& errorMsg);

    /**
     * @brief 根据主键删除数据
     * @param strTableName 表名
     * @param strKeyColName 主键列名
     * @param strKey 主键值
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Delete(const std::string& strTableName, const std::string& strKeyColName, const std::string& strKey, std::string& errorMsg);

    /**
     * @brief 根据条件删除数据
     * @param strTableName 表名
     * @param keyMap 删除条件映射
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Delete(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, std::string& errorMsg);

    /**
     * @brief 执行SQL删除语句
     * @param sql SQL删除语句
     * @param errorMsg 错误信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Delete(const std::string& sql, std::string& errorMsg);

    /**
     * @brief 检查表中是否存在指定主键的记录
     * @param strTableName 表名
     * @param strKeyColName 主键列名
     * @param strKey 主键值
     * @param bExit 返回是否存在
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Exists(const std::string& strTableName, const std::string& strKeyColName, const std::string& strKey, bool& bExit);

    /**
     * @brief 检查表中是否存在符合指定条件的记录
     * @param strTableName 表名
     * @param keyMap 条件映射
     * @param bExit 返回是否存在
     * @return 执行结果，0表示成功，非0表示失败
     */
    int Exists(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, bool& bExit);

    /**
     * @brief 检查数据库是否存在
     * @param dbName 数据库名称
     * @param bExit 返回是否存在
     * @return 执行结果，0表示成功，非0表示失败
     */
    int ExistsDb(const std::string& dbName, bool& bExit);

    /**
     * @brief 创建数据库
     * @param dbName 数据库名称
     * @return 执行结果，0表示成功，非0表示失败
     */
    int CreateDb(const std::string& dbName);

    /**
     * @brief 选择数据库
     * @param dbName 数据库名称
     * @return 执行结果，0表示成功，非0表示失败
     */
    int SelectDb(const std::string& dbName);

    /**
     * @brief 检查数据库中是否存在指定表
     * @param dbName 数据库名称
     * @param tableName 表名
     * @param bExit 返回是否存在
     * @return 执行结果，0表示成功，非0表示失败
     */
    int ExistTable(const std::string& dbName, const std::string& tableName, bool& bExit);

    /**
     * @brief 获取表的列信息
     * @param dbName 数据库名称
     * @param tableName 表名
     * @param col 返回的列信息映射
     * @return 执行结果，0表示成功，非0表示失败
     */
    int GetTableColInfo(const std::string& dbName, const std::string& tableName, std::map<std::string, DBTableColInfo>& col);

    /**
     * @brief 查询表信息
     * @param dbName 数据库名称
     * @param tableName 表名
     * @param bExit 返回表是否存在
     * @param primaryKey 返回的主键信息
     * @param needCreateColumn 返回需要创建的列信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int QueryTableInfo(const std::string& dbName, const std::string& tableName, bool& bExit, std::map<std::string, DBTableColInfo>& primaryKey, std::multimap<uint32_t, std::string>& needCreateColumn);

    /**
     * @brief 创建表
     * @param tableName 表名
     * @param primaryKey 主键信息
     * @param needCreateColumn 需要创建的列信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int CreateTable(const std::string& tableName, const std::map<std::string, DBTableColInfo>& primaryKey, const std::multimap<uint32_t, std::string>& needCreateColumn);

    /**
     * @brief 添加表行
     * @param tableName 表名
     * @param needCreateColumn 需要创建的列信息
     * @return 执行结果，0表示成功，非0表示失败
     */
    int AddTableRow(const std::string& tableName, const std::multimap<uint32_t, std::string>& needCreateColumn);

public:
    /**
     * @brief 检查是否需要重连数据库
     * @return 返回是否需要重连，true表示需要，false表示不需要
     */
    bool IsNeedReconnect();

    /**
     * @brief 连接到数据库
     * @return 返回连接结果，0表示成功，非0表示失败
     */
    int Connect();

    /**
     * @brief 断开数据库连接
     * @return 返回断开结果，0表示成功，非0表示失败
     */
    int Disconnect();

private:
    std::string m_strDbName; // 数据库名称
    std::string m_strDbHost; // 数据库主机地址
    int m_iDbPort; // 数据库端口号
    std::string m_strDbUser; // 数据库用户名
    std::string m_strDbPwd; // 数据库密码

    mysqlpp::Connection* m_pMysqlConnect; // MySQL连接对象指针
    float m_fCheckReconnect; // 检查重连的时间间隔（单位：秒）

    int m_iReconnectTime; // 重连时间间隔（单位：秒）
    int m_iReconnectCount; // 重连次数计数器

    static NFMutex m_stConnectLock; // 连接锁，用于防止多线程调用Connect时崩溃
};
