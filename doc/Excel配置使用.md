### 在list.txt 文件里，加入你需要解析的excel. 
    可以参考game/TestGame目录下的list.txt
### 每一个excel，会有一个main,list的sheet. main主要给策划用来写注释，与程序无关。
    可以参考game/TestGame目录下的Test.xlsx

| main | list | Test | sheet2 |
|------|------|------|--------|

### list的第一列记录需要使用程序解析的sheet.

| Test |
|--------|
|  |
|        |
|        |
### 每一个需要解析的sheet, 前面四行是给解析器使用。用来配置sheet的解析，以及生成对应的proto.
    enum AttrType
    {
      EN_ATTR_MIN = 0; //最小值
      EN_ATTR_LEVEL = 1[(nanopb_enumvopt).macro_name = "等级"]; //等级
      EN_ATTR_STR = 2[(nanopb_enumvopt).macro_name = "力量"]; //力量
      EN_ATTR_MAG = 3[(nanopb_enumvopt).macro_name = "念力"]; //念力
      EN_ATTR_VIT = 4[(nanopb_enumvopt).macro_name = "耐力"]; //耐力
    };

| id    | name     | start_date | strength_level | enumAttr_ch            | enumAttr_en            | enumAttr_number        |
|-------|----------|-----------|----------------|------------------------|------------------------|------------------------|
| 编号    | 名字     | start_date | 强化等级        | enumAttr_ch            | enumAttr_en            | enumAttr_number        |
| int   | string32 | date      | int8           | enum:proto_ff.AttrType | enum:proto_ff.AttrType | enum:proto_ff.AttrType |
| 3     | 3        | 3         | 3              | 3                      | 3                      | 3                      |
| 10001 | 武器      | 2025-01-30 12:23:54          | 1              | 等级                    | EN_ATTR_LEVEL          | 1                      |

    上面的sheet最终将被生成一个proto和一个可以使用proto来解析的二进制bin文件,以及一个bin对应的json文件，json文件只是方便查看转化后的数据对不对。
    message E_TestTest
    {
        optional int32 id = 1[(nanopb).field_cname = "编号"];
        optional string name = 2[(nanopb).field_cname = "名字", (nanopb).max_size_enum = "MAX_STRING_LEN_32"];
	    optional string start_date = 3[(nanopb).field_cname = "start_date", (nanopb).max_size_enum = "MAX_STRING_LEN_64"];
	    optional uint64 start_date_t = 4[(nanopb).field_cname = "start_date"];
        optional int32 strength_level = 5[(nanopb).int_size = IS_8, (nanopb).field_cname = "强化等级"];
	    optional proto_ff.AttrType enumAttr_ch = 6[(nanopb).field_cname = "enumAttr_ch"];
	    optional proto_ff.AttrType enumAttr_en = 7[(nanopb).field_cname = "enumAttr_ch"];
	    optional proto_ff.AttrType enumAttr_number = 8[(nanopb).field_cname = "enumAttr_ch"];
    };
    E_TestTest 第一个Test是excel名字， 第二个Test是sheet名字


### 第一行是英文定义
    第一列英文定义，用来生成proto的field, 不可以重名，最好大小写都不要重复。

### 第二行是中文定义
    第一列中文定义，用来生成注释。方便理解
### 第三行是数据类型
    目前支持的类型：
    整数:int8,uint8,int16,uint16,int,uint,int32,uint32,int64,uint64
    布尔值:bool
    浮点数:float,double
    字符串：string,string32,string64,string128,string256,string512
    时间格式:date,值必须是时间格式字符串, 类似"2025-01-30 12:23:54"
    enum类型:使用proto公共定义的enum类型来定义一列。有方便策划，同时自动检查数值填写的对不对的作用
            格式:enum:proto定义，比如上面的enum:proto_ff.AttrType. proto_ff.AttrType定义在proto_common.proto里
            比如上面的enumAttr_ch,enumAttr_en,enumAttr_number. 这一列的值有可以有三种填法。
            想enumAttr_ch，策划填，不容易出错，填值的中文:等级 
            像enumAttr_en,填英文字符串:EN_ATTR_LEVEL
            想enumAttr_number, 直接填数字:1
    其他特殊用法，后面会介绍到.
    struct类型:更加负责的struct类型。直接使用proto定义好的一个message.数据填json/xml/lua文本，可以直接转化成这个json
    
