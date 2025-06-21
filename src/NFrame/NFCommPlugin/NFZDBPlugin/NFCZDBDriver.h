// -------------------------------------------------------------------------
//    @FileName         :    NFCZDBDriver.h
//    @Author           :    gaoyi
//    @Date             :   2024-05-23
//    @Module           :    NFCZDBDriver
//    @Desc             :
// -------------------------------------------------------------------------

#pragma once

//#define MS_HIREDIS
#ifdef _MSC_VER
#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdint.h>

#else

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
//#include <sys/timeb.h>
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
#include "NFComm/NFKernelMessage/FrameSqlData.pb.h"
#include <string>
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"

#include "zdb.h"
#include "SQLException.h"

#define  NFMYSQLTRYBEGIN TRY {

#define  NFMYSQLTRYEND(msg) }\
         CATCH(SQLException) \
         {\
            errormsg = "UnKnown Error";\
            NFLogError(NF_LOG_DEFAULT, 0, "std::exception [{}] Error:Error:Unknown", msg);\
            return -1; \
         }\
         END_TRY;\

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/**
* @brief libzdb， 里面有一个zdb连接
** 实现了通过protobuf的反射来存取数据，使用方法如下：
*****************************保存数据到db方案*******************************/



class NFCZDBDriver
{
public:
    /**
     * @brief 构造函数
     *
     * @param  nReconnectTime 重连时间
     * @param  nReconnectCount 重连次数，-1表示无数次
     * @return
     */
    NFCZDBDriver(int nReconnectTime = 3, int nReconnectCount = -1);

    /**
     * @brief 构造函数
     *
     * @param  strDBName 数据库名字
     * @param  strDBHost 数据库IP
     * @param  nDBPort   数据库端口
     * @param  strDBUser 数据库用户
     * @param  strDBPwd  数据库密码
     * @return
     */
    NFCZDBDriver(uint32_t dbType, const std::string &strDBName, const std::string &strDBHost, int nDBPort,
                   const std::string &strDBUser, const std::string &strDBPwd);

    /**
     * @brief 析构函数
     */
    virtual ~NFCZDBDriver();


    /**
     * @brief 连接数据库
     *
     * @param  strDBName 数据库名字
     * @param  strDBHost 数据库IP
     * @param  nDBPort   数据库端口
     * @param  strDBUser 数据库用户名
     * @param  strDBPwd  数据库密码
     * @return bool
     */
    int Connect(uint32_t dbType, const std::string &strDBName, const std::string &strDBHost, int nDBPort, const std::string &strDBUser,
                const std::string &strDBPwd);

    /**
     * @brief 循环执行
     *
     * @return bool
     */
    int CheckConnect();

    /**
     * @brief 通过sql语句查询数据
     *
     * @param  qstr sql语句
     * @param  queryResult 查询结果
     * @return bool 查询成功或失败
     */
    int Query(const std::string &qstr, std::vector<std::map<std::string, std::string>> &queryResult, std::string &errormsg);

    /**
     * @brief 执行sql语句
     *
     * @param  qstr sql语句
     * @param  valueVec 返回数据
     * @return int =0执行成功, != 0失败
     */
    int ExecuteOne(const std::string &qstr, std::map<std::string, std::string> &valueVec, std::string &errormsg);

    /**
     * @brief 执行sql语句
     *
     * @param  qstr sql语句
     * @param  valueVec 返回数据
     * @return int =0执行成功, != 0失败
     */
    int ExecuteMore(const std::string &qstr, std::vector<std::map<std::string, std::string>> &valueVec,
                    std::string &errormsg);

    /**
     * @brief 执行sql语句
     *
     * @param  qstr sql语句
     * @param  valueVec 返回数据
     * @return int =0执行成功, != 0失败
     */
    int Execute(const NFrame::storesvr_execute &select, NFrame::storesvr_execute_res &select_res);

    /**
     * @brief 执行sql语句
     *
     * @param  qstr sql语句
     * @param  valueVec 返回数据
     * @return int =0执行成功, != 0失败
     */
    int ExecuteMore(const NFrame::storesvr_execute_more &select, NFrame::storesvr_execute_more_res &select_res);

