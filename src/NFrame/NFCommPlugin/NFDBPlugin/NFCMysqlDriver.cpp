#include "NFCMysqlDriver.h"
#include "NFComm/NFCore/NFCommon.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"
#include "NFComm/NFPluginModule/NFCheck.h"

//m_pMysqlConnect在调用Connect会引发多线程的崩溃，必须枷锁
NFMutex NFCMysqlDriver::m_stConnectLock;

NFCMysqlDriver::NFCMysqlDriver(const int nReconnectTime/* = 60*/, const int nReconnectCount /*= -1*/)
{
    m_fCheckReconnect = 0.0f;
    m_iDbPort = 0;
    m_pMysqlConnect = nullptr;
    m_iReconnectTime = nReconnectTime;
    m_iReconnectCount = nReconnectCount;
}

NFCMysqlDriver::NFCMysqlDriver(const std::string& strDbName, const std::string& strDbHost, const int iDbPort, const std::string& strDbUser, const std::string& strDbPwd)
{
    m_fCheckReconnect = 0.0f;
    m_iDbPort = 0;
    m_pMysqlConnect = nullptr;

    m_iReconnectTime = 3;
    m_iReconnectCount = -1;

    Connect(strDbName, strDbHost, iDbPort, strDbUser, strDbPwd);
}

NFCMysqlDriver::~NFCMysqlDriver()
{
    CloseConnection();
}

int NFCMysqlDriver::Connect(const std::string& strDbName, const std::string& strDbHost, const int iDbPort,
                            const std::string& strDbUser, const std::string& strDbPwd)
{
    m_strDbName = strDbName;
    m_strDbHost = strDbHost;
    m_iDbPort = iDbPort;
    m_strDbUser = strDbUser;
    m_strDbPwd = strDbPwd;

    return Connect();
}

int NFCMysqlDriver::CheckConnect()
{
    if (IsNeedReconnect() && CanReconnect())
    {
        Connect(m_strDbName, m_strDbHost, m_iDbPort, m_strDbUser, m_strDbPwd);
    }

    return 0;
}

int NFCMysqlDriver::Query(const std::string& qstr, mysqlpp::StoreQueryResult& queryResult, std::string& errorMsg)
{
    mysqlpp::Connection* pConnection = GetConnection();
    if (pConnection)
    {
        //NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", qstr);

        NFMYSQLTRYBEGIN
            mysqlpp::Query query = pConnection->query(qstr);
            //query.execute();
            //NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());
            queryResult = query.store();

            query.reset();
        NFMYSQLTRYEND("Query Error")
        return 0;
    }

    return -1;
}

int NFCMysqlDriver::ExecuteOne(const std::string& qstr, std::map<std::string, std::string>& valueVec,
                               std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "ExecuteOne:{}", qstr);

    mysqlpp::StoreQueryResult queryResult;
    if (NFCMysqlDriver::Query(qstr, queryResult, errorMsg) == 0)
    {
        for (size_t i = 0; i < queryResult.num_rows(); ++i)
        {
            for (size_t j = 0; j < queryResult[i].size(); j++)
            {
                std::string value;
                if (!queryResult[i][j].is_null())
                {
                    queryResult[i][j].to_string(value);
                }
                valueVec.emplace(queryResult.field_name(j), value);
            }
            return 0; //-V612
        }
        return 0;
    }
    return -1;
}

int NFCMysqlDriver::Execute(const NFrame::storesvr_execute& select, NFrame::storesvr_execute_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", select.record());

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errorMsg;
    int iRet = ExecuteMore(select.record(), resultVec, errorMsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errorMsg);
        return -1;
    }

    selectRes.mutable_baseinfo()->CopyFrom(select.baseinfo());
    selectRes.mutable_opres()->set_mod_key(select.mod_key());

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];

        google::protobuf::Message* pMessage = nullptr;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != nullptr)
        {
            selectRes.set_record(pMessage->SerializePartialAsString());
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} sql:{}",
                       NFCommon::tostr(result), select.record());
            iRet = -1;
        }

        if (pMessage != nullptr)
        {
            NF_SAFE_DELETE(pMessage);
        }

        break;
    }

    return iRet;
}

int NFCMysqlDriver::ExecuteMore(const NFrame::storesvr_execute_more& select,
                                ::google::protobuf::RepeatedPtrField<NFrame::storesvr_execute_more_res>& vecSelectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errorMsg;
    int iRet = ExecuteMore(select.record(), resultVec, errorMsg);
    if (iRet != 0)
    {
        NFrame::storesvr_execute_more_res* select_res = vecSelectRes.Add();
        select_res->mutable_opres()->set_errmsg(errorMsg);
        return -1;
    }

    NFrame::storesvr_execute_more_res* select_res = vecSelectRes.Add();

    select_res->mutable_baseinfo()->CopyFrom(select.baseinfo());
    select_res->mutable_opres()->set_mod_key(select.mod_key());
    select_res->set_is_lastbatch(false);

    int count = 0;
    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];

        google::protobuf::Message* pMessage = nullptr;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != nullptr)
        {
            select_res->add_record(pMessage->SerializePartialAsString());

            count++;
            select_res->set_row_count(count);
            if (select_res->record_size() >= static_cast<int>(select.baseinfo().max_records()))
            {
                count = 0;
                select_res = vecSelectRes.Add();

                select_res->mutable_baseinfo()->CopyFrom(select.baseinfo());
                select_res->mutable_opres()->set_mod_key(select.mod_key());
                select_res->set_is_lastbatch(false);
            }
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                       NFCommon::tostr(result), select.baseinfo().clname());
            iRet = -1;
        }

        if (pMessage != nullptr)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    select_res->set_is_lastbatch(true);

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::ExecuteMore(const NFrame::storesvr_execute_more& select, NFrame::storesvr_execute_more_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", select.record());

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errorMsg;
    int iRet = ExecuteMore(select.record(), resultVec, errorMsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errorMsg);
        return -1;
    }

    selectRes.mutable_baseinfo()->CopyFrom(select.baseinfo());
    selectRes.mutable_opres()->set_mod_key(select.mod_key());

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];

        google::protobuf::Message* pMessage = nullptr;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != nullptr)
        {
            selectRes.add_record(pMessage->SerializePartialAsString());
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} sql:{}",
                       NFCommon::tostr(result), select.record());
            iRet = -1;
        }

        if (pMessage != nullptr)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    return iRet;
}

int NFCMysqlDriver::ExecuteMore(const std::string& qstr, std::vector<std::map<std::string, std::string>>& valueVec,
                                std::string& errorMsg)
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "query:{}", qstr);

    mysqlpp::StoreQueryResult queryResult;
    if (NFCMysqlDriver::Query(qstr, queryResult, errorMsg) == 0)
    {
        for (size_t i = 0; i < queryResult.num_rows(); ++i)
        {
            valueVec.push_back(std::map<std::string, std::string>());
            std::map<std::string, std::string>& tmpVec = valueVec.back();
            for (size_t j = 0; j < queryResult[i].size(); j++)
            {
                std::string value;
                if (!queryResult[i][j].is_null())
                {
                    queryResult[i][j].to_string(value);
                }
                tmpVec.emplace(queryResult.field_name(j), value);
            }
        }
        return 0;
    }
    return -1;
}

