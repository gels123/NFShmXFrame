#include "NFCZDBDriver.h"
#include "NFComm/NFCore/NFCommon.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFKernelMessage/FrameMsg.pb.h"

NFCZDBDriver::NFCZDBDriver(const int nReconnectTime/* = 60*/, const int nReconnectCount /*= -1*/)
{
    mfCheckReconnect = 0.0f;
    m_dbType = NFrame::E_DBTYPE_SQLITE;
    mnDBPort = 0;
    mnReconnectTime = nReconnectTime;
    mnReconnectCount = nReconnectCount;
    m_pURL = NULL;
    m_pConnectionPool = NULL;
}

NFCZDBDriver::NFCZDBDriver(uint32_t dbType, const std::string &strDBName, const std::string &strDBHost, const int nDBPort,
                               const std::string &strDBUser, const std::string &strDBPwd)
{
    mfCheckReconnect = 0.0f;
    mnDBPort = 0;
    mnReconnectTime = 3;
    mnReconnectCount = -1;
    m_pURL = NULL;
    m_pConnectionPool = NULL;
    NFCZDBDriver::Connect(dbType, strDBName, strDBHost, nDBPort, strDBUser, strDBPwd);
}

NFCZDBDriver::~NFCZDBDriver()
{
    if (m_pCurConnection)
    {
        Connection_close(m_pCurConnection);
    }

    if (m_pURL)
    {
        URL_free(&m_pURL);
    }

    if (m_pConnectionPool)
    {
        ConnectionPool_stop(m_pConnectionPool);
        ConnectionPool_free(&m_pConnectionPool);
    }
}

int NFCZDBDriver::Connect(uint32_t dbType, const std::string &strDBName, const std::string &strDBHost, const int nDBPort,
                            const std::string &strDBUser, const std::string &strDBPwd)
{
    mstrDBName = strDBName;
    mstrDBHost = strDBHost;
    mnDBPort = nDBPort;
    mstrDBUser = strDBUser;
    mstrDBPwd = strDBPwd;
    m_dbType = dbType;

    return Connect();
}

int NFCZDBDriver::CheckConnect()
{
    if (IsNeedReconnect() && CanReconnect())
    {
        Connect(m_dbType, mstrDBName, mstrDBHost, mnDBPort, mstrDBUser, mstrDBPwd);
    }

    return 0;
}

int NFCZDBDriver::Query(const std::string &qstr, std::vector<std::map<std::string, std::string>> &queryResult, std::string &errormsg)
{
    NFMYSQLTRYBEGIN

    auto pConection = GetConnection();
    CHECK_NULL(0, pConection);
    auto pResult = Connection_executeQuery(pConection, qstr.c_str());
    while(ResultSet_next(pResult))
    {
        int colNum = ResultSet_getColumnCount(pResult);
        std::map<std::string, std::string> oneColResult;
        for(int i = 1; i <= colNum; ++i)
        {
            const char* pColName = ResultSet_getColumnName(pResult, i);
            const char* pColValue = ResultSet_getString(pResult, i);

            if (pColName)
            {
                std::string colName(pColName);
                std::string colValue;
                if (pColValue)
                {
                    colValue = pColValue;
                }
                oneColResult[colName] = colValue;
            }
        }
        queryResult.emplace_back(oneColResult);
    }

    NFMYSQLTRYEND("Query Error")

    return 0;
}

int NFCZDBDriver::ExecuteOne(const std::string &qstr, std::map<std::string, std::string> &valueVec,
                               std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "ExecuteOne:{}", qstr);

    std::vector<std::map<std::string, std::string>> queryResult;
    if (NFCZDBDriver::Query(qstr, queryResult, errormsg) == 0)
    {
        for (size_t i = 0; i < (size_t)queryResult.size(); ++i)
        {
            valueVec.swap(queryResult[i]);
            return 0;
        }
        return 0;
    }
    return -1;
}

int NFCZDBDriver::Execute(const NFrame::storesvr_execute &select, NFrame::storesvr_execute_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", select.record());

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errormsg;
    int iRet = ExecuteMore(select.record(), resultVec, errormsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errormsg);
        return -1;
    }

    select_res.mutable_baseinfo()->CopyFrom(select.baseinfo());
    select_res.mutable_opres()->set_mod_key(select.mod_key());

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string> &result = resultVec[i];

        google::protobuf::Message *pMessage = NULL;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != NULL)
        {
            select_res.set_record(pMessage->SerializeAsString());
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} sql:{}",
                       NFCommon::tostr(result), select.record());
            iRet = -1;
        }

        if (pMessage != NULL)
        {
            NF_SAFE_DELETE(pMessage);
        }

        break;
    }

    return iRet;
}

