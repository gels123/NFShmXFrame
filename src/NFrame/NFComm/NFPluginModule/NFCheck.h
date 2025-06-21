// -------------------------------------------------------------------------
//    @FileName         :    NFCheck.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFLogMgr.h"
#include "NFComm/NFCore/NFPlatform.h"

#ifndef NF_ASSERT
#define NF_ASSERT(expr)\
    do {\
        if (unlikely(!(expr)))\
        {\
           try {\
			    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed", #expr);\
            }\
            catch (fmt::format_error& error) {\
				NFLogError(NF_LOG_DEFAULT, 0, "{}", error.what());\
            }\
			NF_COMM_ASSERT(expr);\
        }\
    }while(0)
#endif//NF_ASSERT

#ifndef NF_ASSERT_MSG
#define NF_ASSERT_MSG(expr, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
           try {\
                std::stringstream ss;\
                ss << format;\
                std::string log_event;\
				if (NFStringUtility::Contains(ss.str(), '{'))\
				{\
					log_event = NF_FORMAT(ss.str().c_str(), ##__VA_ARGS__);\
				}\
				else\
				{\
					log_event = NF_SPRINTF(ss.str().c_str(), ##__VA_ARGS__);\
				}\
			    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
            }\
            catch (fmt::format_error& error) {\
                    NFLogError(NF_LOG_DEFAULT, 0, "{}", error.what());\
            }\
			NF_COMM_ASSERT(expr);\
        }\
    }while(0)
#endif//NF_ASSERT_MSG

#ifndef NF_ASSERT_RET_VAL
#define NF_ASSERT_RET_VAL(expr, val, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			try {\
			    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed return:{}", #expr, val);\
           }\
           catch (fmt::format_error& error) {\
				NFLogError(NF_LOG_DEFAULT, 0, "{}", error.what());\
			}\
			NF_COMM_ASSERT(expr);\
            return val;\
        }\
    }while(0)
#endif//NF_ASSERT_RET_VAL

#ifndef NF_ASSERT_RET_VAL_MSG
#define NF_ASSERT_RET_VAL_MSG(expr, val, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
            std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
		    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{} return:{}", #expr, log_event, val);\
			NF_COMM_ASSERT(expr);\
            return val;\
        }\
    }while(0)
#endif//NF_ASSERT_RET_VAL_MSG

#ifndef NF_ASSERT_BREAK
#define NF_ASSERT_BREAK(expr)\
    {\
        if (unlikely(!(expr)))\
        {\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed", #expr);\
			NF_COMM_ASSERT(expr);\
            break;\
        }\
    }
#endif//NF_ASSERT_BREAK

#ifndef NF_ASSERT_BREAK_MSG
#define NF_ASSERT_BREAK_MSG(expr, format, ...)\
    {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
		    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
			NF_COMM_ASSERT(expr);\
            break;\
        }\
    }
#endif//NF_ASSERT_BREAK_MSG

#ifndef NF_ASSERT_CONTINUE
#define NF_ASSERT_CONTINUE(expr)\
    {\
        if (unlikely(!(expr)))\
        {\
           try {\
			    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed", #expr);\
            }\
            catch (fmt::format_error& error) {\
                    NFLogError(NF_LOG_DEFAULT, 0, "{}", error.what());\
            }\
			NF_COMM_ASSERT(expr);\
            continue;\
        }\
    }
#endif//NF_ASSERT_CONTINUE

#ifndef NF_ASSERT_CONTINUE_MSG
#define NF_ASSERT_CONTINUE_MSG(expr, format, ...)\
    {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
			NF_COMM_ASSERT(expr);\
            continue;\
        }\
    }
#endif//NF_ASSERT_CONTINUE_MSG

#ifndef NF_ASSERT_RET_NONE
#define NF_ASSERT_RET_NONE(expr)\
    {\
        if (unlikely(!(expr)))\
        {\
           try {\
			    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:", #expr);\
            }\
            catch (fmt::format_error& error) {\
                    NFLogError(NF_LOG_DEFAULT, 0, "{}", error.what());\
            }\
			NF_COMM_ASSERT(expr);\
            return;\
        }\
    }
#endif//NF_ASSERT_RET_NONE

#ifndef NF_ASSERT_RET_NONE_MSG
#define NF_ASSERT_RET_NONE_MSG(expr, format, ...)\
    {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
			NF_COMM_ASSERT(expr);\
            return;\
        }\
    }
#endif//NF_ASSERT_RET_NONE_MSG

#ifndef NF_ASSERT_NO_EFFECT
#define NF_ASSERT_NO_EFFECT(expr)\
    {\
        if (unlikely(!(expr)))\
        {\
           try {\
			    NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed", #expr);\
            }\
            catch (fmt::format_error& error) {\
                    NFLogError(NF_LOG_DEFAULT, 0, "{}", error.what());\
            }\
			NF_COMM_ASSERT(expr);\
        }\
    }
#endif//NF_ASSERT_NO_EFFECT

#ifndef NF_ASSERT_NO_EFFECT_MSG
#define NF_ASSERT_NO_EFFECT_MSG(expr, format, ...)\
    {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
			NF_COMM_ASSERT(expr);\
        }\
    }
#endif//NF_ASSERT_NO_EFFECT_MSG

#define NF_CHECK(EXPRESSION) \
  NF_ASSERT_MSG(EXPRESSION, "CHECK failed: " #EXPRESSION)

#define NF_CHECK_EQ(A, B) NF_CHECK((A) == (B))
#define NF_CHECK_NE(A, B) NF_CHECK((A) != (B))
#define NF_CHECK_LT(A, B) NF_CHECK((A) <  (B))
#define NF_CHECK_LE(A, B) NF_CHECK((A) <= (B))
#define NF_CHECK_GT(A, B) NF_CHECK((A) >  (B))
#define NF_CHECK_GE(A, B) NF_CHECK((A) >= (B))

#ifndef _USE_ASSERT_
#define _USE_ASSERT_ 0
#endif

#if _USE_ASSERT_
#define assert_i(exp) NF_ASSERT(exp)
#else
#define assert_i(exp)
#endif

#ifndef CHECK_EXPR
#define CHECK_EXPR(expr, ret, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
            NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
            assert_i(expr);\
            return ret;\
        }\
    }while(0)
#endif//CHECK_EXPR

#ifndef CHECK_EXPR_RE_VOID
#define CHECK_EXPR_RE_VOID(expr, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
            assert_i(expr);\
            return;\
        }\
    }while(0)
#endif//CHECK_EXPR_RE_VOID

#ifndef CHECK_NOT_RET
#define CHECK_NOT_RET(expr)\
    do {\
        if (unlikely(!(expr)))\
        {\
            return;\
        }\
    }while(0)
#endif//CHECK_NOT_RET

#ifndef CHECK_EXPR_MSG
#define CHECK_EXPR_MSG(expr, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
        }\
    }while(0)
#endif//CHECK_EXPR_MSG

#ifndef CHECK_EXPR_MSG_RESULT
#define CHECK_EXPR_MSG_RESULT(expr, result, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			result = -1;\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
        }\
    }while(0)
#endif//CHECK_EXPR_MSG_RESULT

#ifndef CHECK_EXPR_ASSERT
#define CHECK_EXPR_ASSERT(expr, ret, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
			NF_COMM_ASSERT(expr);\
			return ret;\
        }\
    }while(0)
#endif//CHECK_EXPR

#ifndef CHECK_EXPR_ASSERT_NOT_RET
#define CHECK_EXPR_ASSERT_NOT_RET(expr, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
			NF_COMM_ASSERT(expr);\
			return;\
        }\
    }while(0)