/**
 * @brief 执行sql语句, 把数据库配置表里的数据取出来
 *
 * @param  table 配置表表明 sheet_fullname protobuf中代表一个表格的message
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
int NFCMysqlDriver::QueryDescStore(const std::string& table, google::protobuf::Message** pMessage)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- table:{}", table);
    int iRet = 0;
    std::string selectSql = "select * from " + table;
    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    CHECK_EXPR(iRet == 0, iRet, "ExecuteMore:{} Failed!", errmsg);

    if (resultVec.size() <= 0)
    {
        return 0;
    }

    //通过sheet_fullname名字，获得这个protobuf类的默认变量
    std::string sheet_fullname = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + std::string("Sheet_") + table;
    //通过protobuf默认便利new出来一个新的sheet_fullname变量
    ::google::protobuf::Message* pSheetMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(sheet_fullname);
    CHECK_EXPR(pSheetMessageObject, -1, "{} New Failed", sheet_fullname);

    if (pMessage)
    {
        *pMessage = pSheetMessageObject;
    }

    const google::protobuf::Descriptor* pSheetFieldDesc = pSheetMessageObject->GetDescriptor();
    CHECK_EXPR(pSheetFieldDesc, -1, "pSheetFieldDesc == NULL");
    const google::protobuf::Reflection* pSheetReflect = pSheetMessageObject->GetReflection();
    CHECK_EXPR(pSheetReflect, -1, "pSheetFieldDesc == NULL");

    for (int sheet_field_index = 0; sheet_field_index < pSheetFieldDesc->field_count(); sheet_field_index++)
    {
        /*  比如 message Sheet_GameRoomDesc
        *		{
        *			repeated GameRoomDesc GameRoomDesc_List = 1  [(nanopb).max_count=100];
        *		}
        *		获得上面GameRoomDesc_List信息
        */
        const google::protobuf::FieldDescriptor* pSheetRepeatedFieldDesc = pSheetFieldDesc->field(sheet_field_index);
        if (pSheetRepeatedFieldDesc->is_repeated() &&
            pSheetRepeatedFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            //如果is_repeated 开始处理
            for (size_t result_i = 0; result_i < resultVec.size(); result_i++)
            {
                const std::map<std::string, std::string>& result = resultVec[result_i];
                ::google::protobuf::Message* pSheetRepeatedMessageObject = pSheetReflect->AddMessage(
                    pSheetMessageObject, pSheetRepeatedFieldDesc);
                NFProtobufCommon::GetDBMessageFromMapFields(result, pSheetRepeatedMessageObject);
            }
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::QueryDescStore(const std::string& table, google::protobuf::Message* pSheetMessageObject)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- table:{}", table);
    CHECK_EXPR(pSheetMessageObject, -1, "pMessage == NULL");
    int iRet = 0;
    std::string selectSql = "select * from " + table;
    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    CHECK_EXPR(iRet == 0, iRet, "ExecuteMore:{} Failed!", errmsg);

    if (resultVec.size() <= 0)
    {
        return 0;
    }

    const google::protobuf::Descriptor* pSheetFieldDesc = pSheetMessageObject->GetDescriptor();
    CHECK_EXPR(pSheetFieldDesc, -1, "pSheetFieldDesc == NULL");
    const google::protobuf::Reflection* pSheetReflect = pSheetMessageObject->GetReflection();
    CHECK_EXPR(pSheetReflect, -1, "pSheetFieldDesc == NULL");

    for (int sheet_field_index = 0; sheet_field_index < pSheetFieldDesc->field_count(); sheet_field_index++)
    {
        /*  比如 message Sheet_GameRoomDesc
        *		{
        *			repeated GameRoomDesc GameRoomDesc_List = 1  [(nanopb).max_count=100];
        *		}
        *		获得上面GameRoomDesc_List信息
        */
        const google::protobuf::FieldDescriptor* pSheetRepeatedFieldDesc = pSheetFieldDesc->field(sheet_field_index);
        if (pSheetRepeatedFieldDesc->is_repeated() &&
            pSheetRepeatedFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            //如果is_repeated 开始处理
            for (size_t result_i = 0; result_i < resultVec.size(); result_i++)
            {
                const std::map<std::string, std::string>& result = resultVec[result_i];
                ::google::protobuf::Message* pSheetRepeatedMessageObject = pSheetReflect->AddMessage(
                    pSheetMessageObject, pSheetRepeatedFieldDesc);
                NFProtobufCommon::GetDBMessageFromMapFields(result, pSheetRepeatedMessageObject);
            }
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::TransTableRowToMessage(const std::map<std::string, std::string>& result, const std::string& packageName, const std::string& className, google::protobuf::Message** pMessage) const
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- table:{}", className);
    std::string proto_fullname;
    if (packageName.empty())
    {
        proto_fullname = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + className;
    }
    else
    {
        proto_fullname = packageName + "." + className;
    }

    //通过protobuf默认便利new出来一个新的proto_fullname变量
    ::google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(proto_fullname);
    CHECK_EXPR(pMessageObject, -1, "{} New Failed", proto_fullname);

    if (pMessage)
    {
        *pMessage = pMessageObject;
    }

    NFProtobufCommon::GetDBMessageFromMapFields(result, pMessageObject);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCMysqlDriver::SelectByCond(const NFrame::storesvr_sel& select, std::string& privateKey, std::unordered_set<std::string>& fields, std::unordered_set<std::string>& privateKeySet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = GetPrivateKeySql(select, privateKey, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    if (select.baseinfo().sel_fields_size() > 0)
    {
        for (int i = 0; i < (int)select.baseinfo().sel_fields_size(); i++)
        {
            fields.insert(select.baseinfo().sel_fields(i));
        }
    }

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); ++iter)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::SelectByCond(const std::string& packageName, const std::string& tableName, const std::string& className, const std::string& privateKey,
                                 const std::unordered_set<std::string>& leftPrivateKeySet,
                                 std::map<std::string, std::string>& recordsMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(tableName, privateKey, leftPrivateKeySet, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];

        google::protobuf::Message* pMessage = nullptr;
        iRet = TransTableRowToMessage(result, packageName, className, &pMessage);
        auto iter = result.find(privateKey);
        if (iRet == 0 && pMessage != nullptr && iter != result.end())
        {
            std::string record = pMessage->SerializePartialAsString();
            recordsMap.emplace(iter->second, record);
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                       NFCommon::tostr(result), tableName);
            iRet = -1;
        }

        if (pMessage != nullptr)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::SelectByCond(const NFrame::storesvr_sel& select,
                                 ::google::protobuf::RepeatedPtrField<NFrame::storesvr_sel_res>& vecSelectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        NFrame::storesvr_sel_res* select_res = vecSelectRes.Add();
        select_res->mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFrame::storesvr_sel_res* select_res = vecSelectRes.Add();

    select_res->mutable_baseinfo()->CopyFrom(select.baseinfo());
    select_res->mutable_opres()->set_mod_key(select.cond().mod_key());
    select_res->set_is_lastbatch(false);

    int count = 0;
    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];

        google::protobuf::Message* pMessage = nullptr;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != nullptr)
        {
            select_res->add_record(pMessage->SerializePartialAsString());

            count++;
            select_res->set_row_count(count);
            if (select_res->record_size() >= static_cast<int>(select.baseinfo().max_records()))
            {
                count = 0;
                select_res = vecSelectRes.Add();

                select_res->mutable_baseinfo()->CopyFrom(select.baseinfo());
                select_res->mutable_opres()->set_mod_key(select.cond().mod_key());
                select_res->set_is_lastbatch(false);
            }
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                       NFCommon::tostr(result), select.baseinfo().tbname());
            iRet = -1;
        }

        if (pMessage != nullptr)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    select_res->set_is_lastbatch(true);

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::GetPrivateKeySql(const NFrame::storesvr_sel& select, std::string& privateKey, std::string& selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");
    std::string className = select.baseinfo().clname();
    CHECK_EXPR(className.size() > 0, -1, "className empty!");

    std::string packageName = select.baseinfo().package_name();

    int iRet = GetPrivateKey(packageName, className, privateKey);
    CHECK_EXPR(iRet == 0, -1, "GetObjKey Failed!");

    if (!select.has_cond())
    {
        selectSql = "select " + privateKey + " from " + tableName;
    }
    else
    {
        selectSql = "select " + privateKey + " from " + tableName;

        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        if (whereCond.private_keys_size() > 0 || whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        if (whereCond.private_keys_size() > 0)
        {
            std::string sql;
            for (int i = 0; i < whereCond.private_keys_size(); i++)
            {
                sql = privateKey + "='" + whereCond.private_keys(i) + "'";

                if (sql.size() > 0 && i < whereCond.private_keys_size() - 1)
                {
                    sql += " and ";
                }

                if (sql.size() > 0)
                {
                    selectSql += sql;
                }
            }
        }

        if (whereCond.private_keys_size() > 0 && whereCond.where_conds_size() > 0)
        {
            selectSql += " and ";
        }

        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int NFCMysqlDriver::GetPrivateKey(const std::string& packageName, const std::string& className, std::string& privateKey)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + className;
    }
    else
    {
        full_name = packageName + "." + className;
    }

    const google::protobuf::Descriptor* pDescriptor = NFProtobufCommon::Instance()->FindDynamicMessageTypeByName(full_name);
    CHECK_EXPR(pDescriptor, -1, "NFProtobufCommon::FindDynamicMessageTypeByName:{} Failed", full_name);

    int iRet = NFProtobufCommon::GetPrivateKeyFromMessage(pDescriptor, privateKey);
    CHECK_EXPR(iRet == 0, -1, "NFProtobufCommon::GetPrivateKeyFromMessage:{} Failed", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::SelectByCond(const NFrame::storesvr_sel& select,
                                 NFrame::storesvr_sel_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    selectRes.set_is_lastbatch(true);

    int count = 0;
    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];

        google::protobuf::Message* pMessage = nullptr;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != nullptr)
        {
            count++;
            selectRes.add_record(pMessage->SerializePartialAsString());
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                       NFCommon::tostr(result), select.baseinfo().tbname());
            iRet = -1;
        }
        if (pMessage != nullptr)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    selectRes.set_row_count(count);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::SelectObj(const std::string& tbName, google::protobuf::Message* pMessage, std::string& errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- tbName:{} errMsg:{}", tbName, errMsg);
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_selobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializePartialAsString());

    NFrame::storesvr_selobj_res select_res;

    int iRet = SelectObj(select, select_res);
    if (iRet == 0)
    {
        pMessage->ParsePartialFromString(select_res.record());
    }
    else
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::SelectObj(const NFrame::storesvr_selobj& select,
                              NFrame::storesvr_selobj_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    iRet = CreateSql(select, keyMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", iRet);

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.mod_key());

    std::vector<std::string> vecFields;
    for (int i = 0; i < (int)select.baseinfo().sel_fields_size(); i++)
    {
        vecFields.push_back(select.baseinfo().sel_fields(i));
    }

    std::map<std::string, std::string> result;
    std::string errmsg;
    iRet = QueryOne(select.baseinfo().tbname(), keyMap, vecFields, result, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    google::protobuf::Message* pMessage = nullptr;
    iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
    if (iRet == 0 && pMessage != nullptr)
    {
        selectRes.set_record(pMessage->SerializePartialAsString());
        NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                   NFCommon::tostr(result), select.baseinfo().tbname());
        iRet = -1;
    }
    if (pMessage != nullptr)
    {
        NF_SAFE_DELETE(pMessage);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::SelectObj(const std::string& packageName, const std::string& tbName, const std::string& className, const std::string& privateKey, const std::string& privateKeyValue, std::string& record)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    keyMap.emplace(privateKey, privateKeyValue);

    std::vector<std::string> vecFields;

    std::map<std::string, std::string> result;
    std::string errmsg;
    iRet = QueryOne(tbName, keyMap, vecFields, result, errmsg);
    if (iRet != 0)
    {
        return iRet;
    }

    google::protobuf::Message* pMessage = nullptr;
    iRet = TransTableRowToMessage(result, packageName, className, &pMessage);
    if (iRet == 0 && pMessage != nullptr)
    {
        record = pMessage->SerializePartialAsString();
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                   NFCommon::tostr(result), tbName);
        iRet = -1;
    }
    if (pMessage != nullptr)
    {
        NF_SAFE_DELETE(pMessage);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::GetPrivateKeySql(const NFrame::storesvr_del& select, std::string& privateKey, std::string& selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");
    std::string className = select.baseinfo().clname();
    CHECK_EXPR(className.size() > 0, -1, "className empty!");

    std::string packageName = select.baseinfo().package_name();

    int iRet = GetPrivateKey(packageName, className, privateKey);
    CHECK_EXPR(iRet == 0, -1, "GetObjKey Failed!");

    if (!select.has_cond())
    {
        selectSql = "select " + privateKey + " from " + tableName;
    }
    else
    {
        selectSql = "select " + privateKey + " from " + tableName;

        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}


int NFCMysqlDriver::DeleteByCond(const NFrame::storesvr_del& select,
                                 NFrame::storesvr_del_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Delete(selectSql, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCMysqlDriver::DeleteByCond(const NFrame::storesvr_del& select, std::string& privateKey, std::unordered_set<std::string>& privateKeySet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = GetPrivateKeySql(select, privateKey, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); ++iter)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::DeleteByCond(const NFrame::storesvr_del& select, const std::string& privateKey,
                                 const std::unordered_set<std::string>& privateKeySet, NFrame::storesvr_del_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, privateKey, privateKeySet, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Delete(selectSql, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::DeleteObj(const NFrame::storesvr_delobj& select,
                              NFrame::storesvr_delobj_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    iRet = CreateSql(select, keyMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Delete(select.baseinfo().tbname(), keyMap, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_delobj& select, std::map<std::string, std::string>& keyMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string className = select.baseinfo().clname();
    CHECK_EXPR(className.size() > 0, -1, "tableName empty!");
    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + className;
    }
    else
    {
        full_name = packageName + "." + className;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    std::string key;
    std::string value;
    NFProtobufCommon::GetPrivateFieldsFromMessage(*pMessageObject, key, value);
    keyMap.emplace(key, value);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_mod& select, std::string& selectSql)
{
    if (select.has_cond())
    {
        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_update& select, std::string& selectSql)
{
    if (select.has_cond())
    {
        selectSql = " ";
        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_del& select, const std::string& privateKey,
                              const std::unordered_set<std::string>& leftPrivateKeySet, std::string& selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");

    selectSql = "delete from " + tableName + " where ";

    for (auto iter = leftPrivateKeySet.begin(); iter != leftPrivateKeySet.end(); ++iter)
    {
        if (iter != leftPrivateKeySet.begin())
        {
            selectSql += " or ";
        }
        selectSql += privateKey + " = \"" + *iter + "\"";
    }

    selectSql += ";\n";

    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_del& select, std::string& selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");

    if (select.has_cond())
    {
        selectSql = "delete from " + tableName;
        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int
NFCMysqlDriver::CreateSql(const NFrame::storesvr_selobj& select, std::map<std::string, std::string>& keyMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string className = select.baseinfo().clname();
    CHECK_EXPR(className.size() > 0, -1, "className empty!");
    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + className;
    }
    else
    {
        full_name = packageName + "." + className;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    NFProtobufCommon::GetDBMapFieldsFromMessage(*pMessageObject, keyMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::CreateSql(const std::string& tableName, const std::string& privateKey,
                              const std::unordered_set<std::string>& leftPrivateKeySet, std::string& selectSql)
{
    std::string stringFileds = "*";
    selectSql = "select " + stringFileds + " from " + tableName;

    if (leftPrivateKeySet.size() > 0)
    {
        selectSql += " where ";
    }

    for (auto iter = leftPrivateKeySet.begin(); iter != leftPrivateKeySet.end(); ++iter)
    {
        if (iter != leftPrivateKeySet.begin())
        {
            selectSql += " or ";
        }
        selectSql += privateKey + " = \"" + *iter + "\"";
    }

    selectSql += ";\n";

    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_sel& select, std::string& selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");

    std::string stringFileds = "*";
    if (select.baseinfo().sel_fields_size() > 0)
    {
        stringFileds = "";
        for (int i = 0; i < (int)select.baseinfo().sel_fields_size(); i++)
        {
            if (i != select.baseinfo().sel_fields_size() - 1)
            {
                stringFileds += select.baseinfo().sel_fields(i) + ",";
            }
            else
            {
                stringFileds += select.baseinfo().sel_fields(i);
            }
        }
    }

    if (!select.has_cond())
    {
        selectSql = "select " + stringFileds + " from " + tableName;
    }
    else
    {
        selectSql = "select " + stringFileds + " from " + tableName;

        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        if (whereCond.private_keys_size() > 0 || whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }

        if (whereCond.private_keys_size() > 0)
        {
            std::string privateKey;
            int iRet = GetPrivateKey(select.baseinfo().package_name(), select.baseinfo().clname(), privateKey);
            CHECK_ERR(0, iRet, "GetPrivateKey Failed, packageName:{} className:{}", select.baseinfo().package_name(), select.baseinfo().clname());
            for (int i =0; i < whereCond.private_keys_size(); i++)
            {
                std::string sql;
                sql = privateKey + "='" + whereCond.private_keys(i) + "'";
                if (sql.size() > 0 && i < whereCond.private_keys_size() - 1)
                {
                    sql += " and ";
                }

                if (sql.size() > 0)
                {
                    selectSql += sql;
                }
            }
        }

        if (whereCond.private_keys_size() > 0 && whereCond.where_conds_size() > 0)
        {
            selectSql += " and ";
        }

        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

mysqlpp::Connection* NFCMysqlDriver::GetConnection()
{
    return m_pMysqlConnect;
}

void NFCMysqlDriver::CloseConnection()
{
    if (m_pMysqlConnect)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "CloseConnection dbName:{} dbHost:{} dbPort:{}", m_strDbName, m_strDbHost, m_iDbPort);
        delete m_pMysqlConnect;
        m_pMysqlConnect = nullptr;
    }
}

bool NFCMysqlDriver::Enable()
{
    return !IsNeedReconnect();
}

bool NFCMysqlDriver::CanReconnect()
{
    m_fCheckReconnect += 0.1f;

    //30检查断线重连
    if (m_fCheckReconnect < m_iReconnectTime)
    {
        return false;
    }

    if (m_iReconnectCount == 0)
    {
        return false;
    }

    m_fCheckReconnect = 0.0f;

    return true;
}

int NFCMysqlDriver::Reconnect()
{
    CloseConnection();
    NFLogInfo(NF_LOG_DEFAULT, 0, "Start Reconnect nServerID:{}, strIP:{}, nPort{}, strDBName:{}, strDBUser:{}, strDBPwd:{}",
              m_strDbName, m_strDbHost, m_iDbPort, m_strDbName, m_strDbUser, m_strDbPwd);
    int iRet = Connect(m_strDbName, m_strDbHost, m_iDbPort, m_strDbUser, m_strDbPwd);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0,
                   "Reconnect Failed!:nServerID:{}, strIP:{}, nPort{}, strDBName:{}, strDBUser:{}, strDBPwd:{}",
                   m_strDbName, m_strDbHost, m_iDbPort, m_strDbName, m_strDbUser, m_strDbPwd);
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "Reconnect Success!:nServerID:{}, strIP:{}, nPort{}, strDBName:{}, strDBUser:{}, strDBPwd:{}",
                  m_strDbName, m_strDbHost, m_iDbPort, m_strDbName, m_strDbUser, m_strDbPwd);
    }

    if (m_iReconnectCount > 0)
    {
        m_iReconnectCount--;
    }

    return 0;
}

bool NFCMysqlDriver::IsNeedReconnect()
{
    //没有配置表
    if (m_strDbHost.length() < 1 || m_strDbUser.length() < 1)
    {
        return false;
    }

    if (nullptr == m_pMysqlConnect)
    {
        return true;
    }

    if (!m_pMysqlConnect->connected())
    {
        CloseConnection();
        return true;
    }

    if (!m_pMysqlConnect->ping())
    {
        CloseConnection();
        return true;
    }

    return false;
}

int NFCMysqlDriver::Connect()
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    m_pMysqlConnect = new mysqlpp::Connection();
    if (nullptr == m_pMysqlConnect)
    {
        return -1;
    }
    // ReSharper disable once CppEntityAssignedButNoRead
    std::string errorMsg;
    NFMYSQLTRYBEGIN
        m_pMysqlConnect->set_option(new mysqlpp::MultiStatementsOption(true));
        m_pMysqlConnect->set_option(new mysqlpp::SetCharsetNameOption("utf8mb4"));
        m_pMysqlConnect->set_option(new mysqlpp::ReconnectOption(true));
        m_pMysqlConnect->set_option(new mysqlpp::ConnectTimeoutOption(60));
        m_pMysqlConnect->set_option(new mysqlpp::ReadTimeoutOption(1));

        {
            //m_pMysqlConnect在调用Connect会引发多线程的崩溃，必须枷锁
            // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
            NFLock lock(m_stConnectLock);
            if (!m_pMysqlConnect->connect(m_strDbName.c_str(), m_strDbHost.c_str(), m_strDbUser.c_str(), m_strDbPwd.c_str(),
                                          m_iDbPort))
            {
                CloseConnection();
                // 连接失败
                return -1;
            }
        }

        // 设置超时时间为24小时
        mysqlpp::Query query = m_pMysqlConnect->query("set interactive_timeout = 24*3600");
        query.execute();
        query.reset();
    NFMYSQLTRYEND("Connect faild")
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Disconnect()
{
    CloseConnection();
    return 0;
}

int NFCMysqlDriver::Update(const std::string& strTableName, const std::map<std::string, std::string>& keyMap,
                           const std::map<std::string, std::string>& keyValueMap,
                           std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }

    bool bExist = false;
    if (Exists(strTableName, keyMap, bExist) != 0)
    {
        return -1;
    }

    int iRet = 0;
    if (bExist)
    {
        iRet = Modify(strTableName, keyMap, keyValueMap, errorMsg);
    }
    else
    {
        std::map<std::string, std::string> insertMap = keyValueMap;
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            insertMap.emplace(iter->first, iter->second);
        }
        iRet = Insert(strTableName, insertMap, errorMsg);
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::Modify(const std::string& strTableName, const std::string& where,
                           const std::map<std::string, std::string>& keyValueMap, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }

    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        // update
        query << "UPDATE " << strTableName << " SET ";
        int i = 0;
        for (auto iter = keyValueMap.begin(); iter != keyValueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << "," << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            i++;
        }

        if (!where.empty())
        {
            query << " WHERE " << where;
        }
        query << ";";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        query.execute();
        query.reset();
    NFMYSQLTRYEND("modify error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Modify(const std::string& strTableName, const std::map<std::string, std::string>& keyMap,
                           const std::map<std::string, std::string>& keyValueMap, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }

    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        // update
        query << "UPDATE " << strTableName << " SET ";
        int i = 0;
        for (auto iter = keyValueMap.begin(); iter != keyValueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << "," << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            i++;
        }

        query << " WHERE ";
        i = 0;
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            i++;
        }
        query << ";";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        query.execute();
        query.reset();
    NFMYSQLTRYEND("modify error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Insert(const std::string& strTableName, const std::map<std::string, std::string>& keyValueMap,
                           std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        // insert
        query << "INSERT INTO " << strTableName << "(";
        int i = 0;
        for (auto iter = keyValueMap.begin(); iter != keyValueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first;
            }
            else
            {
                query << ", " << iter->first;
            }
            i++;
        }

        query << ") VALUES (";
        i = 0;
        for (auto iter = keyValueMap.begin(); iter != keyValueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << mysqlpp::quote << iter->second;
            }
            else
            {
                query << ", " << mysqlpp::quote << iter->second;
            }
            i++;
        }

        query << ");";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        query.execute();
        query.reset();
    NFMYSQLTRYEND("error")
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::QueryOne(const std::string& strTableName, const std::map<std::string, std::string>& keyMap,
                             std::map<std::string, std::string>& valueVec, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    valueVec.clear();
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "SELECT * ";

        query << " FROM " << strTableName << " WHERE ";
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (iter == keyMap.begin())
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << mysqlpp::quote << iter->second;
            }
        }
        query << " limit 1;";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        //query.execute(); // 官网例子不需要execute
        mysqlpp::StoreQueryResult xResult = query.store();
        query.reset();

        if (xResult.empty() || !xResult)
        {
            return NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
        }

        for (size_t i = 0; i < xResult.num_rows(); ++i)
        {
            for (size_t j = 0; j < xResult[i].size(); j++)
            {
                std::string value;
                if (!xResult[i][j].is_null())
                {
                    xResult[i][j].to_string(value);
                }
                valueVec.emplace(xResult.field_name(j), value);
            }
        }
    NFMYSQLTRYEND("query error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::QueryOne(const std::string& strTableName, const std::map<std::string, std::string>& keyMap,
                             const std::vector<std::string>& fieldVec,
                             std::map<std::string, std::string>& valueVec, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    valueVec.clear();
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "SELECT ";
        if (fieldVec.size() > 0)
        {
            for (auto iter = fieldVec.begin(); iter != fieldVec.end(); ++iter)
            {
                if (iter == fieldVec.begin())
                {
                    query << *iter;
                }
                else
                {
                    query << "," << *iter;
                }
            }
        }
        else
        {
            query << "*";
        }


        query << " FROM " << strTableName << " WHERE ";
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (iter == keyMap.begin())
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << mysqlpp::quote << iter->second;
            }
        }
        query << " limit 1;";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        //query.execute(); // 官网例子不需要execute
        mysqlpp::StoreQueryResult xResult = query.store();
        query.reset();

        if (xResult.empty() || !xResult)
        {
            return NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
        }

        for (size_t i = 0; i < xResult.num_rows(); ++i)
        {
            for (size_t j = 0; j < xResult[i].size(); j++)
            {
                std::string value;
                if (!xResult[i][j].is_null())
                {
                    xResult[i][j].to_string(value);
                }
                valueVec.emplace(xResult.field_name(j), value);
            }
        }
    NFMYSQLTRYEND("query error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::QueryMore(const std::string& strTableName, const std::map<std::string, std::string>& keyMap,
                              const std::vector<std::string>& fieldVec,
                              std::vector<std::map<std::string, std::string>>& valueVec, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    valueVec.clear();
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "SELECT ";
        for (std::vector<std::string>::const_iterator iter = fieldVec.begin(); iter != fieldVec.end(); ++iter)
        {
            if (iter == fieldVec.begin())
            {
                query << *iter;
            }
            else
            {
                query << "," << *iter;
            }
        }

        query << " FROM " << strTableName << " WHERE ";
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (iter == keyMap.begin())
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << mysqlpp::quote << iter->second;
            }
        }
        query << ";";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        //query.execute(); // 官网例子不需要execute
        mysqlpp::StoreQueryResult xResult = query.store();
        query.reset();

        if (xResult.empty() || !xResult)
        {
            return NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
        }

        for (size_t i = 0; i < xResult.num_rows(); ++i)
        {
            valueVec.push_back(std::map<std::string, std::string>());
            std::map<std::string, std::string>& tmpVec = valueVec.back();
            for (size_t j = 0; j < xResult[i].size(); j++)
            {
                std::string value;
                if (!xResult[i][j].is_null())
                {
                    xResult[i][j].to_string(value);
                }
                tmpVec.emplace(xResult.field_name(j), value);
            }
        }
    NFMYSQLTRYEND("query error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Delete(const std::string& strTableName, const std::map<std::string, std::string>& keyMap, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "DELETE FROM " << strTableName << " WHERE ";
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (iter == keyMap.begin())
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << mysqlpp::quote << iter->second;
            }
        }
        query << ";";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        query.execute();
        query.reset();

    NFMYSQLTRYEND("delete error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Delete(const std::string& strTableName, const std::string& strKeyColName,
                           const std::string& strKey, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "DELETE FROM " << strTableName << " WHERE " << strKeyColName << " = " << mysqlpp::quote << strKey <<
            ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        query.execute();
        query.reset();

    NFMYSQLTRYEND("delete error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Delete(const std::string& sql, std::string& errorMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }

    std::string msg = "delete error";
    try
    {
        mysqlpp::Query query = pConnection->query();
        query << sql << ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        query.execute();
        query.reset();
    }
    catch (mysqlpp::BadQuery er)
    {
        errorMsg = er.what();
        NFLogError(NF_LOG_DEFAULT, 0, "BadQuery [{}] Error:{}", msg, er.what());
        return -1;
    }
    catch (const mysqlpp::BadConversion& er)
    {
        errorMsg = er.what();
        NFLogError(NF_LOG_DEFAULT, 0, "BadConversion [{}] Error:{} retrieved data size:{}, actual size:{}", msg,
                   er.what(), er.retrieved, er.actual_size);
        return -1;
    }
    catch (const mysqlpp::Exception& er)
    {
        errorMsg = er.what();
        NFLogError(NF_LOG_DEFAULT, 0, "mysqlpp::Exception [{}] Error:{}", msg, er.what());
        return -1;
    }
    catch (...)
    {
        errorMsg = "Unknown Error";
        NFLogError(NF_LOG_DEFAULT, 0, "std::exception [{}] Error:Error:Unknown", msg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Exists(const std::string& strTableName, const std::map<std::string, std::string>& keyMap,
                           bool& bExit)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }

    if (keyMap.empty())
    {
        bExit = false;
        return 0;
    }

    // ReSharper disable once CppEntityAssignedButNoRead
    std::string errorMsg;
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "SELECT 1 FROM " << strTableName << " WHERE ";
        int i = 0;
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << mysqlpp::quote << iter->second;
            }
            i++;
        }
        query << " limit 1;";

        NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        //query.execute();
        mysqlpp::StoreQueryResult result = query.store();
        query.reset();

        if (!result || result.empty())
        {
            bExit = false;
            return 0;
        }

    NFMYSQLTRYEND("exist error")

    bExit = true;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::Exists(const std::string& strTableName, const std::string& strKeyColName,
                           const std::string& strKey, bool& bExit)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    mysqlpp::Connection* pConnection = GetConnection();
    if (nullptr == pConnection)
    {
        return -1;
    }
    // ReSharper disable once CppEntityAssignedButNoRead
    std::string errorMsg;
    NFMYSQLTRYBEGIN
        mysqlpp::Query query = pConnection->query();
        query << "SELECT 1 FROM " << strTableName << " WHERE " << strKeyColName << " = " << mysqlpp::quote << strKey <<
            " LIMIT 1;";

        mysqlpp::StoreQueryResult result = query.store();
        query.reset();

        if (!result || result.empty())
        {
            bExit = false;
            return 0;
        }

    NFMYSQLTRYEND("exist error")

    bExit = true;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::InsertObj(const std::string& tbName, const google::protobuf::Message* pMessage, std::string& errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_insertobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializePartialAsString());

    NFrame::storesvr_insertobj_res select_res;
    int iRet = InsertObj(select, select_res);
    if (iRet != 0)
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::InsertObj(const NFrame::storesvr_insertobj& select,
                              NFrame::storesvr_insertobj_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> resultMap;
    iRet = CreateSql(select, resultMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Insert(select.baseinfo().tbname(), resultMap, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCMysqlDriver::CreateSql(const NFrame::storesvr_insertobj& select, std::map<std::string, std::string>& resultMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");

    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    }
    else
    {
        full_name = packageName + "." + tableName;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    NFProtobufCommon::GetDBMapFieldsFromMessage(*pMessageObject, resultMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::ModifyObj(const std::string& tbName, const google::protobuf::Message* pMessage, std::string& errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_modobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializePartialAsString());

    NFrame::storesvr_modobj_res select_res;
    int iRet = ModifyObj(select, select_res);
    if (iRet != 0)
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::GetPrivateKeySql(const NFrame::storesvr_mod& select, std::string& privateKey, std::string& selectSql)
{
    std::string className = select.baseinfo().clname();
    CHECK_EXPR(className.size() > 0, -1, "className empty!");
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");

    std::string packageName = select.baseinfo().package_name();

    int iRet = GetPrivateKey(packageName, className, privateKey);
    CHECK_EXPR(iRet == 0, -1, "GetObjKey Failed!");

    if (!select.has_cond())
    {
        selectSql = "select " + privateKey + " from " + tableName;
    }
    else
    {
        selectSql = "select " + privateKey + " from " + tableName;

        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " and ";
        }
        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int NFCMysqlDriver::UpdateByCond(const NFrame::storesvr_update& select, std::string& privateKey, std::unordered_set<std::string>& privateKeySet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = GetPrivateKeySql(select, privateKey, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); ++iter)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::ModifyByCond(const NFrame::storesvr_mod& select, std::string& privateKey, std::unordered_set<std::string>& privateKeySet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = GetPrivateKeySql(select, privateKey, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        return -1;
    }

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string>& result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); ++iter)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::ModifyByCond(const NFrame::storesvr_mod& select, NFrame::storesvr_mod_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    std::string where;
    CreateSql(select, where);

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Modify(select.baseinfo().tbname(), where, keyValueMap, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}


int NFCMysqlDriver::ModifyObj(const NFrame::storesvr_modobj& select,
                              NFrame::storesvr_modobj_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Modify(select.baseinfo().tbname(), keyMap, keyValueMap, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_mod& select, std::map<std::string, std::string>& keyMap,
                              std::map<std::string, std::string>& kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");
    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    }
    else
    {
        full_name = packageName + "." + tableName;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "CreateSql From message:{}", pMessageObject->DebugString());

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_update& select, std::map<std::string, std::string>& keyMap,
                              std::map<std::string, std::string>& kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");
    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    }
    else
    {
        full_name = packageName + "." + tableName;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "CreateSql From message:{}", pMessageObject->DebugString());

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::CreateSql(const NFrame::storesvr_modobj& select, std::map<std::string, std::string>& keyMap,
                              std::map<std::string, std::string>& kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");
    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    }
    else
    {
        full_name = packageName + "." + tableName;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "CreateSql From message:{}", pMessageObject->DebugString());

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::UpdateObj(const std::string& tbName, const google::protobuf::Message* pMessage, std::string& errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_updateobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializePartialAsString());

    NFrame::storesvr_updateobj_res select_res;
    int iRet = UpdateObj(select, select_res);
    if (iRet != 0)
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::GetPrivateKeySql(const NFrame::storesvr_update& select, std::string& privateKey, std::string& selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");

    std::string packageName = select.baseinfo().package_name();

    int iRet = GetPrivateKey(packageName, tableName, privateKey);
    CHECK_EXPR(iRet == 0, -1, "GetObjKey Failed!");

    if (!select.has_cond())
    {
        selectSql = "select " + privateKey + " from " + tableName;
    }
    else
    {
        selectSql = "select " + privateKey + " from " + tableName;

        const ::NFrame::storesvr_wherecond& whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk& vk = whereCond.where_conds(i);
            if (vk.cmp_operator() == ::NFrame::E_CMPOP_EQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATER)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESS)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<'" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_GREATEREQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + ">='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + ">=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_LESSEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "<='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "<=" + vk.column_value();
                }
            }
            else if (vk.cmp_operator() == ::NFrame::E_CMPOP_NOTEQUAL)
            {
                if (vk.column_type() == ::NFrame::E_COLUMNTYPE_STRING)
                {
                    sql += vk.column_name() + "!='" + vk.column_value() + "'";
                }
                else
                {
                    sql += vk.column_name() + "!=" + vk.column_value();
                }
            }

            if (sql.size() > 0 && i < whereCond.where_conds_size() - 1)
            {
                if (vk.logic_operator() == ::NFrame::E_LOGICOP_AND)
                {
                    sql += " and ";
                }
                else if (vk.logic_operator() == ::NFrame::E_LOGICOP_OR)
                {
                    sql += " or ";
                }
            }

            if (sql.size() > 0)
            {
                selectSql += sql;
            }
        }

        if (whereCond.where_conds_size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " and ";
        }

        if (whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int NFCMysqlDriver::UpdateByCond(const NFrame::storesvr_update& select, NFrame::storesvr_update_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    std::string where;
    CreateSql(select, where);

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Modify(select.baseinfo().tbname(), where, keyValueMap, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::UpdateObj(const NFrame::storesvr_updateobj& select,
                              NFrame::storesvr_updateobj_res& selectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *selectRes.mutable_baseinfo() = select.baseinfo();
    selectRes.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Update(select.baseinfo().tbname(), keyMap, keyValueMap, errmsg);
    if (iRet != 0)
    {
        selectRes.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCMysqlDriver::CreateSql(const NFrame::storesvr_updateobj& select, std::map<std::string, std::string>& keyMap,
                          std::map<std::string, std::string>& kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "tableName empty!");
    std::string packageName = select.baseinfo().package_name();

    std::string full_name;
    if (packageName.empty())
    {
        full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    }
    else
    {
        full_name = packageName + "." + tableName;
    }
    google::protobuf::Message* pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCMysqlDriver::ExistsDb(const std::string& dbName, bool& bExit)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = Exists("information_schema.SCHEMATA", "SCHEMA_NAME", dbName, bExit);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "ExistsDB Error, dbName:{}", dbName);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

/**
 * @brief 创建数据库
 * @param dbName
 * @return
 */
int NFCMysqlDriver::CreateDb(const std::string& dbName)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string errorMsg;
    int iRet = 0;
    NFMYSQLTRYBEGIN
        mysqlpp::Connection* pConnection = GetConnection();
        if (nullptr == pConnection)
        {
            iRet = -1;
        }
        else
        {
            if (pConnection->create_db(dbName) == false)
            {
                iRet = -1;
            }
        }
    NFMYSQLTRYEND("error")

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "create db failed, dbName:{} errMsg:{}", dbName, errorMsg);
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "Create Database:{} Success", dbName);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

/**
 * @brief 选择数据库
 * @param dbName
 * @return
 */
int NFCMysqlDriver::SelectDb(const std::string& dbName)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string errorMsg;
    int iRet = 0;
    NFMYSQLTRYBEGIN
        mysqlpp::Connection* pConnection = GetConnection();
        if (nullptr == pConnection)
        {
            iRet = -1;
        }
        else
        {
            if (pConnection->select_db(dbName) == false)
            {
                iRet = -1;
            }
        }
    NFMYSQLTRYEND("error")

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "create db failed, dbName:{} errMsg:{}", dbName, errorMsg);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::ExistTable(const std::string& dbName, const std::string& tableName, bool& bExit)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::map<std::string, std::string> keyMap;
    keyMap.emplace("TABLE_SCHEMA", dbName);
    keyMap.emplace("TABLE_NAME", tableName);
    int iRet = Exists("information_schema.TABLES", keyMap, bExit);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "ExistTable Error, dbName:{}, tableName:{}", dbName, tableName);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}


int NFCMysqlDriver::GetTableColInfo(const std::string& dbName, const std::string& tableName, std::map<std::string, DBTableColInfo>& col)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::map<std::string, std::string> keyMap;
    keyMap.emplace("table_schema", dbName);
    keyMap.emplace("TABLE_NAME", tableName);
    std::vector<std::string> fieldVec;
    fieldVec.push_back("column_name");
    fieldVec.push_back("data_type");
    fieldVec.push_back("column_type");
    fieldVec.push_back("CHARACTER_MAXIMUM_LENGTH");
    fieldVec.push_back("column_key");
    std::vector<std::map<std::string, std::string>> valueVec;
    std::string errorMsg;
    int iRet = QueryMore("information_schema.COLUMNS", keyMap, fieldVec, valueVec, errorMsg);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "QueryMore Error, dbName:{}, tableName:{} errorMsg:{}", dbName, tableName, errorMsg);
        return iRet;
    }

    for (int i = 0; i < static_cast<int>(valueVec.size()); i++)
    {
        std::map<std::string, std::string>& colData = valueVec[i];
        std::string fields = colData["column_name"];
        std::string strDataType = colData["data_type"];
        std::string strColumnType = colData["column_type"];
        std::string charMax = colData["CHARACTER_MAXIMUM_LENGTH"];
        std::string columnKey = colData["column_key"];
        DBTableColInfo colInfo;
        colInfo.m_colType = NFProtobufCommon::GetPBDataTypeFromDBDataType(strDataType, strColumnType);
        if (colInfo.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
        {
            colInfo.m_bufsize = NFCommon::strto<uint32_t>(charMax);
            if (colInfo.m_bufsize >= 1024)
            {
                colInfo.m_bufsize = 1024;
            }
        }

        if (columnKey == "PRI")
        {
            colInfo.m_primaryKey = true;
        }
        else if (columnKey == "UNI")
        {
            colInfo.m_unionKey = true;
        }
        else if (columnKey == "MUL")
        {
            colInfo.m_indexKey = true;
        }


        col[fields] = colInfo;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::QueryTableInfo(const std::string& dbName, const std::string& tableName, bool& bExit,
                                   std::map<std::string, DBTableColInfo>& primaryKey, std::multimap<uint32_t, std::string>& needCreateColumn)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = ExistTable(dbName, tableName, bExit);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "QueryTableInfo Error, dbName:{}, tableName:{}", dbName, tableName);
        return iRet;
    }

    std::string full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    const google::protobuf::Descriptor* pDescriptor = NFProtobufCommon::Instance()->FindDynamicMessageTypeByName(full_name);
    CHECK_EXPR(pDescriptor, -1, "NFProtobufCommon::FindDynamicMessageTypeByName:{} Failed", full_name);

    std::vector<std::pair<std::string, DBTableColInfo>> mapFields;
    iRet = NFProtobufCommon::Instance()->GetDbFieldsInfoFromMessage(pDescriptor, primaryKey, mapFields);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetDbFieldsInfoFromMessage Error, dbName:{}, tableName:{}", dbName, tableName);
        return iRet;
    }

    std::map<std::string, DBTableColInfo> colData;
    if (bExit)
    {
        iRet = GetTableColInfo(dbName, tableName, colData);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "GetTableColInfo Error, dbName:{}, tableName:{}", dbName, tableName);
            return iRet;
        }

        for (auto iter = primaryKey.begin(); iter != primaryKey.end(); ++iter)
        {
            auto findIter = colData.find(iter->first);
            if (findIter != colData.end())
            {
                if (findIter->second.m_colType != iter->second.m_colType)
                {
                    NFLogError(NF_LOG_DEFAULT, 0,
                               "dbName:{}, tableName:{} Exist Col:{}, but the db col data type:{} is not equal protobuf data type:{}, please check",
                               dbName, tableName, iter->first, findIter->second.m_colType, iter->second.m_colType);
                }
                else
                {
                    std::string sql;

                    if (findIter->second.m_primaryKey == true && iter->second.m_primaryKey == false)
                    {
                        sql.clear();
                        NF_FORMAT_EXPR(sql, "alter table {} DROP PRIMARY KEY;", tableName);
                        if (sql.size() > 0)
                            needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                    }

                    if (findIter->second.m_primaryKey == false && iter->second.m_primaryKey == true)
                    {
                        sql.clear();
                        NF_FORMAT_EXPR(sql, "alter table {} add PRIMARY KEY ({});", tableName, iter->first);
                        if (sql.size() > 0)
                            needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                    }
                }
            }
            else
            {
                std::string otherInfo;
                if (iter->second.m_notNull)
                {
                    otherInfo += " NOT NULL ";
                }

                if (iter->second.m_autoIncrement)
                {
                    otherInfo += " AUTO_INCREMENT ";
                }

                if (iter->second.m_isDefaultValue)
                {
                    otherInfo += " Default = " + NFCommon::tostr(iter->second.m_defaultValue);
                }

                if (iter->second.m_comment.size() > 0)
                {
                    otherInfo += " COMMENT \"" + iter->second.m_comment + "\"";
                }

                std::string sql;
                NF_FORMAT_EXPR(sql, "alter table {} add column `{}` {} {};", tableName, iter->first,
                               NFProtobufCommon::GetDBDataTypeFromPBDataType(iter->second.m_colType, iter->second.m_bufsize), otherInfo);

                if (sql.size() > 0)
                    needCreateColumn.emplace(iter->second.m_fieldIndex, sql);

                if (iter->second.m_primaryKey)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} add PRIMARY KEY {};", tableName, iter->first);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }
            }
        }
    }

    for (auto iter = mapFields.begin(); iter != mapFields.end(); ++iter)
    {
        auto findIter = colData.find(iter->first);
        if (findIter != colData.end())
        {
            if (findIter->second.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE && iter->second.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
            {
                findIter->second.m_colType = google::protobuf::FieldDescriptor::CPPTYPE_STRING;
            }

            if (findIter->second.m_colType != iter->second.m_colType)
            {
                NFLogError(NF_LOG_DEFAULT, 0,
                           "dbName:{}, tableName:{} Exist Col:{}, but the db col data type:{} is not equal protobuf data type:{}, please check",
                           dbName, tableName, iter->first, findIter->second.m_colType, iter->second.m_colType);
            }
            else
            {
                std::string sql;

                if (findIter->second.m_primaryKey == true && iter->second.m_primaryKey == false)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} DROP PRIMARY KEY;", tableName);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }

                if (findIter->second.m_unionKey == true && iter->second.m_unionKey == false)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} DROP INDEX {};", tableName, iter->first);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }

                if (findIter->second.m_indexKey == true && iter->second.m_indexKey == false)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} DROP INDEX {};", tableName, iter->first);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }

                if (findIter->second.m_primaryKey == false && iter->second.m_primaryKey == true)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} add PRIMARY KEY ({});", tableName, iter->first);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }

                if (findIter->second.m_unionKey == false && iter->second.m_unionKey == true)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} add UNIQUE {} ({});", tableName, iter->first, iter->first);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }

                if (findIter->second.m_indexKey == false && iter->second.m_indexKey == true)
                {
                    sql.clear();
                    NF_FORMAT_EXPR(sql, "alter table {} add INDEX {} ({});", tableName, iter->first, iter->first);
                    if (sql.size() > 0)
                        needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
                }
            }
        }
        else
        {
            std::string otherInfo;
            if (iter->second.m_notNull)
            {
                otherInfo += " NOT NULL ";
            }

            if (iter->second.m_isDefaultValue)
            {
                otherInfo += " Default = " + NFCommon::tostr(iter->second.m_defaultValue);
            }

            if (iter->second.m_comment.size() > 0)
            {
                otherInfo += " COMMENT \"" + iter->second.m_comment + "\"";
            }

            std::string sql;
            NF_FORMAT_EXPR(sql, "alter table {} add column `{}` {} {};", tableName, iter->first,
                           NFProtobufCommon::GetDBDataTypeFromPBDataType(iter->second.m_colType, iter->second.m_bufsize), otherInfo);

            if (sql.size() > 0)
                needCreateColumn.emplace(iter->second.m_fieldIndex, sql);

            if (iter->second.m_primaryKey)
            {
                sql.clear();
                NF_FORMAT_EXPR(sql, "alter table {} add PRIMARY KEY ({});", tableName, iter->first);
                if (sql.size() > 0)
                    needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
            }

            if (iter->second.m_unionKey)
            {
                sql.clear();
                NF_FORMAT_EXPR(sql, "alter table {} add UNIQUE {} ({});", tableName, iter->first, iter->first);
                if (sql.size() > 0)
                    needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
            }

            if (iter->second.m_indexKey)
            {
                sql.clear();
                NF_FORMAT_EXPR(sql, "alter table {} add INDEX {} ({});", tableName, iter->first, iter->first);
                if (sql.size() > 0)
                    needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
            }
        }
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::CreateTable(const std::string& tableName, const std::map<std::string, DBTableColInfo>& primaryKey,
                                const std::multimap<uint32_t, std::string>& needCreateColumn)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::string sql;
    std::string colSql;
    std::string privateKey = "PRIMARY KEY(";
    std::string auto_increment;

    for (auto iter = primaryKey.begin(); iter != primaryKey.end(); ++iter)
    {
        if (iter == primaryKey.begin())
        {
            privateKey += iter->first;
        }
        else
        {
            privateKey += "," + iter->first;
        }

        std::string col;
        std::string otherInfo;
        if (iter->second.m_notNull)
        {
            otherInfo += " NOT NULL ";
        }

        if (iter->second.m_autoIncrement)
        {
            otherInfo += " AUTO_INCREMENT ";
            auto_increment = "AUTO_INCREMENT = " + NFCommon::tostr(iter->second.m_autoIncrementValue);
        }

        if (iter->second.m_isDefaultValue)
        {
            otherInfo += " Default = " + NFCommon::tostr(iter->second.m_defaultValue);
        }

        if (iter->second.m_comment.size() > 0)
        {
            otherInfo += " COMMENT \"" + iter->second.m_comment + "\"";
        }
        NF_FORMAT_EXPR(col, " {} {} {},", iter->first, NFProtobufCommon::GetDBDataTypeFromPBDataType(iter->second.m_colType, iter->second.m_bufsize),
                       otherInfo)
        colSql += col;
    }
    privateKey += ")";

    NF_FORMAT_EXPR(sql, "CREATE TABLE IF NOT EXISTS {} ({} {}) ENGINE=InnoDB {} DEFAULT CHARSET=utf8;", tableName, colSql, privateKey,
                   auto_increment);

    std::map<std::string, std::string> mapValue;
    std::string errMsg;
    iRet = ExecuteOne(sql, mapValue, errMsg);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "executeone sql:{} fail, err:{}", sql, errMsg);
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "Create Table Success! sql:{}", sql);
    }

    for (auto iter = needCreateColumn.begin(); iter != needCreateColumn.end(); ++iter)
    {
        iRet = ExecuteOne(iter->second, mapValue, errMsg);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "executeone sql:{} fail, err:{}", iter->second, errMsg);
            return iRet;
        }
        else
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "add Table Col Success! sql:{}", iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCMysqlDriver::AddTableRow(const std::string& tableName, const std::multimap<uint32_t, std::string>& needCreateColumn)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;

    std::map<std::string, std::string> mapValue;
    std::string errMsg;
    for (auto iter = needCreateColumn.begin(); iter != needCreateColumn.end(); ++iter)
    {
        iRet = ExecuteOne(iter->second, mapValue, errMsg);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "executeone sql:{} fail, err:{}", iter->second, errMsg);
            return iRet;
        }
        else
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "add Table Col Success! sql:{}", iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}
