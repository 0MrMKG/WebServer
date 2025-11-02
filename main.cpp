#include "webserver.h"

int main(int argc, char *argv[])
{
    int port = 9006; // 服务器监听端口号（默认9006，可通过 -p 指定）
    int sql_num = 8; // 数据库连接池连接数量（默认8，可通过 -s 指定）
    int thread_num = 8; //线程池线程数量（默认8，可通过 -t 指定）

    string user = "debian-sys-maint";
    string passwd = "Wqnz31rhSnOSBcbc";
    string databasename = "yourdb";

    int OPT_LINGER = 0;  // 是否启用优雅关闭连接：0=关闭，1=开启
    int TRIGMode = 0;    // 触发模式组合：0=LT+LT，1=LT+ET，2=ET+LT，3=ET+ET
    int close_log = 0;   // 是否关闭日志系统：0=开启日志，1=关闭日志
    int actor_model = 0; // 并发模型：0=Proactor，1=Reactor

    WebServer server;

    /*
        server.init(
            config.PORT,         // 服务器监听端口号（默认9006，可通过 -p 指定）
            user,                // 数据库用户名
            passwd,              // 数据库密码
            databasename,        // 数据库名
            config.LOGWrite,     // 日志写入方式：0=同步日志，1=异步日志
            config.OPT_LINGER,   // 是否启用优雅关闭连接：0=关闭，1=开启
            config.TRIGMode,     // 触发模式组合：0=LT+LT，1=LT+ET，2=ET+LT，3=ET+ET
            config.sql_num,      // 数据库连接池连接数量（默认8，可通过 -s 指定）
            config.thread_num,   // 线程池线程数量（默认8，可通过 -t 指定）
            config.close_log,    // 是否关闭日志系统：0=开启日志，1=关闭日志
            config.actor_model   // 并发模型：0=Proactor，1=Reactor
        );
    */

    //初始化server
    server.init(port, user, passwd, databasename, 0, 
                OPT_LINGER, TRIGMode, sql_num, thread_num, 
                close_log, actor_model);
    //日志
    server.log_write();
    //数据库
    server.sql_pool();
    //线程池
    server.thread_pool();
    //触发模式
    server.trig_mode();

    //监听
    server.eventListen();
    //运行
    server.eventLoop();

    return 0;
}