#endif//CHECK_EXPR

#ifndef CHECK_EQ
#define CHECK_EQ(ret) CHECK_EXPR(ret, -1, "{}", #ret)
#endif

#ifndef CHECK_RET
#define CHECK_RET(ret, format, ...) CHECK_EXPR(0 == ret, ret, format, ##__VA_ARGS__)
#endif//CHECK_RET

#ifndef CHECK_RET_ASSERT
#define CHECK_RET_ASSERT(ret, format, ...) CHECK_EXPR_ASSERT(0 == ret, ret, format, ##__VA_ARGS__)
#endif//CHECK_RET

#ifndef CHECK_PLAYER_EXPR
#define CHECK_PLAYER_EXPR(id, expr, ret, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, id, "CHECK {} failed:{}", #expr, log_event);\
			assert_i(expr);\
			return ret;\
        }\
    }while(0)
#endif//CHECK_PLAYER_EXPR

#ifndef CHECK_PLAYER_RET
#define CHECK_PLAYER_RET(id, ret, format, ...) CHECK_PLAYER_EXPR(id, 0 == ret, ret, format, ##__VA_ARGS__)
#endif//CHECK_PLAYER_RET

#ifndef CHECK_NULL_ASSERT
#define CHECK_NULL_ASSERT(ptr) CHECK_EXPR_ASSERT(ptr, -1, "");
#endif //CHECK_NULL

