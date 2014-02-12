/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __CONFIG_HEADER__
#define __CONFIG_HEADER__
#include <unistd.h>
#include <vector>
#include <string>


using namespace std;

class Config
{
public:
    Config();
    ~Config();

    int read_config(const char *file_name);
    int get_listen_port() const
    {
        return m_port;
    }

    int get_worker_num() const
    {
        return m_worker_num;
    }

    int get_keep_alive() const
    {
        return m_keep_alive;
    }

    int get_asure_count() const
    {
        return m_asure_count;
    }

    int get_lifetime() const
    {
        return m_lifetime;
    }

private:
    int m_port;
    int m_worker_num;
    int m_keep_alive;
    int m_asure_count;
    int m_lifetime;
};

Config *glb_config();

#endif
