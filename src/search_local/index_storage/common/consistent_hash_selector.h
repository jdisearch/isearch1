/*
 * =====================================================================================
 *
 *       Filename:  consistent_hash_selector.h
 *
 *    Description:  consistent hash selector.
 *
 *        Version:  1.0
 *        Created:  09/08/2020 10:02:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  qiulu, choulu@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */
#ifndef CONSISTENT_HASH_SELECTOR_H__
#define CONSISTENT_HASH_SELECTOR_H__

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "chash.h"

class ConsistentHashSelector
{
public:
    uint32_t Hash(const char *key, int len) { return chash(key, len); }
    const std::string &Select(uint32_t hash);
    const std::string &Select(const char *key, int len) { return Select(Hash(key, len)); }

    static const int VIRTUAL_NODE_COUNT = 100;
    void add_node(const char *name);
    void Clear()
    {
        m_nodes.clear();
        m_nodeNames.clear();
    }

private:
    std::map<uint32_t, int> m_nodes;
    std::vector<std::string> m_nodeNames;
};

#endif