    /**
     * @brief 执行sql语句
     *
     * @param  qstr sql语句
     * @param  valueVec 返回数据
     * @return int =0执行成功, != 0失败
     */
    int ExecuteMore(const NFrame::storesvr_execute_more &select,
                    ::google::protobuf::RepeatedPtrField<NFrame::storesvr_execute_more_res> &vecSelectRes);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int SelectByCond(const NFrame::storesvr_sel &select, NFrame::storesvr_sel_res &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int
    SelectByCond(const NFrame::storesvr_sel &select, ::google::protobuf::RepeatedPtrField<NFrame::storesvr_sel_res> &select_res);

    /**
     * @brief 获得表的唯一key
     * @param tbName
     * @param privateKey
     * @return
     */
    int GetPrivateKey(const std::string packageName, const std::string &className, std::string &privateKey);

    /**
     * @brief 获得表的唯一key's sql
     * @param select
     * @param select_res
     * @return
     */
    int GetPrivateKeySql(const NFrame::storesvr_sel &select, std::string &privateKey, std::string &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取获取到private key, 以及所有被选出数据的private key集合
     * @param select
     * @param privateKey
     * @param privateKeySet
     * @return
     */
    int SelectByCond(const NFrame::storesvr_sel &select, std::string &privateKey, std::unordered_set<std::string> &fields, std::unordered_set<std::string> &privateKeySet);

    /**
     * @brief 通过select结构体， 从数据库获取获取到private key, 以及所有被选出数据的private key集合
     * @param select
     * @param privateKey
     * @param privateKeySet
     * @return
     */
    int SelectByCond(const std::string& packageName, const std::string& tableName, const std::string& className, const std::string &privateKey,
                     const std::unordered_set<std::string> &leftPrivateKeySet, std::map<std::string, std::string>& recordsMap);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const std::string& tableName, const std::string &privateKey,
                  const std::unordered_set<std::string> &leftPrivateKeySet, std::string &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_sel &select, std::string &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int SelectObj(const std::string &tbName, google::protobuf::Message *pMessage, std::string &errMsg);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int SelectObj(const NFrame::storesvr_selobj &select, NFrame::storesvr_selobj_res &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int SelectObj(const std::string& packageName, const std::string& tbName, const std::string& className, const std::string& privateKey, const std::string& privateKeyValue, std::string& record);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_selobj &select, std::map<std::string, std::string> &keyMap);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int DeleteByCond(const NFrame::storesvr_del &select, NFrame::storesvr_del_res &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int DeleteByCond(const NFrame::storesvr_del &select, std::string &privateKey, std::unordered_set<std::string> &privateKeySet);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int
    DeleteByCond(const NFrame::storesvr_del &select, const std::string &privateKey, const std::unordered_set<std::string> &privateKeySet,
                 NFrame::storesvr_del_res &select_res);

    /**
     * @brief 获得表的唯一key's sql
     * @param select
     * @param select_res
     * @return
     */
    int GetPrivateKeySql(const NFrame::storesvr_del &select, std::string &privateKey, std::string &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_del &select, std::string &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_del &select, const std::string &privateKey, const std::unordered_set<std::string> &privateKeySet,
                  std::string &select_res);

    int CreateSql(const NFrame::storesvr_mod &select, std::string &select_res);

    int CreateSql(const NFrame::storesvr_update &select, std::string &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int DeleteObj(const NFrame::storesvr_delobj &select, NFrame::storesvr_delobj_res &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_delobj &select, std::map<std::string, std::string> &keyMap);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int InsertObj(const std::string &tbName, const google::protobuf::Message *pMessage, std::string &errMsg);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int InsertObj(const NFrame::storesvr_insertobj &select, NFrame::storesvr_insertobj_res &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_insertobj &select, std::map<std::string, std::string> &resultMap);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int ModifyObj(const std::string &tbName, const google::protobuf::Message *pMessage, std::string &errMsg);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int ModifyByCond(const NFrame::storesvr_mod &select, NFrame::storesvr_mod_res &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int ModifyByCond(const NFrame::storesvr_mod &select, std::string &privateKey, std::unordered_set<std::string> &privateKeySet);

    /**
     * @brief 获得表的唯一key's sql
     * @param select
     * @param select_res
     * @return
     */
    int GetPrivateKeySql(const NFrame::storesvr_mod &select, std::string &privateKey, std::string &select_res);

    int ModifyObj(const NFrame::storesvr_modobj &select, NFrame::storesvr_modobj_res &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_mod &select, std::map<std::string, std::string> &keyMap,
                  std::map<std::string, std::string> &kevValueMap);

    int CreateSql(const NFrame::storesvr_update &select, std::map<std::string, std::string> &keyMap,
                  std::map<std::string, std::string> &kevValueMap);

    int CreateSql(const NFrame::storesvr_modobj &select, std::map<std::string, std::string> &keyMap,
                  std::map<std::string, std::string> &kevValueMap);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int UpdateObj(const std::string &tbName, const google::protobuf::Message *pMessage, std::string &errMsg);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int UpdateByCond(const NFrame::storesvr_update &select, NFrame::storesvr_update_res &select_res);

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    int UpdateByCond(const NFrame::storesvr_update &select, std::string &privateKey, std::unordered_set<std::string> &privateKeySet);

    /**
     * @brief 获得表的唯一key's sql
     * @param select
     * @param select_res
     * @return
     */
    int GetPrivateKeySql(const NFrame::storesvr_update &select, std::string &privateKey, std::string &select_res);

    int UpdateObj(const NFrame::storesvr_updateobj &select, NFrame::storesvr_updateobj_res &select_res);

    /**
     * @brief 通过select结构体，生成sql语句
     *
     * @param  select 查询语句
     * @param  selectSql 生成sql语句
     * @return int =0执行成功, != 0失败
     */
    int CreateSql(const NFrame::storesvr_updateobj &select, std::map<std::string, std::string> &keyMap,
                  std::map<std::string, std::string> &kevValueMap);

    /**
     * @brief 执行sql语句, 把数据库配置表里的数据取出来
     *
     * @param  table 配置表表明
     * @param  sheet_fullname protobuf中代表一个表格的message
     * @param  pMessage sheet_fullname的protobuf的数据结构，携带返回数据
     *  比如 message Sheet_GameRoomDesc
     *		{
     *			repeated GameRoomDesc GameRoomDesc_List = 1  [(nanopb).max_count=100];
     *		}
     * 代表一个Excel表格GameRoomDesc, 同时数据库有一个表GameRoomDesc
     * 都用这个数据结构来表达，以及存取数据
     *
     *
     * @return bool 执行成功或失败
     */
    int QueryDescStore(const std::string &table, google::protobuf::Message **pMessage);


    /**
     * @brief 执行sql语句, 把数据库配置表里的数据取出来
     *
     * @param  table 配置表表明
     * @param  sheet_fullname protobuf中代表一个表格的message
     * @param  pMessage sheet_fullname的protobuf的数据结构，携带返回数据
     *  比如 message Sheet_GameRoomDesc
     *		{
     *			repeated GameRoomDesc GameRoomDesc_List = 1  [(nanopb).max_count=100];
     *		}
     * 代表一个Excel表格GameRoomDesc, 同时数据库有一个表GameRoomDesc
     * 都用这个数据结构来表达，以及存取数据
     *
     *
     * @return bool 执行成功或失败
     */
    int QueryDescStore(const std::string &table, google::protobuf::Message *pMessage);

    /**
     * @brief 将result数据库中的一列数据转化存在message中
     *
     * @param  result数据库中的一列数据
     * table  表名也是message在protobuf里的名字
     * @param  pMessage 转化后的mesage
     * @return int =0执行成功, != 0失败
     */
    int TransTableRowToMessage(const std::map<std::string, std::string> &result, const std::string &packageName, const std::string &className,
                               google::protobuf::Message **pMessage);

    /**
     * @brief 获得连接
     *
     * @return ConnectionPool_T
     */
    Connection_T GetConnection();

    /**
     * @brief 关闭连接
     *
     * @return void
     */
    virtual void CloseConnection(Connection_T pConn);

    /**
     * @brief 是否连接有效
     *
     * @return bool
     */
    bool Enable();

    /**
     * @brief 是否可以重连
     *
     * @return bool
     */
    bool CanReconnect();

    /**
     * @brief 重连
     *
     * @return bool
     */
    int Reconnect();

    /**
     * @brief mysql语句更新和插入
     *
     * @param  strTableName		表名
     * @param  strKeyColName	key所在列的列名
     * @param  strKey			key数据
     * @param  keyvalueMap		数据
     * @return bool				成功或失败
     */
    int Update(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
               const std::map<std::string, std::string> &keyvalueMap, std::string &errormsg);


    /**
     * @brief mysql语句更新和插入
     *
     * @param  strTableName		表名
     * @param  strKeyColName	key所在列的列名
     * @param  strKey			key数据
     * @param  keyvalueMap		数据
     * @return bool				成功或失败
     */
    int Modify(const std::string &strTableName, const std::string &where,
               const std::map<std::string, std::string> &keyvalueMap, std::string &errormsg);

    int Modify(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
               const std::map<std::string, std::string> &keyvalueMap, std::string &errormsg);

    /**
     * @brief mysql语句更新和插入
     *
     * @param  strTableName		表名
     * @param  keyvalueMap		数据
     * @return bool				成功或失败
     */
    int Insert(const std::string &strTableName, const std::map<std::string, std::string> &keyvalueMap,
               std::string &errormsg);

    /**
     * @brief 查询数据一条数据
     *
     * @param  strTableName		表名
     * @param  strKeyColName	key所在列的列名
     * @param  strKey			key数据
     * @param  fieldVec			要取出的数据的列名
     * @param  valueVec			要取出的数据
     * @return bool				成功或失败
     */
    int QueryOne(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                 const std::vector<std::string> &fieldVec, std::map<std::string, std::string> &valueVec,
                 std::string &errormsg);


    /**
     * @brief 查询数据一条数据
     *
     * @param  strTableName		表名
     * @param  strKeyColName	key所在列的列名
     * @param  strKey			key数据
     * @param  fieldVec			要取出的数据的列名
     * @param  valueVec			要取出的数据
     * @return bool				成功或失败
     */
    int QueryOne(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                 std::map<std::string, std::string> &valueVec,
                 std::string &errormsg);

    /**
     * @brief 查询数据许多数据
     *
     * @param  strTableName		表名
     * @param  strKeyColName	key所在列的列名
     * @param  strKey			key数据
     * @param  fieldVec			要取出的数据的列名
     * @param  valueVec			要取出的数据
     * @return bool				成功或失败
     */
    int QueryMore(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                  const std::vector<std::string> &fieldVec,
                  std::vector<std::map<std::string, std::string>> &valueVec, std::string &errormsg);

    /**
     * @brief 删一行数据
     *
     * @param  strTableName
     * @param  strKeyColName
     * @param  strKey
     * @return bool
     */
    int Delete(const std::string &strTableName, const std::string &strKeyColName, const std::string &strKey, std::string &errormsg);

    int Delete(const std::string &strTableName, const std::map<std::string, std::string> &keyMap, std::string &errormsg);

    int Delete(const std::string &sql, std::string &errormsg);

    /**
     * @brief 判断数据是否存在
     *
     * @param  strTableName
     * @param  strKeyColName
     * @param  strKey
     * @param  bExit
     * @return bool
     */
    int Exists(const std::string &strTableName, const std::string &strKeyColName, const std::string &strKey,
               bool &bExit);

    /**
     * @brief 判断数据是否存在
     *
     * @param  strTableName
     * @param  strKeyColName
     * @param  strKey
     * @param  bExit
     * @return bool
     */
    int Exists(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
               bool &bExit);

    /**
     * @brief 是否存在数据库
     * @param dbName
     * @return
     */
    int ExistsDB(const std::string &dbName, bool &bExit);

    /**
     * @brief 创建数据库
     * @param dbName
     * @return
     */
    int CreateDB(const std::string &dbName);

    /**
     * @brief 选择数据库
     * @param dbName
     * @return
     */
    int SelectDB(const std::string &dbName);

    /**
     * @brief 是否存在表格
     * @param dbName
     * @param tableName
     * @param bExit
     * @return
     */
    int ExistTable(const std::string &dbName, const std::string &tableName, bool &bExit);

    /**
     * @brief 获取表列信息
     * @param dbName
     * @param tableName
     * @param col
     * @return
     */
    int GetTableColInfo(const std::string &dbName, const std::string &tableName, std::map<std::string, DBTableColInfo> &col);

    /**
     * @brief 查询表格信息
     * @param tableName
     * @param pTableMessage
     * @param needCreateColumn
     * @return
     */
    int QueryTableInfo(const std::string &dbName, const std::string &tableName, bool &bExit, std::map<std::string, DBTableColInfo> &primaryKey,
                       std::multimap<uint32_t, std::string> &needCreateColumn);

    /**
     * @brief 创建table
     * @param serverID
     * @param dbName
     * @param tableName
     * @param needCreateColumn
     * @return
     */
    int CreateTable(const std::string &tableName, const std::map<std::string, DBTableColInfo> &primaryKey,
                    const std::multimap<uint32_t, std::string> &needCreateColumn);

    /**
     * @brief 比较老的表列，看是否需要增加新的列
     * @param tableName
     * @param needCreateColumn
     * @return
     */
    int AddTableRow(const std::string &tableName, const std::multimap<uint32_t, std::string> &needCreateColumn);

public:
    /**
     * @brief 是否需要重连
     *
     * @return bool
     */
    bool IsNeedReconnect();

    /**
     * @brief 连接
     *
     * @return bool
     */
    int Connect();

    /**
     * @brief 不在链接
     */
    int Disconnect();

private:
    std::string mstrDBName;
    std::string mstrDBHost;
    int mnDBPort;
    std::string mstrDBUser;
    std::string mstrDBPwd;

    uint32_t m_dbType;

    URL_T m_pURL;
    ConnectionPool_T m_pConnectionPool;
    Connection_T m_pCurConnection;
    float mfCheckReconnect; //检查重连的时间

    int mnReconnectTime;
    int mnReconnectCount;
};