### 第四行是客户端、服务器解析，0表示不解析，1客户端需要，2客户端.服务器需要。3.服务器需要。

### 其他特殊类型介绍, 单个复杂数据结构体:
| id    | pos.x | pos.y | attr.attr              | attr.value | jsonItemOne                     |
|-------|-------|-------|------------------------|------------|---------------------------------|
| 编号   | 位置.x  | 位置.y  | 附加属性.属性          | 附加属性.数值 | jsonItemOne                     |
| int   | int   | int   | enum:proto_ff.AttrType | float      | struct:proto_ff.Item:json       |
| 3     | 3     | 3     | 3                      | 3          | 3                               |
| 10001 | 30    | 100   | 等级                    | 1.11       | {"id":1,"value":2,"quality":3}  |

    单个复杂数据结构体的表达方式:
        1.自动生成结构体, 比如上面的pos.x, pos.y. 利用英文中的.点号隔开，将自动生成结构体。
            message E_TestTestPos
            {
                optional int32 x = 1[(nanopb).field_cname = "x"];
	            optional int32 y = 2[(nanopb).field_cname = "y"];
            };
            对应E_TestTest的field:
                optional E_TestPos pos = 10[(nanopb).field_cname = "位置"];
       2.利用现有结构体定义，格式同上，但是在excel.json文件中需要制定,比如"colMessageType": "attr_list:AttrPair;",指定attr_list，使用AttrPair解析。而不是自动生成结构体。方便很多结构体共用。
            {
                "excel": [
                    {
                        "name": "Test",
                        "sheet": [
                            {
                                "name": "Test",
                                "unique_index": "",
                                "multi_index": "",
                                "relation": "",
                                "createSql": true,
                                "field_enum": "",
                                "import_proto": "proto_common.proto",
                                "colMessageType": "attr:AttrPair;",
                                "add_field": "",
                                "otherName": "Test"
                            }
                        ]
                    }
                ]
            }
            对应E_TestTest的field:
                optional AttrPair attr = 12[(nanopb).field_cname = "附加属性", (nanopb).max_count = 2];
            其中AttrPair定义在公共文件proto_common.proto里:
                message AttrPair
                {
                  optional AttrType attr = 1[(nanopb).field_cname = "属性"];
                  optional float value = 2[(nanopb).field_cname = "数值"];
                }
        3.json/xml/lua复杂数据结构体。只需要一列就可以定义。值填json字符串，可以自动转换为已经定义好的proto结构体.比如上面的jsonItemOne
            格式：struct:protoMessage:json|xml|lua, 比如上面的struct:proto_ff.Item:json
            对应E_TestTest的field:
                optional proto_ff.Item jsonItemOne = 19[(nanopb).field_cname = "jsonItemOne", (nanopb).parse_type=FPT_JSON];
            其中proto_ff.Item定义在公共文件proto_commm.proto里:
                message Item
                {
                  optional uint32 id = 1;
                  optional uint64 value = 2;
                  optional int32  quality = 3;
                }