int NFCZDBDriver::ExecuteMore(const NFrame::storesvr_execute_more &select,
                                ::google::protobuf::RepeatedPtrField<NFrame::storesvr_execute_more_res> &vecSelectRes)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errormsg;
    int iRet = ExecuteMore(select.record(), resultVec, errormsg);
    if (iRet != 0)
    {
        NFrame::storesvr_execute_more_res *select_res = vecSelectRes.Add();
        select_res->mutable_opres()->set_errmsg(errormsg);
        return -1;
    }

    NFrame::storesvr_execute_more_res *select_res = vecSelectRes.Add();

    select_res->mutable_baseinfo()->CopyFrom(select.baseinfo());
    select_res->mutable_opres()->set_mod_key(select.mod_key());
    select_res->set_is_lastbatch(false);

    int count = 0;
    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string> &result = resultVec[i];

        google::protobuf::Message *pMessage = NULL;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != NULL)
        {
            select_res->add_record(pMessage->SerializeAsString());

            count++;
            select_res->set_row_count(count);
            if ((int) select_res->record_size() >= (int) select.baseinfo().max_records())
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

        if (pMessage != NULL)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    select_res->set_is_lastbatch(true);

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::ExecuteMore(const NFrame::storesvr_execute_more &select, NFrame::storesvr_execute_more_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "query:{}", select.record());

    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errormsg;
    int iRet = ExecuteMore(select.record(), resultVec, errormsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errormsg);
        return -1;
    }

    select_res.mutable_baseinfo()->CopyFrom(select.baseinfo());
    select_res.mutable_opres()->set_mod_key(select.mod_key());

    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string> &result = resultVec[i];

        google::protobuf::Message *pMessage = NULL;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != NULL)
        {
            select_res.add_record(pMessage->SerializeAsString());
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} sql:{}",
                       NFCommon::tostr(result), select.record());
            iRet = -1;
        }

        if (pMessage != NULL)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    return iRet;
}

