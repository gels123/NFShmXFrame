// -------------------------------------------------------------------------
//    @FileName         :    NFStoreProtoCommon.cpp
//    @Author           :    gaoyi
//    @Date             :    23-3-28
//    @Email			:    445267987@qq.com
//    @Module           :    NFStoreProtoCommon
//
// -------------------------------------------------------------------------

#include "NFStoreProtoCommon.h"
#include "NFProtobufCommon.h"
#include "NFCheck.h"

std::string NFStoreProtoCommon::storesvr_selectbycond(const std::string& dbname, const std::string& tbname,
                                                      uint64_t mod_key, const std::vector<std::string>& fields,
                                                      const std::vector<NFrame::storesvr_vk>& vk_list,
                                                      const std::string& additional_conds/* = ""*/, int maxRecords/* = 100*/,
                                                      const std::string& cls_name/* = ""*/, const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_sel select;
    storesvr_selectbycond(select, dbname, tbname, mod_key, fields, vk_list, additional_conds, maxRecords, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_selectbycond(NFrame::storesvr_sel& select, const std::string& dbname, const std::string& tbname,
                                               uint64_t mod_key, const std::vector<std::string>& fields,
                                               const std::vector<NFrame::storesvr_vk>& vk_list,
                                               const std::string& additional_conds/* = ""*/, int maxRecords/* = 100*/,
                                               const std::string& cls_name/* = ""*/, const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.mutable_baseinfo()->set_package_name(package_name);
    for (int i = 0; i < (int)fields.size(); i++)
    {
        select.mutable_baseinfo()->add_sel_fields(fields[i]);
    }
    select.mutable_baseinfo()->set_max_records(maxRecords);

    select.mutable_cond()->set_mod_key(mod_key);

    select.mutable_cond()->set_where_additional_conds(additional_conds);
    for (size_t i = 0; i < vk_list.size(); i++)
    {
        ::NFrame::storesvr_vk* pvk = select.mutable_cond()->add_where_conds();
        *pvk = vk_list[i];
    }
}

std::string NFStoreProtoCommon::storesvr_selectbycond(const std::string& dbname, const std::string& tbname, uint64_t mod_key, const std::vector<std::string>& fields, const std::vector<std::string>& privateKeys, int maxRecords, const std::string& cls_name, const std::string& package_name)
{
    NFrame::storesvr_sel select;
    storesvr_selectbycond(select, dbname, tbname, mod_key, fields, privateKeys, maxRecords, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_selectbycond(NFrame::storesvr_sel& select, const std::string& dbname, const std::string& tbname, uint64_t mod_key, const std::vector<std::string>& fields, const std::vector<std::string>& privateKeys, int maxRecords, const std::string& cls_name,
                                               const std::string& package_name)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.mutable_baseinfo()->set_package_name(package_name);
    for (int i = 0; i < (int)fields.size(); i++)
    {
        select.mutable_baseinfo()->add_sel_fields(fields[i]);
    }
    select.mutable_baseinfo()->set_max_records(maxRecords);

    select.mutable_cond()->set_mod_key(mod_key);
    for (size_t i = 0; i < privateKeys.size(); i++)
    {
        select.mutable_cond()->add_private_keys(privateKeys[i]);
    }
}

void NFStoreProtoCommon::storesvr_selectobj(NFrame::storesvr_selobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/,
                                            const std::vector<std::string>& vecFields/* = std::vector<std::string>()*/)
{
    storesvr_selectobj(select, dbname, tbname, mod_key, msg_obj.SerializePartialAsString(), cls_name, package_name, vecFields);
}

void NFStoreProtoCommon::storesvr_selectobj(NFrame::storesvr_selobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key, const std::string& msgObjStr, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/,
                                            const std::vector<std::string>& vecFields/* = std::vector<std::string>()*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    if (tbname.empty())
    {
        select.mutable_baseinfo()->set_tbname(cls_name);
    }
    else
    {
        select.mutable_baseinfo()->set_tbname(tbname);
    }
    select.set_mod_key(mod_key);
    select.set_record(msgObjStr);
    if (vecFields.size() > 0)
    {
        for (int i = 0; i < (int)vecFields.size(); i++)
        {
            select.mutable_baseinfo()->add_sel_fields(vecFields[i]);
        }
    }
}

// select对象查询，返回打包数据，该数据可直接网络发送
std::string NFStoreProtoCommon::storesvr_selectobj(const std::string& dbname, const std::string& tbname,
                                                   uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                                   const std::string& package_name/* = ""*/,
                                                   const std::vector<std::string>& vecFields/* = std::vector<std::string>()*/)
{
    NFrame::storesvr_selobj select;
    storesvr_selectobj(select, dbname, tbname, mod_key, msg_obj, cls_name, package_name, vecFields);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_insertobj(NFrame::storesvr_insertobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key,
                                            const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/)
{
    storesvr_insertobj(select, dbname, tbname, mod_key, msg_obj.SerializePartialAsString(), cls_name, package_name);
}

void NFStoreProtoCommon::storesvr_insertobj(NFrame::storesvr_insertobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key,
                                            const std::string& msgObjStr, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.set_mod_key(mod_key);
    select.set_record(msgObjStr);
}

// insert对象插入，返回打包数据
std::string NFStoreProtoCommon::storesvr_insertobj(const std::string& dbname, const std::string& tbname,
                                                   uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                                   const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_insertobj select;
    storesvr_insertobj(select, dbname, tbname, mod_key, msg_obj, cls_name, package_name);
    return select.SerializePartialAsString();
}

// 按条件删除
std::string NFStoreProtoCommon::storesvr_deletebycond(const std::string& dbname, const std::string& tbname,
                                                      uint64_t mod_key, const std::vector<NFrame::storesvr_vk>& vk_list,
                                                      const std::string& additional_conds /*= ""*/, const std::string& cls_name/* = ""*/,
                                                      const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_del select;
    storesvr_deletebycond(select, dbname, tbname, mod_key, vk_list, additional_conds, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_deletebycond(NFrame::storesvr_del& select, const std::string& dbname, const std::string& tbname,
                                               uint64_t mod_key, const std::vector<NFrame::storesvr_vk>& vk_list,
                                               const std::string& additional_conds /*= ""*/, const std::string& cls_name/* = ""*/,
                                               const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.mutable_cond()->set_mod_key(mod_key);
    select.mutable_cond()->set_where_additional_conds(additional_conds);
    for (size_t i = 0; i < vk_list.size(); i++)
    {
        ::NFrame::storesvr_vk* pvk = select.mutable_cond()->add_where_conds();
        *pvk = vk_list[i];
    }
}

// 按对象删除
std::string NFStoreProtoCommon::storesvr_deleteobj(const std::string& dbname, const std::string& tbname,
                                                   uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                                   const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_delobj select;
    storesvr_deleteobj(select, dbname, tbname, mod_key, msg_obj, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_deleteobj(NFrame::storesvr_delobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.set_mod_key(mod_key);
    select.set_record(msg_obj.SerializePartialAsString());
}

std::string NFStoreProtoCommon::storesvr_modifybycond(const std::string& dbname, const std::string& tbname,
                                                      uint64_t mod_key, const ::google::protobuf::Message& msg_obj,
                                                      const std::vector<NFrame::storesvr_vk>& vk_list,
                                                      const std::string& additional_conds/* = ""*/, const std::string& cls_name/* = ""*/,
                                                      const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_mod select;
    storesvr_modifybycond(select, dbname, tbname, mod_key, msg_obj, vk_list, additional_conds, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_modifybycond(NFrame::storesvr_mod& select, const std::string& dbname, const std::string& tbname,
                                               uint64_t mod_key, const ::google::protobuf::Message& msg_obj,
                                               const std::vector<NFrame::storesvr_vk>& vk_list,
                                               const std::string& additional_conds/* = ""*/, const std::string& cls_name/* = ""*/,
                                               const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }

    select.mutable_cond()->set_mod_key(mod_key);

    select.mutable_cond()->set_where_additional_conds(additional_conds);
    for (size_t i = 0; i < vk_list.size(); i++)
    {
        ::NFrame::storesvr_vk* pvk = select.mutable_cond()->add_where_conds();
        *pvk = vk_list[i];
    }

    select.set_record(msg_obj.SerializePartialAsString());
}

// 按对象修改
std::string NFStoreProtoCommon::storesvr_modifyobj(const std::string& dbname, const std::string& tbname,
                                                   uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                                   const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_modobj select;
    storesvr_modifyobj(select, dbname, tbname, mod_key, msg_obj, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_modifyobj(NFrame::storesvr_modobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.set_mod_key(mod_key);
    select.set_record(msg_obj.SerializePartialAsString());
}


std::string NFStoreProtoCommon::storesvr_updatebycond(const std::string& dbname, const std::string& tbname,
                                                      uint64_t mod_key, const ::google::protobuf::Message& msg_obj,
                                                      const std::vector<NFrame::storesvr_vk>& vk_list,
                                                      const std::string& additional_conds/* = ""*/, const std::string& cls_name/* = ""*/,
                                                      const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_update select;
    storesvr_updatebycond(select, dbname, tbname, mod_key, msg_obj, vk_list, additional_conds, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_updatebycond(NFrame::storesvr_update& select, const std::string& dbname, const std::string& tbname,
                                               uint64_t mod_key, const ::google::protobuf::Message& msg_obj,
                                               const std::vector<NFrame::storesvr_vk>& vk_list,
                                               const std::string& additional_conds/* = ""*/, const std::string& cls_name/* = ""*/,
                                               const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }

    select.mutable_cond()->set_mod_key(mod_key);

    select.mutable_cond()->set_where_additional_conds(additional_conds);
    for (size_t i = 0; i < vk_list.size(); i++)
    {
        ::NFrame::storesvr_vk* pvk = select.mutable_cond()->add_where_conds();
        *pvk = vk_list[i];
    }

    select.set_record(msg_obj.SerializePartialAsString());
}

// 修改插入
std::string NFStoreProtoCommon::storesvr_updateobj(const std::string& dbname, const std::string& tbname,
                                                   uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                                   const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_updateobj select;
    storesvr_updateobj(select, dbname, tbname, mod_key, msg_obj, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_updateobj(NFrame::storesvr_updateobj& select, const std::string& dbname, const std::string& tbname,
                                            uint64_t mod_key, const ::google::protobuf::Message& msg_obj, const std::string& cls_name/* = ""*/,
                                            const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.set_mod_key(mod_key);
    select.set_record(msg_obj.SerializePartialAsString());
}

// 按对象修改
std::string NFStoreProtoCommon::storesvr_execute(const std::string& dbname, const std::string& tbname,
                                                 uint64_t mod_key, const std::string& msg, const std::string& cls_name/* = ""*/,
                                                 const std::string& package_name/* = ""*/)
{
    NFrame::storesvr_execute select;
    storesvr_execute(select, dbname, tbname, mod_key, msg, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_execute(NFrame::storesvr_execute& select, const std::string& dbname, const std::string& tbname,
                                          uint64_t mod_key, const std::string& msg, const std::string& cls_name/* = ""*/,
                                          const std::string& package_name/* = ""*/)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.set_mod_key(mod_key);
    select.set_record(msg + ";");
}

// 按对象修改
std::string NFStoreProtoCommon::storesvr_execute_more(const std::string& dbname, const std::string& tbname,
                                                      uint64_t mod_key, const std::string& msg, int max_records, const std::string& cls_name, const std::string& package_name)
{
    NFrame::storesvr_execute_more select;
    storesvr_execute_more(select, dbname, tbname, mod_key, msg, max_records, cls_name, package_name);
    return select.SerializePartialAsString();
}

void NFStoreProtoCommon::storesvr_execute_more(NFrame::storesvr_execute_more& select, const std::string& dbname, const std::string& tbname,
                                               uint64_t mod_key, const std::string& msg, int max_records, const std::string& cls_name,
                                               const std::string& package_name)
{
    select.mutable_baseinfo()->set_dbname(dbname);
    select.mutable_baseinfo()->set_tbname(tbname);
    select.mutable_baseinfo()->set_package_name(package_name);
    if (cls_name.empty())
    {
        select.mutable_baseinfo()->set_clname(tbname);
    }
    else
    {
        select.mutable_baseinfo()->set_clname(cls_name);
    }
    select.mutable_baseinfo()->set_max_records(max_records);
    select.set_mod_key(mod_key);
    select.set_record(msg + ";");
}

int NFStoreProtoCommon::get_proto_field_type(const google::protobuf::FieldDescriptor& fieldDesc)
{
    switch (fieldDesc.cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            return NFrame::E_COLUMNTYPE_NUM;
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            return NFrame::E_COLUMNTYPE_STRING;
        }
        break;
        default:
            break;
    }
    return NFrame::E_COLUMNTYPE_NUM;
}

int NFStoreProtoCommon::get_vk_list_from_proto(const google::protobuf::Message& message, std::vector<NFrame::storesvr_vk>& vk_list)
{
    std::map<std::string, std::pair<int, std::string>> keyMap;

    const google::protobuf::Descriptor* pDesc = message.GetDescriptor();
    CHECK_NULL(0, pDesc);

    const google::protobuf::Reflection* pReflect = message.GetReflection();
    CHECK_NULL(0, pReflect);

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->options().GetExtension(nanopb).db_no_field()) continue;
        if (!pFieldDesc->is_repeated() && pReflect->HasField(message, pFieldDesc) == false) continue;
        if (pFieldDesc->is_repeated() && pReflect->FieldSize(message, pFieldDesc) == 0) continue;

        if (pFieldDesc->is_repeated() == false)
        {
            keyMap.emplace(pFieldDesc->name(),
                           std::make_pair((int)get_proto_field_type(*pFieldDesc), NFProtobufCommon::GetFieldsString(message, pFieldDesc)));
        }
    }

    for (auto iter = keyMap.begin(); iter != keyMap.end(); ++iter)
    {
        NFrame::storesvr_vk cmd1;
        cmd1.set_column_name(iter->first);
        cmd1.set_column_value(iter->second.second);
        cmd1.set_column_type((NFrame::storesvr_column_type)iter->second.first);
        cmd1.set_cmp_operator(NFrame::E_CMPOP_EQUAL);

        auto temp_iter = iter;
        temp_iter++;
        if (temp_iter != keyMap.end())
        {
            cmd1.set_logic_operator(NFrame::E_LOGICOP_AND);
        }

        vk_list.push_back(cmd1);
    }
    return 0;
}