#ifndef CHECK_PLAYER_NULL
#define CHECK_PLAYER_NULL(id, ptr) CHECK_PLAYER_EXPR(id, ptr, -1, "")
#endif//CHECK_PLAYER_NULL

template<bool>
struct CompileTimeError;

template<>
struct CompileTimeError<true>
{
};

#ifndef COMPILE_TIME_ASSERT
#define COMPILE_TIME_ASSERT(expr) \
    (CompileTimeError<(expr) != 0> ())
#endif //COMPILE_TIME_ASSERT

#ifndef CHECK_EXPR_BREAK
#define CHECK_EXPR_BREAK(expr, format, ...)\
    {\
        if(unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
            break;\
		}\
	}
#endif//CHECK_EXPR_CONTINUE

#ifndef CHECK_EXPR_CONTINUE
#define CHECK_EXPR_CONTINUE(expr, format, ...)\
    {\
        if(unlikely(!(expr)))\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			NFLogError(NF_LOG_DEFAULT, 0, "CHECK {} failed:{}", #expr, log_event);\
            continue;\
		}\
	}
#endif//CHECK_EXPR_CONTINUE

#ifndef CHECK_CONTINUE
#define CHECK_CONTINUE(expr)\
    {\
        if(unlikely(!(expr)))\
        {\
            continue;\
		}\
	}
#endif//CHECK_CONTINUE

#ifndef CHECK_BREAK
#define CHECK_BREAK(expr)\
    {\
        if(unlikely(!(expr)))\
        {\
            break;\
		}\
	}
#endif//CHECK_BREAK

#ifndef CHECK_BREAK_VALUE
#define CHECK_BREAK_VALUE(expr, ret, value)\
    {\
        if(unlikely(!(expr)))\
        {\
            ret = value;\
            break;\
		}\
	}
#endif//CHECK_BREAK_VALUE

#ifndef CHECK_EXPR_RETURN
#define CHECK_EXPR_RETURN(expr)\
    do {\
        if(unlikely(!(expr)))\
        {\
            return 0;\
        }\
    }while(0)
#endif//CHECK_EXPR_RETURN

#ifndef CHECK_NULL_RETURN
#define CHECK_NULL_RETURN(ptr) CHECK_EXPR_RETURN(ptr)
#endif//CHECK_NULL_RETURN

