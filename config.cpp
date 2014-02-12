/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include "log.h"
#include "config.h"
#include "tinyxml.h"

const static int DEFAULT_WORK_NUM = 4;

Config::Config()
{

}

Config::~Config()
{

}

int Config::read_config(const char *file_name)
{
    TiXmlDocument doc(file_name);
    bool loadOkay = doc.LoadFile();

    if (!loadOkay)
    {
        logerr( "Could not load file:%s. Error='%s'. Exiting.\n", file_name, doc.ErrorDesc());
        return -1;
    }

    TiXmlElement *root_element = doc.RootElement();
    TiXmlElement *child_element = root_element->FirstChildElement("port");
    if (!child_element)
    {
        logerr("read port not found!\n");
        return -1;
    }
    m_port = atoi(child_element->GetText());
    logdbg("m_port:%d\n", m_port);

    child_element = root_element->FirstChildElement("worker_num");
    if (!child_element)
    {
        logerr("read port not found!\n");
        return -1;
    }
    m_worker_num = atoi(child_element->GetText());
    if (m_worker_num > DEFAULT_WORK_NUM || m_worker_num < 1)
    {
        logerr("Invalid worker num:%d, set to:%d\n", m_worker_num, DEFAULT_WORK_NUM);
        m_worker_num = DEFAULT_WORK_NUM;
    }
    logdbg("worker:%d\n", m_worker_num);

    child_element = root_element->FirstChildElement("keep_alive");
    if (!child_element)
    {
        logerr("read keep_alive not found!\n");
        return -1;
    }
    m_keep_alive = atoi(child_element->GetText());
    logdbg("keep-alive:%d\n", m_keep_alive);

    child_element = root_element->FirstChildElement("asure_count");
    if (!child_element)
    {
        logerr("read asure_count not found!\n");
        return -1;
    }
    m_asure_count = atoi(child_element->GetText());
    logdbg("asure_count:%d\n", m_asure_count);

    child_element = root_element->FirstChildElement("lifetime");
    if (!child_element)
    {
        logerr("read lifetime not found!\n");
        return -1;
    }
    m_lifetime = atoi(child_element->GetText());
    logdbg("lifetime:%d\n", m_lifetime);


    return 0;
}

Config *glb_config()
{
    static Config s_instance;
    return &s_instance;
}



