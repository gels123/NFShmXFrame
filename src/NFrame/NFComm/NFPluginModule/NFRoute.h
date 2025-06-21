// -------------------------------------------------------------------------
//    @FileName         :    NFRoute.h
//    @Author           :    gaoyi
//    @Date             :    23-12-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFRoute
//
// -------------------------------------------------------------------------

#pragma once

/**
 * @brief 路由分类  假如你有服务器1，服务器2，服务器3，和一个跨服服务器 组成的服务器做. 每一个服务器都有一个自己的唯一的worldsvr, 3个logicsvr，n个sns服务器， sns服务器服务器可以是一个，也可以N个，每一个都唯一，都是sns服务器，但功能不同。所有索引不一样。服务器索引可以是friend--1,guild--2
 * 1.本服路由  LOCAL_ROUTE              本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
 *                                      例如：服务器1的logic1发一条消息给worldsvr, 那就只会发给服务器1的worldsvr. 跨服务器的logic1发一条消息给worldsvr, 只会发给跨服服务器1的worldsvr.
 * 2.本服索引路由  LOCAL_ROUTE+index       本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
 *                                      例如：服务器1的logic1发一条消息给sns-friend, 那就只会发给服务器1的sns-friend. 跨服务器的logic1发一条消息给sns-friend, 只会发给跨服服务器1的sns-friend.
 * 3.跨服路由 CROSS_ROUTE               明确指定要找跨服服务器， 才走跨服路由
 *                                     例如：服务器1的logic1发一条消息给worldsvr, 那就只会发给跨服服务器1的worldsvr. 跨服务器的logic1发一条消息给worldsvr, 也只会发给跨服服务器1的worldsvr.
 * 4.跨服索引路由 CROSS_ROUTE+index          明确指定要找跨服服务器， 才走跨服路由
 *                                     例如：服务器1的logic1发一条消息给跨服服务器的sns-friend, 那就只会发给跨服服务器的sns-friend. 跨服务器的logic1发一条消息给sns-friend, 只会发给跨服服务器的sns-friend.
 * 7.LOCAL_ROUTE_ZONE+zoneId 区服路由  LOCAL_ROUTE_ZONE + 区服的zid(1-4096)         只有跨服route server服务器，才有区服路由的能力。 用于跨服服务器给区服服务器发送消息
                                        例如：跨服服务器worldsvr要给服务器1的worldsvr发送消息， 那么只要发LOCAL_ROUTE_ZONE+服务器1的zid即可, 如果是要发logicsvr, 那么只有随机一个logicsvr会收到。一般用来发唯一服务区。
 * 8.CROSS_ROUTE_ZONE+zoneId               区服路由的补充，跨服路由同服务器类型群发路由 (最大分区4096， 所以10000+zoneid, 10001-14096之间), 对统一类型的服务器群发
                                        例如：跨服服务器worldsvr要给服务器1的所有logicsvr发送消息， 那么只要CROSS_ROUTE_ZONE+发服务器1的zid即可
 * 9.LOCAL_ALL_ROUTE               LOCAL_ROUTE的补充，对由于同类型的服务器发送本服路由
 *                                      例如：服务器1的worldsvr发一条消息给logic，只有服务器1的所有logic都会受到这条消息,  跨服务器的worldsvr发一条消息给logic, 也只有跨服务器的所有logic都会受到这条消息
 * 10.CROSS_ALL_ROUTE               CROSS_ROUTE，对由于同类型的服务器发送跨服路由
 *                                 如：服务器1的worldsvr发一条消息给logic，只有跨服务器的所有logic都会受到这条消息,  跨服务器的worldsvr发一条消息给logic, 也只有跨服务器的所有logic都会受到这条消息
 * 11.LOCAL_AND_CROSS_ALL_ROUTE               LOCAL_ROUTE和CROSS_ROUTE的补充，对由于同类型本服和跨服的服务器发送跨服路由
 *                                  例如：服务器1的worldsvr发一条消息给logic，服务器1和跨服务器的所有logic都会受到这条消息,  跨服务器的worldsvr发一条消息给logic, 也只有服务器1和跨服务器的所有logic都会受到这条消息, 服务器2，服务器3，不会受到这条消息
 * 12.ALL_LOCAL_AND_ALL_CROSS_ROUTE               LOCAL_ROUTE和CROSS_ROUTE的补充，对由于同类型所有本服和跨服的服务器发送跨服路由, 包含所有服务器和跨服服务器，
 *                                  例如：服务器1的worldsvr发一条消息给logic，服务器1,服务器2，服务器3，和跨服务器的所有logic都会受到这条消息
 * 13.busid路由 服务器唯一ID
 */

/**
 * @brief
 */
#define LOCAL_ROUTE 0

/**
 * @brief 跨服路由
 */
#define CROSS_ROUTE 10000

/**
 * @brief 跨服路由同服务器类型群发路由 (最大分区4096， 所以LOCAL_ROUTE_ZONE+zoneid, 20001-24096之间)
 */
#define LOCAL_ROUTE_ZONE 20000
/**
 * @brief 跨服路由同服务器类型群发路由 (最大分区4096， 所以CROSS_ROUTE_ZONE+zoneid, 30001-34096之间)
 */
#define CROSS_ROUTE_ZONE 30000

#define LOCAL_ALL_ROUTE 40000
#define CROSS_ALL_ROUTE 40001
#define LOCAL_AND_CROSS_ALL_ROUTE 40002
#define ALL_LOCAL_AND_ALL_CROSS_ROUTE 40003
#define LOCAL_AND_CROSS_MAX 41000