#define MLOG_EQ( Val, MustBeVal, iRetCode,  format, ... )\
    if( (Val) != (MustBeVal) )\
    {\
		std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
        LOG_ERR(0, iRetCode, "{} != {} Failed {}", #Val, #MustBeVal,log_event);\
        return iRetCode;\
    }
#define MLOG_FATAL_EQ( Val, MustBeVal, iRetCode,  format, ... )\
    if( (Val) != (MustBeVal) )\
    {\
		std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
        LOG_FATAL(0, iRetCode, "{} != {} Failed {}", #Val, #MustBeVal, log_event);\
        return iRetCode;\
    }
#define LOG_TRACE_PROTO_DEBUG(RoleID,  format, ...)\
    do\
    {\
		std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
        LOG_INFO(RoleID, "[{}::{}:{}] {}\n", CLASSNAME,__FUNCTION__, __LINE__, log_event);\
    }while(0)

#define CHECK_NULL_RETCODE(RoleID,  pPointer, iRetCode,  format, ... )\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, iRetCode, "{} is NULL {}", #pPointer, log_event);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_NULL_WARN_RETCODE(RoleID,  pPointer, iRetCode,  format, ... )\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_WARN(RoleID, iRetCode, "{} is NULL {}", #pPointer, log_event);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_NULL_RET_VAL(RoleID, pPointer, val)\
    do{\
		if( pPointer == NULL )\
        {\
			LOG_ERR(RoleID, -1, "{} is NULL ", #pPointer);\
			return val;\
        }\
    }while(0)

#define CHECK_NULL_WARN_RET_VAL(RoleID, pPointer, val)\
    do{\
    if( pPointer == NULL )\
        {\
        LOG_WARN(RoleID, -1, "{} is NULL ", #pPointer);\
        return val;\
        }\
    }while(0)

#define CHECK_NULL_RET_VOID(RoleID, pPointer)\
    do{\
		if( pPointer == NULL )\
        {\
			LOG_ERR(RoleID, -1, "{} is NULL ", #pPointer);\
			return;\
        }\
    }while(0)

#define CHECK_NULL(RoleID,  pPointer)\
	do{\
		if( pPointer == NULL )\
		{\
			LOG_ERR(RoleID, -1, "{} is NULL ", #pPointer);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_NULL_WARN_WF(RoleID,  pPointer,  format, ...)\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_WARN(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_NULL_WF(RoleID,  pPointer,  format, ...)\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_PLAYER_NULL_WF( RoleID, pPointer,  format, ... )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return -1;\
		}\
	} while (0);

#define CHECK_NULL_RE_NULL( RoleID, pPointer,  format, ... )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, -1, "pointer is NULL {}", log_event);\
			return NULL;\
		}\
	}while(0)

#define CHECK_NULL_WARN_RE_NULL( RoleID, pPointer,  format, ... )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_WARN(RoleID, -1, "pointer is NULL {}", log_event);\
			return NULL;\
		}\
	}while(0)

#define CHECK_NULL_RET_VOID_WF(RoleID, pPointer,  format, ... )\
    do{\
		if( pPointer == NULL )\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return;\
        }\
    }while(0)

#define CHECK_NULL_RE_VAR_WF(RoleID, pPointer, var,  format, ... )\
    do{\
    if( pPointer == NULL )\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return var;\
        }\
    }while(0)

#define CHECK_NULL_WARN_RE_VAR_WF(RoleID, pPointer, var, format, ...)                 \
    do                                                                           \
    {                                                                            \
        if (pPointer == NULL)                                                    \
        {                                                                        \
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
            LOG_WARN(RoleID, -1, "{} is NULL {}", #pPointer, log_event); \
            return var;                                                          \
        }                                                                        \
    } while (0)

#define CHECK_NULL_RE_FALSE( RoleID, pPointer )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			return false;\
		}\
	}while(0)

#define CHECK_NULL_RE_FALSE_WF(RoleID, pPointer,  format, ... )\
    do{\
		if( pPointer == NULL )\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return false;\
        }\
    }while(0)

#define CHECK_NULL_RE_TRUE( RoleID, pPointer )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			return true;\
		}\
	}while(0)

#define CHECK_NULL_RE_VAL( RoleID, pPointer, val )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			return val;\
		}\
	}while(0)

