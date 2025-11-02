#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log()
{
    //同步不需要设置
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}

//异步需要设置阻塞队列的长度，同步不需要设置
//Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    //如果设置了max_queue_size,则设置为异步
    if (max_queue_size >= 1)
    {
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        pthread_t tid;
        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    //获取当前时间    time.h
    /*
        struct tm {
        int tm_sec;   // 秒 [0,59]
        int tm_min;   // 分 [0,59]
        int tm_hour;  // 时 [0,23]
        int tm_mday;  // 日 [1,31]
        int tm_mon;   // 月 [0,11]，注意：0 表示一月
        int tm_year;  // 自 1900 年起的年数，比如 2025 -> 125
        int tm_wday;  // 星期 [0,6]，0 表示星期日
        int tm_yday;  // 一年中的第几天 [0,365]
        int tm_isdst; // 夏令时标志
        };   
    */
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t); //将时间戳 t 转换为本地时间结构体
    struct tm my_tm = *sys_tm;


    //file_name == "./ServerLog"
    //strrchr() 找到 字符串 file_name 中最后一个 / 的位置。
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (p == NULL)
    {
        //2024_11_01_log.txt
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        //path/to/2024_11_01_log.txt
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }
    m_today = my_tm.tm_mday;


    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL)
    {
        return false;
    }

    return true;
}



void Log::write_log(int level, const char *format, ...)
{

    //日志时间和级别前缀
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }


    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    m_count++;


    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log
    {
        // 切换文件
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
       
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       
       // 构造新日志文件名
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
 
    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);
    string log_str;

    m_mutex.lock(); //对日志缓冲区加锁，保证多线程写日志时不会冲突

    //写入的具体时间内容格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();


    /*
    m_is_async → 标记日志是否 异步模式
    true → 日志先放入队列，由后台线程写文件
    false → 同步模式，当前线程直接写文件
    !m_log_queue->full() → 队列没有满，才能入队
    避免队列溢出
    如果队列满了，则退化为同步写文件
    */
    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void)
{
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}