### 其他特殊类型, 单列数组:
| id    | IntArray  | IntArray  | IntList                             | enumAttrArray          | enumAttrArray          | enumAttrList        |
|-------|-----------|-----------|-------------------------------------|------------------------|------------------------|------------------------|
| 编号   | IntArray1 | IntArray2 | IntList                            | enumAttrArray1         | enumAttrArray2         | enumAttrList        |
| int   | int       | int      | int:json:array:COMON_JSON_ATTR_LIST | enum:proto_ff.AttrType | enum:proto_ff.AttrType | enum:proto_ff.AttrType:json:array:COMON_JSON_ATTR_LIST |
| 3     | 3         | 3         | 3                                  | 3                      | 3                      | 3                      |
| 10001 | 1         | 2         | [1,2,3,4]                          | 等级                     | EN_ATTR_LEVEL          | [1,2,"EN_ATTR_MOV_SPD","控制命中修正"]                      |
    
    单列数组的两种表达方式: 
    1. 第一列英文field相同，第二列中文后面加数字。比如上面的IntArray,enumAttrArray.他将解析成field:
        repeated int IntArray = 13[(nanopb).field_cname = "IntArray", (nanopb).max_count = 2];
        repeated proto_ff.AttrType enumAttrArray = 13[(nanopb).field_cname = "enumAttrArray", (nanopb).max_count = 2];
    2. 使用json方案，只有一列，值是一个json数组。 比如上面的他IntList,enumAttrList, 将解析成field:
        repeated int IntList = 14[(nanopb).field_cname = "IntList", (nanopb).max_count_enum = "COMON_JSON_ATTR_LIST", (nanopb).parse_type=FPT_JSON];
        repeated proto_ff.AttrType enumAttrList = 14[(nanopb).field_cname = "enumAttrList", (nanopb).max_count_enum = "COMON_JSON_ATTR_LIST", (nanopb).parse_type=FPT_JSON];
        IntList,enumAttrList唯一的区别,enumAttrList使用的enum，可以使用中文，英文，数字多种表达方式。

### 其他特殊类型介绍, 复杂数据结构体数组:
| id    | pos[0].x | pos[0].y | attr[1].attr           | attr[1].value | jsonItemArray            | jsonItemArray             | jsonItemMore                   |
|-------|---------|----------|------------------------|---------------|--------------------------|---------------------------|---------------------------------|
| 编号   | 位置0x    | 位置0y     | 附加属性1属性          | 附加属性1数值   | jsonItemArray1           | jsonItemArray2            | jsonItemMore                     |
| int   | int     | int      | enum:proto_ff.AttrType | float         | struct:proto_ff.Item:json | struct:proto_ff.Item:json | struct:proto_ff.Item:json:array:COMON_JSON_ITEM_LIST       |
| 3     | 3       | 3        | 3                      | 3             | 3                        | 3                         | 3                               |
| 10001 | 30      | 100      | 等级                     | 1.11          | {"id":1,"value":2,"quality":3}                      |  {"id":1,"value":2,"quality":3}                         | [{"id":1,"value":2,"quality":3},{"id":1,"value":2,"quality":3}]  |

    复杂数据结构体数组的表达方式:
        1.自动生成结构体数组, 比如上面的pos[0].x, pos[0].y. 利用英文中的.点号隔开，将自动生成结构体. 利用[]明确数组索引。数组索引可以从0开始，也可以从1开始。
            message E_TestTestPos
            {
                optional int32 x = 1[(nanopb).field_cname = "x"];
	            optional int32 y = 2[(nanopb).field_cname = "y"];
            };
            对应E_TestTest的field:
                repeated E_TestPos pos = 10[(nanopb).field_cname = "位置"];
       2.利用现有结构体定义，格式同上，但是在excel.json文件中需要制定,比如"colMessageType": "attr_list:AttrPair;",指定attr_list，使用AttrPair解析。而不是自动生成结构体。方便很多结构体共用。
            {
                "excel": [
                    {
                        "name": "Test",
                        "sheet": [
                            {
                                "name": "Test",
                                "unique_index": "",
                                "multi_index": "",
                                "relation": "",
                                "createSql": true,
                                "field_enum": "",
                                "import_proto": "proto_common.proto",
                                "colMessageType": "attr:AttrPair;",
                                "add_field": "",
                                "otherName": "Test"
                            }
                        ]
                    }
                ]
            }
            对应E_TestTest的field:
                repeated AttrPair attr = 12[(nanopb).field_cname = "附加属性", (nanopb).max_count = 2];
            其中AttrPair定义在公共文件proto_common.proto里:
                message AttrPair
                {
                  optional AttrType attr = 1[(nanopb).field_cname = "属性"];
                  optional float value = 2[(nanopb).field_cname = "数值"];
                }
        3.json/xml/lua复杂数据结构体数组, 有两种方式。
            3.1 只需要一列就可以定义。值填json字符串，json必须是一个数组格式，可以自动转换为已经定义好的proto结构体数组.比如上面的jsonItemMore
                格式：struct:protoMessage:json|xml|lua:array:arrayNum, 比如上面的struct:proto_ff.Item:json:array:COMON_JSON_ITEM_LIST, 其中COMON_JSON_ITEM_LIST是定义在proto_common.proto里的一个enum, 表达数组大小
                对应E_TestTest的field:
                    repeated proto_ff.Item jsonItemMore = 20[(nanopb).field_cname = "jsonItemMore", (nanopb).max_count_enum = "COMON_JSON_ITEM_LIST", (nanopb).parse_type=FPT_JSON];
                其中proto_ff.Item定义在公共文件proto_commm.proto里:
                    message Item
                    {
                    optional uint32 id = 1;
                    optional uint64 value = 2;
                    optional int32  quality = 3;
                    }
            3.2 需要多列，数组格式，单独的一列都是json个数.可以自动转换为已经定义好的proto结构体数组.比如上面的jsonItemArray 利用英文中的.点号隔开，将自动生成结构体. 利用[]明确数组索引。数组索引可以从0开始，也可以从1开始。
                对应E_TestTest的field:
                    repeated proto_ff.Item jsonItemArray = 18[(nanopb).field_cname = "jsonItemArray", (nanopb).max_count = 2, (nanopb).parse_type=FPT_JSON];

        3. 嵌套组合使用，比较复杂，可参考Test.xlsx生成的proto