int NFCZDBDriver::ExecuteMore(const std::string &qstr, std::vector<std::map<std::string, std::string>> &valueVec,
                                std::string &errormsg)
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "query:{}", qstr);

    return NFCZDBDriver::Query(qstr, valueVec, errormsg);
}

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
int NFCZDBDriver::QueryDescStore(const std::string &table, google::protobuf::Message **pMessage)
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
    ::google::protobuf::Message *pSheetMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(sheet_fullname);
    CHECK_EXPR(pSheetMessageObject, -1, "{} New Failed", sheet_fullname);

    if (pMessage)
    {
        *pMessage = pSheetMessageObject;
    }

    const google::protobuf::Descriptor *pSheetFieldDesc = pSheetMessageObject->GetDescriptor();
    CHECK_EXPR(pSheetFieldDesc, -1, "pSheetFieldDesc == NULL");
    const google::protobuf::Reflection *pSheetReflect = pSheetMessageObject->GetReflection();
    CHECK_EXPR(pSheetReflect, -1, "pSheetFieldDesc == NULL");

    for (int sheet_field_index = 0; sheet_field_index < pSheetFieldDesc->field_count(); sheet_field_index++)
    {
        /*  比如 message Sheet_GameRoomDesc
        *		{
        *			repeated GameRoomDesc GameRoomDesc_List = 1  [(nanopb).max_count=100];
        *		}
        *		获得上面GameRoomDesc_List信息
        */
        const google::protobuf::FieldDescriptor *pSheetRepeatedFieldDesc = pSheetFieldDesc->field(sheet_field_index);
        if (pSheetRepeatedFieldDesc->is_repeated() &&
            pSheetRepeatedFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            //如果is_repeated 开始处理
            for (size_t result_i = 0; result_i < resultVec.size(); result_i++)
            {
                const std::map<std::string, std::string> &result = resultVec[result_i];
                ::google::protobuf::Message *pSheetRepeatedMessageObject = pSheetReflect->AddMessage(
                        pSheetMessageObject, pSheetRepeatedFieldDesc);
                NFProtobufCommon::GetDBMessageFromMapFields(result, pSheetRepeatedMessageObject);
            }
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::QueryDescStore(const std::string &table, google::protobuf::Message *pSheetMessageObject)
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

    const google::protobuf::Descriptor *pSheetFieldDesc = pSheetMessageObject->GetDescriptor();
    CHECK_EXPR(pSheetFieldDesc, -1, "pSheetFieldDesc == NULL");
    const google::protobuf::Reflection *pSheetReflect = pSheetMessageObject->GetReflection();
    CHECK_EXPR(pSheetReflect, -1, "pSheetFieldDesc == NULL");

    for (int sheet_field_index = 0; sheet_field_index < pSheetFieldDesc->field_count(); sheet_field_index++)
    {
        /*  比如 message Sheet_GameRoomDesc
        *		{
        *			repeated GameRoomDesc GameRoomDesc_List = 1  [(nanopb).max_count=100];
        *		}
        *		获得上面GameRoomDesc_List信息
        */
        const google::protobuf::FieldDescriptor *pSheetRepeatedFieldDesc = pSheetFieldDesc->field(sheet_field_index);
        if (pSheetRepeatedFieldDesc->is_repeated() &&
            pSheetRepeatedFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            //如果is_repeated 开始处理
            for (size_t result_i = 0; result_i < resultVec.size(); result_i++)
            {
                const std::map<std::string, std::string> &result = resultVec[result_i];
                ::google::protobuf::Message *pSheetRepeatedMessageObject = pSheetReflect->AddMessage(
                        pSheetMessageObject, pSheetRepeatedFieldDesc);
                NFProtobufCommon::GetDBMessageFromMapFields(result, pSheetRepeatedMessageObject);
            }
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::TransTableRowToMessage(const std::map<std::string, std::string> &result, const std::string &packageName, const std::string &className,
                                           google::protobuf::Message **pMessage)
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
    ::google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(proto_fullname);
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
NFCZDBDriver::SelectByCond(const NFrame::storesvr_sel &select, std::string &privateKey, std::unordered_set<std::string> &fields, std::unordered_set<std::string> &privateKeySet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = GetPrivateKeySql(select, privateKey, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    if (select.baseinfo().sel_fields_size() > 0)
    {
        for (int i = 0; i < (int) select.baseinfo().sel_fields_size(); i++)
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
        const std::map<std::string, std::string> &result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); iter++)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::SelectByCond(const std::string& packageName, const std::string& tableName, const std::string& className, const std::string &privateKey,
                                 const std::unordered_set<std::string> &leftPrivateKeySet,
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
        const std::map<std::string, std::string> &result = resultVec[i];

        google::protobuf::Message *pMessage = NULL;
        iRet = TransTableRowToMessage(result, packageName, className, &pMessage);
        auto iter = result.find(privateKey);
        if (iRet == 0 && pMessage != NULL && iter != result.end())
        {
            std::string record = pMessage->SerializeAsString();
            recordsMap.emplace(iter->second, record);
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                       NFCommon::tostr(result), tableName);
            iRet = -1;
        }

        if (pMessage != NULL)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::SelectByCond(const NFrame::storesvr_sel &select,
                                 ::google::protobuf::RepeatedPtrField<NFrame::storesvr_sel_res> &vecSelectRes)
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
        NFrame::storesvr_sel_res *select_res = vecSelectRes.Add();
        select_res->mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFrame::storesvr_sel_res *select_res = vecSelectRes.Add();

    select_res->mutable_baseinfo()->CopyFrom(select.baseinfo());
    select_res->mutable_opres()->set_mod_key(select.cond().mod_key());
    select_res->set_is_lastbatch(false);

    int count = 0;
    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string> &result = resultVec[i];

        google::protobuf::Message *pMessage = NULL;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != NULL)
        {
            select_res->add_record(pMessage->SerializeAsString());

            count++;
            select_res->set_row_count(count);
            if ((int) select_res->record_size() >= (int) select.baseinfo().max_records())
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

        if (pMessage != NULL)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    select_res->set_is_lastbatch(true);

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::GetPrivateKeySql(const NFrame::storesvr_sel &select, std::string &privateKey, std::string &selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");
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

        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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

int NFCZDBDriver::GetPrivateKey(const std::string packageName, const std::string &className, std::string &privateKey)
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

    const google::protobuf::Descriptor *pDescriptor = NFProtobufCommon::Instance()->FindDynamicMessageTypeByName(full_name);
    CHECK_EXPR(pDescriptor, -1, "NFProtobufCommon::FindDynamicMessageTypeByName:{} Failed", full_name);

    int iRet = NFProtobufCommon::GetPrivateKeyFromMessage(pDescriptor, privateKey);
    CHECK_EXPR(iRet == 0, -1, "NFProtobufCommon::GetPrivateKeyFromMessage:{} Failed", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::SelectByCond(const NFrame::storesvr_sel &select,
                                 NFrame::storesvr_sel_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::vector<std::map<std::string, std::string>> resultVec;
    std::string errmsg;
    iRet = ExecuteMore(selectSql, resultVec, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    select_res.set_is_lastbatch(true);

    int count = 0;
    for (size_t i = 0; i < resultVec.size(); i++)
    {
        const std::map<std::string, std::string> &result = resultVec[i];

        google::protobuf::Message *pMessage = NULL;
        iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
        if (iRet == 0 && pMessage != NULL)
        {
            count++;
            select_res.add_record(pMessage->SerializeAsString());
            NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                       NFCommon::tostr(result), select.baseinfo().tbname());
            iRet = -1;
        }
        if (pMessage != NULL)
        {
            NF_SAFE_DELETE(pMessage);
        }
    }

    select_res.set_row_count(count);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::SelectObj(const std::string &tbName, google::protobuf::Message *pMessage, std::string &errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- tbName:{} errMsg:{}", tbName, errMsg);
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_selobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializeAsString());

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

int NFCZDBDriver::SelectObj(const NFrame::storesvr_selobj &select,
                              NFrame::storesvr_selobj_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    iRet = CreateSql(select, keyMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", iRet);

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.mod_key());

    std::vector<std::string> vecFields;
    for (int i = 0; i < (int) select.baseinfo().sel_fields_size(); i++)
    {
        vecFields.push_back(select.baseinfo().sel_fields(i));
    }

    std::map<std::string, std::string> result;
    std::string errmsg;
    iRet = QueryOne(select.baseinfo().tbname(), keyMap, vecFields, result, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    google::protobuf::Message *pMessage = NULL;
    iRet = TransTableRowToMessage(result, select.baseinfo().package_name(), select.baseinfo().clname(), &pMessage);
    if (iRet == 0 && pMessage != NULL)
    {
        select_res.set_record(pMessage->SerializeAsString());
        NFLogTrace(NF_LOG_DEFAULT, 0, "{}", pMessage->Utf8DebugString());
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                   NFCommon::tostr(result), select.baseinfo().tbname());
        iRet = -1;
    }
    if (pMessage != NULL)
    {
        NF_SAFE_DELETE(pMessage);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::SelectObj(const std::string& packageName, const std::string& tbName, const std::string& className, const std::string& privateKey, const std::string& privateKeyValue, std::string& record)
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

    google::protobuf::Message *pMessage = NULL;
    iRet = TransTableRowToMessage(result, packageName, className, &pMessage);
    if (iRet == 0 && pMessage != NULL)
    {
        record = pMessage->SerializeAsString();
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "TransTableRowToMessage Failed, result:{} tableName:{}",
                   NFCommon::tostr(result), tbName);
        iRet = -1;
    }
    if (pMessage != NULL)
    {
        NF_SAFE_DELETE(pMessage);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::GetPrivateKeySql(const NFrame::storesvr_del &select, std::string &privateKey, std::string &selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");
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

        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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


int NFCZDBDriver::DeleteByCond(const NFrame::storesvr_del &select,
                                 NFrame::storesvr_del_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Delete(selectSql, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCZDBDriver::DeleteByCond(const NFrame::storesvr_del &select, std::string &privateKey, std::unordered_set<std::string> &privateKeySet)
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
        const std::map<std::string, std::string> &result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); iter++)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::DeleteByCond(const NFrame::storesvr_del &select, const std::string &privateKey,
                                 const std::unordered_set<std::string> &privateKeySet, NFrame::storesvr_del_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string selectSql;
    int iRet = 0;
    iRet = CreateSql(select, privateKey, privateKeySet, selectSql);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed:{}", selectSql);

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Delete(selectSql, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::DeleteObj(const NFrame::storesvr_delobj &select,
                              NFrame::storesvr_delobj_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    iRet = CreateSql(select, keyMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Delete(select.baseinfo().tbname(), keyMap, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::CreateSql(const NFrame::storesvr_delobj &select, std::map<std::string, std::string> &keyMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string className = select.baseinfo().clname();
    CHECK_EXPR(className.size() > 0, -1, "talbeName empty!");
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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
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

int NFCZDBDriver::CreateSql(const NFrame::storesvr_mod &select, std::string &selectSql)
{
    if (select.has_cond())
    {
        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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

        if (whereCond.where_additional_conds().size() > 0 && whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " " + whereCond.where_additional_conds();
        }
    }

    return 0;
}

int NFCZDBDriver::CreateSql(const NFrame::storesvr_update &select, std::string &selectSql)
{
    if (select.has_cond())
    {
        selectSql = " ";
        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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

int NFCZDBDriver::CreateSql(const NFrame::storesvr_del &select, const std::string &privateKey,
                              const std::unordered_set<std::string> &leftPrivateKeySet, std::string &selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");

    selectSql = "delete from " + tableName + " where ";

    for (auto iter = leftPrivateKeySet.begin(); iter != leftPrivateKeySet.end(); iter++)
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

int NFCZDBDriver::CreateSql(const NFrame::storesvr_del &select, std::string &selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");

    if (select.has_cond())
    {
        selectSql = "delete from " + tableName;
        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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
NFCZDBDriver::CreateSql(const NFrame::storesvr_selobj &select, std::map<std::string, std::string> &keyMap)
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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    NFProtobufCommon::GetDBMapFieldsFromMessage(*pMessageObject, keyMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::CreateSql(const std::string& tableName, const std::string &privateKey,
                              const std::unordered_set<std::string> &leftPrivateKeySet, std::string &selectSql)
{
    std::string stringFileds = "*";
    selectSql = "select " + stringFileds + " from " + tableName;

    if (leftPrivateKeySet.size() > 0)
    {
        selectSql += " where ";
    }

    for (auto iter = leftPrivateKeySet.begin(); iter != leftPrivateKeySet.end(); iter++)
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

int NFCZDBDriver::CreateSql(const NFrame::storesvr_sel &select, std::string &selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");

    std::string stringFileds = "*";
    if (select.baseinfo().sel_fields_size() > 0)
    {
        stringFileds = "";
        for (int i = 0; i < (int) select.baseinfo().sel_fields_size(); i++)
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

        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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

Connection_T NFCZDBDriver::GetConnection()
{
    return m_pCurConnection;
}

void NFCZDBDriver::CloseConnection(Connection_T pConn)
{
    Connection_close(pConn);
}

bool NFCZDBDriver::Enable()
{
    return !IsNeedReconnect();
}

bool NFCZDBDriver::CanReconnect()
{
    mfCheckReconnect += 0.1f;

    //30检查断线重连
    if (mfCheckReconnect < mnReconnectTime)
    {
        return false;
    }

    if (mnReconnectCount == 0)
    {
        return false;
    }

    mfCheckReconnect = 0.0f;

    return true;
}

int NFCZDBDriver::Reconnect()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Start Reconnect nServerID:{}, strIP:{}, nPort{}, strDBName:{}, strDBUser:{}, strDBPwd:{}",
              mstrDBName, mstrDBHost, mnDBPort, mstrDBName, mstrDBUser, mstrDBPwd);
    int iRet = Connect(m_dbType, mstrDBName, mstrDBHost, mnDBPort, mstrDBUser, mstrDBPwd);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0,
                   "Reconnect Failed!:nServerID:{}, strIP:{}, nPort{}, strDBName:{}, strDBUser:{}, strDBPwd:{}",
                   mstrDBName, mstrDBHost, mnDBPort, mstrDBName, mstrDBUser, mstrDBPwd);
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "Reconnect Success!:nServerID:{}, strIP:{}, nPort{}, strDBName:{}, strDBUser:{}, strDBPwd:{}",
                  mstrDBName, mstrDBHost, mnDBPort, mstrDBName, mstrDBUser, mstrDBPwd);
    }

    if (mnReconnectCount > 0)
    {
        mnReconnectCount--;
    }

    return 0;
}

bool NFCZDBDriver::IsNeedReconnect()
{
    //没有配置表
    if (mstrDBHost.length() < 1 || mstrDBUser.length() < 1)
    {
        return false;
    }

    return false;
}

int NFCZDBDriver::Connect()
{
    std::string errormsg;
    NFMYSQLTRYBEGIN

    if (m_dbType == NFrame::E_DBTYPE_SQLITE)
    {
        std::string userMsg = NF_FORMAT("sqlite:///tmp/{}.db", mstrDBName);
        m_pURL = URL_new(userMsg.c_str());
        CHECK_NULL(0, m_pURL);
        m_pConnectionPool = ConnectionPool_new(m_pURL);
    }
    else if (m_dbType == NFrame::E_DBTYPE_SQLITE)
    {
        std::string userMsg = NF_FORMAT("mysql:///tmp/{}.db", mstrDBName);
        m_pURL = URL_new(userMsg.c_str());
        CHECK_NULL(0, m_pURL);
        m_pConnectionPool = ConnectionPool_new(m_pURL);
    }

    ConnectionPool_start(m_pConnectionPool);
    m_pCurConnection = ConnectionPool_getConnection(m_pConnectionPool);
    CHECK_NULL(0, m_pCurConnection);

    NFMYSQLTRYEND("Connect faild")
    return 0;
}

int NFCZDBDriver::Disconnect()
{
    return 0;
}

int NFCZDBDriver::Update(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                           const std::map<std::string, std::string> &keyvalueMap,
                           std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    bool bExist = false;
    if (Exists(strTableName, keyMap, bExist) != 0)
    {
        return -1;
    }

    int iRet = 0;
    if (bExist)
    {
        iRet = Modify(strTableName, keyMap, keyvalueMap, errormsg);
    }
    else
    {
        std::map<std::string, std::string> insertMap = keyvalueMap;
        for (auto iter = keyMap.begin(); iter != keyMap.end(); iter++)
        {
            insertMap.emplace(iter->first, iter->second);
        }
        iRet = Insert(strTableName, insertMap, errormsg);
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::Modify(const std::string &strTableName, const std::string &where,
                           const std::map<std::string, std::string> &keyvalueMap, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        // update
        query << "UPDATE " << strTableName << " SET ";
        int i = 0;
        for (auto iter = keyvalueMap.begin(); iter != keyvalueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << "," << iter->first << " = " << "'" << iter->second;
            }
            i++;
        }

        if (!where.empty())
        {
            query << " WHERE " << where;
        }
        query << ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        Connection_execute(pConnection, query.str().c_str());
    NFMYSQLTRYEND("modify error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Modify(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                           const std::map<std::string, std::string> &keyvalueMap, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        // update
        query << "UPDATE " << strTableName << " SET ";
        int i = 0;
        for (auto iter = keyvalueMap.begin(); iter != keyvalueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << "," << iter->first << " = " << "'" << iter->second;
            }
            i++;
        }

        query << " WHERE ";
        i = 0;
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << "'" << iter->second;
            }
            i++;
        }
        query << ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        Connection_execute(pConnection, query.str().c_str());
    NFMYSQLTRYEND("modify error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Insert(const std::string &strTableName, const std::map<std::string, std::string> &keyvalueMap,
                           std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        // insert
        query << "INSERT INTO " << strTableName << "(";
        int i = 0;
        for (auto iter = keyvalueMap.begin(); iter != keyvalueMap.end(); ++iter)
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
        for (auto iter = keyvalueMap.begin(); iter != keyvalueMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << "'" << iter->second;
            }
            else
            {
                query << ", " << "'" << iter->second;
            }
            i++;
        }

        query << ");";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        Connection_execute(pConnection, query.str().c_str());
    NFMYSQLTRYEND("error")
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::QueryOne(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                             std::map<std::string, std::string> &valueVec, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    valueVec.clear();
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        query << "SELECT * ";

        query << " FROM " << strTableName << " WHERE ";
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (iter == keyMap.begin())
            {
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << "'" << iter->second;
            }
        }
        query << " limit 1;";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        std::vector<std::map<std::string, std::string>> queryResult;
        Query(query.str(), queryResult, errormsg);

        if (queryResult.empty())
        {
            return NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
        }

        valueVec.swap(queryResult[0]);
    NFMYSQLTRYEND("query error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::QueryOne(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                             const std::vector<std::string> &fieldVec,
                             std::map<std::string, std::string> &valueVec, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    valueVec.clear();
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
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
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << "'" << iter->second;
            }
        }
        query << " limit 1;";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        std::vector<std::map<std::string, std::string>> queryResult;
        Query(query.str(), queryResult, errormsg);

        if (queryResult.empty())
        {
            return NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
        }

        valueVec.swap(queryResult[0]);
    NFMYSQLTRYEND("query error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::QueryMore(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                              const std::vector<std::string> &fieldVec,
                              std::vector<std::map<std::string, std::string>> &valueVec, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    valueVec.clear();
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
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
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << "'" << iter->second;
            }
        }
        query << ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        Query(query.str(), valueVec, errormsg);

        if (valueVec.empty())
        {
            return NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
        }
    NFMYSQLTRYEND("query error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Delete(const std::string &strTableName, const std::map<std::string, std::string> &keyMap, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        query << "DELETE FROM " << strTableName << " WHERE ";
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (iter == keyMap.begin())
            {
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << "'" << iter->second;
            }
        }
        query << ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        Connection_execute(pConnection, query.str().c_str());

    NFMYSQLTRYEND("delete error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Delete(const std::string &strTableName, const std::string &strKeyColName,
                           const std::string &strKey, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        query << "DELETE FROM " << strTableName << " WHERE " << strKeyColName << " = " << "'" << strKey <<
              "';";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        Connection_execute(pConnection, query.str().c_str());

    NFMYSQLTRYEND("delete error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Delete(const std::string &sql, std::string &errormsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    NFMYSQLTRYBEGIN
        std::stringstream query;
        query << sql << ";";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());
        Connection_execute(pConnection, query.str().c_str());
    NFMYSQLTRYEND("delete error")

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Exists(const std::string &strTableName, const std::map<std::string, std::string> &keyMap,
                           bool &bExit)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    if (keyMap.empty())
    {
        bExit = false;
        return 0;
    }

    std::string errormsg;
    NFMYSQLTRYBEGIN
        std::stringstream query;
        query << "SELECT 1 FROM " << strTableName << " WHERE ";
        int i = 0;
        for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
        {
            if (i == 0)
            {
                query << iter->first << " = " << "'" << iter->second;
            }
            else
            {
                query << " and " << iter->first << " = " << "'" << iter->second;
            }
            i++;
        }
        query << " limit 1;";

        NFLogDebug(NF_LOG_DEFAULT, 0, "query:{}", query.str());

        std::vector<std::map<std::string, std::string>> queryResult;
        Query(query.str(), queryResult, errormsg);

        if (queryResult.empty())
        {
            bExit = false;
            return 0;
        }

    NFMYSQLTRYEND("exist error")

    bExit = true;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::Exists(const std::string &strTableName, const std::string &strKeyColName,
                           const std::string &strKey, bool &bExit)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    std::string errormsg;
    NFMYSQLTRYBEGIN
        std::stringstream query;
        query << "SELECT 1 FROM " << strTableName << " WHERE " << strKeyColName << " = " << "'" << strKey <<
              "' LIMIT 1;";

        std::vector<std::map<std::string, std::string>> queryResult;
        Query(query.str(), queryResult, errormsg);

        if (queryResult.empty())
        {
            bExit = false;
            return 0;
        }

    NFMYSQLTRYEND("exist error")

    bExit = true;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::InsertObj(const std::string &tbName, const google::protobuf::Message *pMessage, std::string &errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_insertobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializeAsString());

    NFrame::storesvr_insertobj_res select_res;
    int iRet = InsertObj(select, select_res);
    if (iRet != 0)
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::InsertObj(const NFrame::storesvr_insertobj &select,
                              NFrame::storesvr_insertobj_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> resultMap;
    iRet = CreateSql(select, resultMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Insert(select.baseinfo().tbname(), resultMap, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCZDBDriver::CreateSql(const NFrame::storesvr_insertobj &select, std::map<std::string, std::string> &resultMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");

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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    NFProtobufCommon::GetDBMapFieldsFromMessage(*pMessageObject, resultMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::ModifyObj(const std::string &tbName, const google::protobuf::Message *pMessage, std::string &errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_modobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializeAsString());

    NFrame::storesvr_modobj_res select_res;
    int iRet = ModifyObj(select, select_res);
    if (iRet != 0)
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::GetPrivateKeySql(const NFrame::storesvr_mod &select, std::string &privateKey, std::string &selectSql)
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

        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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

int NFCZDBDriver::UpdateByCond(const NFrame::storesvr_update &select, std::string &privateKey, std::unordered_set<std::string> &privateKeySet)
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
        const std::map<std::string, std::string> &result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); iter++)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::ModifyByCond(const NFrame::storesvr_mod &select, std::string &privateKey, std::unordered_set<std::string> &privateKeySet)
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
        const std::map<std::string, std::string> &result = resultVec[i];
        for (auto iter = result.begin(); iter != result.end(); iter++)
        {
            CHECK_EXPR(iter->first == privateKey, -1, "");
            privateKeySet.insert(iter->second);
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::ModifyByCond(const NFrame::storesvr_mod &select, NFrame::storesvr_mod_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    std::string where;
    CreateSql(select, where);

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Modify(select.baseinfo().tbname(), where, keyValueMap, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}


int NFCZDBDriver::ModifyObj(const NFrame::storesvr_modobj &select,
                              NFrame::storesvr_modobj_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Modify(select.baseinfo().tbname(), keyMap, keyValueMap, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::CreateSql(const NFrame::storesvr_mod &select, std::map<std::string, std::string> &keyMap,
                              std::map<std::string, std::string> &kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");
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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "CreateSql From message:{}", pMessageObject->DebugString());

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::CreateSql(const NFrame::storesvr_update &select, std::map<std::string, std::string> &keyMap,
                              std::map<std::string, std::string> &kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");
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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "CreateSql From message:{}", pMessageObject->DebugString());

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::CreateSql(const NFrame::storesvr_modobj &select, std::map<std::string, std::string> &keyMap,
                              std::map<std::string, std::string> &kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");
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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);
    NFLogTrace(NF_LOG_DEFAULT, 0, "CreateSql From message:{}", pMessageObject->DebugString());

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::UpdateObj(const std::string &tbName, const google::protobuf::Message *pMessage, std::string &errMsg)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(pMessage, -1, "pMessage == NULL");

    NFrame::storesvr_updateobj select;
    select.mutable_baseinfo()->set_tbname(tbName);
    select.set_record(pMessage->SerializeAsString());

    NFrame::storesvr_updateobj_res select_res;
    int iRet = UpdateObj(select, select_res);
    if (iRet != 0)
    {
        errMsg = select_res.opres().errmsg();
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCZDBDriver::GetPrivateKeySql(const NFrame::storesvr_update &select, std::string &privateKey, std::string &selectSql)
{
    std::string tableName = select.baseinfo().tbname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");

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

        const ::NFrame::storesvr_wherecond &whereCond = select.cond();
        if (whereCond.where_conds_size() > 0 || whereCond.where_additional_conds().size() > 0)
        {
            selectSql += " where ";
        }
        for (int i = 0; i < whereCond.where_conds_size(); i++)
        {
            std::string sql;
            const ::NFrame::storesvr_vk &vk = whereCond.where_conds(i);
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

int NFCZDBDriver::UpdateByCond(const NFrame::storesvr_update &select, NFrame::storesvr_update_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    std::string where;
    CreateSql(select, where);

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.cond().mod_key());
    std::string errmsg;
    iRet = Modify(select.baseinfo().tbname(), where, keyValueMap, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::UpdateObj(const NFrame::storesvr_updateobj &select,
                              NFrame::storesvr_updateobj_res &select_res)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::map<std::string, std::string> keyMap;
    std::map<std::string, std::string> keyValueMap;
    iRet = CreateSql(select, keyMap, keyValueMap);
    CHECK_EXPR(iRet == 0, -1, "CreateSql Failed");

    *select_res.mutable_baseinfo() = select.baseinfo();
    select_res.mutable_opres()->set_mod_key(select.mod_key());
    std::string errmsg;
    iRet = Update(select.baseinfo().tbname(), keyMap, keyValueMap, errmsg);
    if (iRet != 0)
    {
        select_res.mutable_opres()->set_errmsg(errmsg);
        return iRet;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int
NFCZDBDriver::CreateSql(const NFrame::storesvr_updateobj &select, std::map<std::string, std::string> &keyMap,
                          std::map<std::string, std::string> &kevValueMap)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string tableName = select.baseinfo().clname();
    CHECK_EXPR(tableName.size() > 0, -1, "talbeName empty!");
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
    google::protobuf::Message *pMessageObject = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pMessageObject, -1, "NFProtobufCommon::CreateMessageByName:{} Failed", full_name);
    CHECK_EXPR(pMessageObject->ParsePartialFromString(select.record()), -1, "ParsePartialFromString Failed:{}", full_name);

    NFProtobufCommon::GetMapDBFieldsFromMessage(*pMessageObject, keyMap, kevValueMap);
    delete pMessageObject;
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCZDBDriver::ExistsDB(const std::string &dbName, bool &bExit)
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
int NFCZDBDriver::CreateDB(const std::string &dbName)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string errormsg;
    auto pConnection = GetConnection();
    CHECK_NULL(0, pConnection);

    int iRet = 0;
    NFMYSQLTRYBEGIN
    std::stringstream query;
    query << "CREATE DATABASE `" << dbName << "`;";
    Connection_execute(pConnection, query.str().c_str());
    NFMYSQLTRYEND("error")
    return iRet;

    return 0;
}

/**
 * @brief 选择数据库
 * @param dbName
 * @return
 */
int NFCZDBDriver::SelectDB(const std::string &dbName)
{
    return 0;
}

/**
 * @brief 是否存在表格
 * @param tableName
 * @return
 */
int NFCZDBDriver::ExistTable(const std::string &dbName, const std::string &tableName, bool &bExit)
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


int NFCZDBDriver::GetTableColInfo(const std::string &dbName, const std::string &tableName, std::map<std::string, DBTableColInfo> &col)
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

    for (int i = 0; i < (int) valueVec.size(); i++)
    {
        std::map<std::string, std::string> &colData = valueVec[i];
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

/**
 * @brief 查询表格信息
 * @param tableName
 * @param pTableMessage
 * @param needCreateColumn
 * @return
 */
int NFCZDBDriver::QueryTableInfo(const std::string &dbName, const std::string &tableName, bool &bExit,
                                   std::map<std::string, DBTableColInfo> &primaryKey, std::multimap<uint32_t, std::string> &needCreateColumn)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = ExistTable(dbName, tableName, bExit);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "QueryTableInfo Error, dbName:{}, tableName:{}", dbName, tableName);
        return iRet;
    }

    std::string full_name = DEFINE_DEFAULT_PROTO_PACKAGE_ADD + tableName;
    const google::protobuf::Descriptor *pDescriptor = NFProtobufCommon::Instance()->FindDynamicMessageTypeByName(full_name);
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

        for (auto iter = primaryKey.begin(); iter != primaryKey.end(); iter++)
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

    for (auto iter = mapFields.begin(); iter != mapFields.end(); iter++)
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

int NFCZDBDriver::CreateTable(const std::string &tableName, const std::map<std::string, DBTableColInfo> &primaryKey,
                                const std::multimap<uint32_t, std::string> &needCreateColumn)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;
    std::string sql;
    std::string colSql;
    std::string privateKey = "PRIMARY KEY(";
    std::string auto_increment;

    for (auto iter = primaryKey.begin(); iter != primaryKey.end(); iter++)
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

    for (auto iter = needCreateColumn.begin(); iter != needCreateColumn.end(); iter++)
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

int NFCZDBDriver::AddTableRow(const std::string &tableName, const std::multimap<uint32_t, std::string> &needCreateColumn)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    int iRet = 0;

    std::map<std::string, std::string> mapValue;
    std::string errMsg;
    for (auto iter = needCreateColumn.begin(); iter != needCreateColumn.end(); iter++)
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