#define CHECK_ERR( RoleID, iRetCode,  format, ... )\
	do{\
		if( iRetCode != 0 )\
		{\
			LOG_ERR(RoleID, iRetCode,  format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_NULL_WARN_RET_VOID(RoleID, pPointer)\
    do{\
    if( pPointer == NULL )\
        {\
        LOG_WARN(RoleID, -1, "%s is NULL ", #pPointer);\
        return;\
        }\
    }while(0)

#define CHECK_NULL_WARN(RoleID,  pPointer)\
		do{\
			if( pPointer == NULL )\
			{\
				LOG_WARN(RoleID, -1, "%s is NULL ", #pPointer);\
				return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
			}\
		}while(0)

#define CHECK_NULL_WF_WARN(RoleID,  pPointer,  format, ...)\
		do{\
			if( pPointer == NULL )\
			{\
				std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
				LOG_WARN(RoleID, -1, "{} is NULL ", #pPointer, log_event);\
				return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
			}\
		}while(0)

#define CHECK_NULL_WF_WARN_RET_VOID(RoleID,  pPointer,  format, ...)\
        do{\
            if( pPointer == NULL )\
            {\
				std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
                LOG_WARN(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
                return ;\
            }\
        }while(0)


#define CHECK_NULL_DEBUG(RoleID,  pPointer)\
		do{\
			if( pPointer == NULL )\
			{\
				LOG_DEBUG(RoleID, "%s is NULL ", #pPointer);\
				return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
			}\
		}while(0)

#define CHECK_NULL_DEBUG_WF(RoleID,  pPointer,  format, ...)\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_DEBUG(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_INFO( RoleID, iRetCode,  format, ... )\
	do{\
		if( iRetCode != 0 )\
		{\
			LOG_INFO(RoleID, format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_WARN( RoleID, iRetCode,  format, ... )\
	do{\
		if( iRetCode != 0 )\
		{\
			LOG_WARN(RoleID, iRetCode, format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_FATAL( RoleID, iRetCode,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR(RoleID, iRetCode,  format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_ERR_RE_NULL( RoleID, iRetCode,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR(RoleID, iRetCode,  format, ##__VA_ARGS__);\
			return NULL;\
		}\
	}while(0)

#define CHECK_WARN_RE_NULL( RoleID, iRetCode,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_WARN(RoleID, iRetCode,  format, ##__VA_ARGS__);\
			return NULL;\
		}\
	}while(0)

#define CHECK_ERR_RE_VAL( RoleID, iRetCode, val,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR(RoleID, iRetCode, #val " " format,  ##__VA_ARGS__);\
			return val;\
		}\
	}while(0)

#define CHECK_WARN_RE_VAL( RoleID, iRetCode, val,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_WARN(RoleID, iRetCode, #val " " format,  ##__VA_ARGS__);\
			return val;\
		}\
	}while(0)


#define CHECK_ERR_RE_VOID( RoleID, iRetCode,  format, ... )\
    do{\
    if ( iRetCode != 0 )\
        {\
            LOG_ERR(RoleID, iRetCode,  format, ##__VA_ARGS__);\
            return;\
        }\
    }while(0)

#define CHECK_WARN_RE_VOID( RoleID, iRetCode,  format, ... )\
    do{\
    if ( iRetCode != 0 )\
        {\
            LOG_WARN(RoleID, iRetCode,  format, ##__VA_ARGS__);\
            return;\
        }\
    }while(0)

#define CHECK_ERR_NT( RoleID, iRetCode,  format, ...)\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR(RoleID, iRetCode,  format, ##__VA_ARGS__);\
		}\
	}while(0)

#define CHECK_WARN_NT( RoleID, iRetCode,  format, ...)\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_WARN(RoleID, iRetCode,  format, ##__VA_ARGS__);\
		}\
	}while(0)

#define CHECK_WARN_BREAK_WF_M( RoleID, module, iRetCode,  format, ... )\
	if( iRetCode != 0 )\
	{\
		LOG_WARN_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
		break;\
	}
#define CHECK_WARN_BREAK(RoleID, iRetCode, format, ...) \
    if (iRetCode != 0)                                               \
    {                                                                \
        LOG_WARN(RoleID, iRetCode, format, ##__VA_ARGS__); \
        break;                                                       \
    }
#define CHECK_ERR_RT( RoleID, iRetCode,  format, ...)\
	CHECK_ERR_RE_NULL( RoleID, iRetCode,  format, ##__VA_ARGS__)

#define CHECK_W_ERR( iRetCode,  format, ... )\
    CHECKW_IF_ERR_RETCODE( iRetCode, CLASSNAME,  format, ##__VA_ARGS__)
// Value Check
// 要求INT等于目标值，否则报错
#define CHECK_INT_EQ(RoleID, INT, VALUE) \
	do{ \
		if(INT != VALUE) \
		{ \
			LOG_ERR(RoleID, -1, "%s:%d not eq %d ", #INT, INT, VALUE);\
			return ERR_VALUE_CHECK_INVALID; \
		} \
	}while(0)

// 要求INT大于等于目标值，否则报错
#define CHECK_INT_GE(RoleID, INT, VALUE) \
	do{ \
		if(INT < VALUE) \
		{ \
			LOG_ERR(RoleID, -1, "%s:%d is less than %d ", #INT, INT, VALUE);\
			return ERR_VALUE_CHECK_INVALID; \
		} \
	}while(0)

// 要求INT大于等于目标值，否则报错
#define CHECK_INT_GE_WARN(RoleID, INT, VALUE) \
	do{ \
		if(INT < VALUE) \
		{ \
			LOG_WARN(RoleID, -1, "%s:%d is less than %d ", #INT, INT, VALUE);\
			return ERR_VALUE_CHECK_INVALID; \
		} \
	}while(0)

// 要求INT大于等于目标值，否则报错
#define CHECK_UINT_GE(RoleID, UINT, VALUE) \
	do{ \
		if(UINT < VALUE) \
		{ \
			LOG_ERR(RoleID, -1, "%s:%u is less than %u ", #UINT, UINT, VALUE);\
			return ERR_VALUE_CHECK_INVALID; \
		} \
	}while(0)

// 要求INT小于等于目标值，否则报错
#define CHECK_INT_LE(RoleID, INT, VALUE) \
	do{ \
		if(INT > VALUE) \
		{ \
			LOG_ERR(RoleID, -1, "%s:%d is greater than %d ", #INT, INT, VALUE);\
			return ERR_VALUE_CHECK_INVALID; \
		} \
	}while(0)

//打WARN日志
#define CHECK_NULL_WARN_RETCODE_M(RoleID,  module, pPointer, iRetCode,  format, ... )\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_WARN_M(RoleID, module, iRetCode, "{} is NULL {}", #pPointer, log_event);\
			return iRetCode;\
		}\
	}while(0)

///check 打 module log
#define CHECK_NULL_RETCODE_M(RoleID,  module, pPointer, iRetCode,  format, ... )\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR_M(RoleID, module, iRetCode, "{} is NULL {}", #pPointer, log_event);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_NULL_RET_VAL_M(RoleID, module, pPointer, val)\
    do{\
    if( pPointer == NULL )\
        {\
        LOG_ERR_M(RoleID, module, -1, "%s is NULL ", #pPointer);\
        return val;\
        }\
    }while(0)

#define CHECK_NULL_RET_VOID_M(RoleID, module, pPointer)\
    do{\
    if( pPointer == NULL )\
        {\
        LOG_ERR_M(RoleID, module, -1, "%s is NULL ", #pPointer);\
        return;\
        }\
    }while(0)

#define CHECK_NULL_M(RoleID, module, pPointer)\
	do{\
		if( pPointer == NULL )\
		{\
			LOG_ERR_M(RoleID, module, -1, "%s is NULL ", #pPointer);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_NULL_M_WARN(RoleID, module, pPointer)\
	do{\
		if( pPointer == NULL )\
		{\
			LOG_WARN_M(RoleID, module, -1, "%s is NULL ", #pPointer);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_NULL_WF_M(RoleID, module, pPointer,  format, ...)\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR_M(RoleID, module, -1, "{} is NULL {}", #pPointer, log_event);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_NULL_WF_M_WARN(RoleID, module, pPointer,  format, ...)\
	do{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_WARN_M(RoleID, module, -1, "{} is NULL {}", #pPointer, log_event);\
			return ERR_LOG_SYSTEM_SERVER_OBJ_NULL;\
		}\
	}while(0)

#define CHECK_PLAYER_NULL_WF_M( RoleID, module, pPointer,  format, ... )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR_M(RoleID, module, -1, "{} is NULL {}", #pPointer, log_event);\
			return -1;\
		}\
	} while (0);

#define CHECK_NULL_RE_NULL_M( RoleID, module, pPointer,  format, ... )\
	do\
	{\
		if( pPointer == NULL )\
		{\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR_M(RoleID, module, -1, "pointer is NULL {}", log_event);\
			return NULL;\
		}\
	}while(0)

#define CHECK_NULL_RET_VOID_WF_M(RoleID, module, pPointer,  format, ... )\
    do{\
    if( pPointer == NULL )\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR_M(RoleID, module, -1, "{} is NULL {}", #pPointer, log_event);\
			return;\
        }\
    }while(0)

#define CHECK_NULL_RE_VAR_WF_M(RoleID, module, pPointer, var,  format, ... )\
    do{\
    if( pPointer == NULL )\
        {\
			std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
			LOG_ERR_M(RoleID, module, -1, "{} is NULL {}", #pPointer, log_event);\
			return var;\
        }\
    }while(0)

#define CHECK_ERR_BREAK( RoleID, iRetCode)\
	if( iRetCode != 0 )\
	{\
		break;\
	}
#define CHECK_ERR_BREAK_WF( RoleID, iRetCode,  format, ... )\
	if( iRetCode != 0 )\
	{\
		LOG_ERR(RoleID, iRetCode,  format, ##__VA_ARGS__);\
		break;\
	}
#define CHECK_ERR_BREAK_WF_M( RoleID, module, iRetCode,  format, ... )\
	if( iRetCode != 0 )\
	{\
		LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
		break;\
	}

#define CHECK_ERR_BREAK_WF_M_WARN( RoleID, module, iRetCode,  format, ... )\
	if( iRetCode != 0 )\
	{\
		LOG_WARN_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
		break;\
	}
#define CHECK_NULL_BREAK( RoleID, pPointer)\
	if( pPointer == NULL )\
	{\
		LOG_ERR(RoleID, -1, "%s is NULL ", #pPointer);\
		break;\
	}

#define CHECK_NULL_BREAK_WARN(RoleID, pPointer)             \
    if (pPointer == NULL)                              \
    {                                                  \
        LOG_WARN(RoleID, -1, "%s is NULL ", #pPointer); \
        break;                                         \
    }
#define CHECK_NULL_BREAK_M( RoleID, module,  pPointer)\
	if( pPointer == NULL )\
	{\
		LOG_ERR_M(RoleID, module, -1, "%s is NULL ", #pPointer);\
		break;\
	}

#define CHECK_NULL_BREAK_WF( RoleID, pPointer,  format, ... )\
	if( pPointer == NULL )\
	{\
		std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
		LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
		break;\
	}
#define CHECK_NULL_BREAK_WF_M( RoleID, module, pPointer,  format, ... )\
	if( pPointer == NULL )\
	{\
		std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
		LOG_ERR_M(RoleID, module, -1, "{} is NULL {}", #pPointer, log_event);\
		break;\
	}

#define CHECK_NULL_CONTINUE_RET_VOID(RoleID, pPointer)\
    if( pPointer == NULL )\
    {\
		LOG_ERR(RoleID, -1, "%s is NULL ", #pPointer);\
		continue;\
    }
#define CHECK_NULL_CONTINUE( RoleID, pPointer)\
	if( pPointer == NULL )\
	{\
		continue;\
	}

#define CHECK_NULL_CONTINUE_WF( RoleID, pPointer,  format, ... )\
	if( pPointer == NULL )\
	{\
		std::string log_event = NF_GET_STRING(format, ##__VA_ARGS__);\
		LOG_ERR(RoleID, -1, "{} is NULL {}", #pPointer, log_event);\
		continue;\
	}
//Err Check
#define CHECK_ERR_M( RoleID, module, iRetCode,  format, ... )\
	do{\
		if( iRetCode != 0 )\
		{\
			LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)


#define CHECK_WARN_M( RoleID, module, iRetCode,  format, ... )\
	do{\
		if( iRetCode != 0 )\
		{\
			LOG_WARN_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)

#define CHECK_FATAL_M( RoleID, module, iRetCode,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
			return iRetCode;\
		}\
	}while(0)


#define CHECK_ERR_RE_NULL_M( RoleID, module, iRetCode,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
			return NULL;\
		}\
	}while(0)

#define CHECK_ERR_RE_VAL_M( RoleID, module, iRetCode, val,  format, ... )\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR_M(RoleID, module, iRetCode, #val " " format,  ##__VA_ARGS__);\
			return val;\
		}\
	}while(0)


#define CHECK_ERR_RE_VOID_M( RoleID,  module, iRetCode,  format, ... )\
    do{\
    if ( iRetCode != 0 )\
        {\
            LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
            return;\
        }\
    }while(0)

#define CHECK_ERR_NT_M( RoleID, module, iRetCode,  format, ...)\
	do{\
		if ( iRetCode != 0 )\
		{\
			LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__);\
		}\
	}while(0)