### excel.json特殊规则代码生成
    {
        "excel": [
            {
                "name": "Test",
                "sheet": [
                    {
                        "name": "Test",
                        "unique_index": "",
                        "multi_index": "",
                        "relation": "",
                        "createSql": true,
                        "field_enum": "",
                        "import_proto": "proto_common.proto",
                        "colMessageType": "attr_list:AttrPair;",
                        "add_field": "",
                        "otherName": "Test"
                    }
                ]
            }
        ]
    }
    其中excel下的name是excel的名字，sheet下的name是sheet的名字。如果不指定otherName, 将生成proto定义为message E_TestTest,
    如果填写了otherName, 生成message E_OtherName，主要是方便代码好看.
    unique_index：给这个表格生成一个额外的唯一索引。可以是一个列的唯一索引，也可以是多列组合的唯一索引.
    multi_index:给这个表格生成额外的多维索引，类似multimap.key可以是一列，可以可以是多列。
    relation:关联表格，将一列数据关联到一个表格，将自动生成代码。用来检查这一列数据是否存在。服务器启动时检查。
    field_enum:指定莫一列数据，必须是这个enum里的数据。如果不是，解析的时候报错。
    import_proto:导入额外的proto.
    colMessageType:指定莫一列数据，使用已经定义好的结构体.
    add_field:增加一些额外的field.

| Stl                     | shmstl               |
|-------------------------|----------------------|
| std::vector             | NFShmVector          |
| std::list               | NFShmList            |
| std::unordered_map      | NFShmHashMap         |
| std::unordered_multimap | NFShmHashMultiMap    |
|                         | NFShmHashMapWithList |
| std::unordered_set      | NFShmHashSet         |
| std::unordered_multiset | NFShmHashMultiSet    |
| std::pair               | NFShmPair            |
| std::queue              | NFShmQueue           |
| std::stack              | NFShmStack           |
| std::string             | NFShmString          |
| std::priority_queue     | NFShmPriorityQueue   |
| std::map                | NFShmMap             |
| std::set                | NFShmSet             |
| std::dequeue            | NFShmDequeue         |
| std::bitset             | NFShmBitSet          |
|                         | NFShmDyVector        |
|                         | NFShmDyList          |
|                         | NFShmDyHashMap       |
|                         | NFShmDyHashSet       |
