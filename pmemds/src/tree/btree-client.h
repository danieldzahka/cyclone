#ifndef PMEMDS_BTREE_CLIENT_H
#define PMEMDS_BTREE_CLIENT_H

#include "../pmemds-client.h"
#include "../pmemds_log.h"

namespace pmemdsclient{

    class BTreeEngine:public PMEngine{

    public:
        BTreeEngine(PMClient *handle, const std::string &path, size_t size, unsigned long core_mask);
        ~BTreeEngine();


        int create(uint8_t flags);
        int remove();
        std::string get(const unsigned long key);
        int put(const unsigned long key, const std::string& value);

    private:
        PMClient *client;
        std::string ds_name;
        std::string path;
        size_t size;
        unsigned long core_mask;
    };
}

#endif //PMEMDS_BTREE_CLIENT_H
