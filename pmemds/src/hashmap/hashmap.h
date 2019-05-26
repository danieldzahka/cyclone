
#pragma once

#include "../pmemds.h"
#include "pstring.h"
#include "concurrent_hash_map.hpp"


#define MAX_KEY_SIZE 20
#define MAX_VALUE_SIZE 100

using pmem::obj::pool;
using pmem::obj::persistent_ptr;

namespace pmemds {


	class HashMapEngine : public PMEngine {
  public:
    HashMapEngine(const string& path, size_t size);          // default constructor
    ~HashMapEngine();                                        // default destructor
		const string ENGINE = "hashmap";
		string Engine() final { return ENGINE; }               // engine identifier

		void exec(uint16_t op_name,
				  uint8_t ds_type, std::string ds_id, unsigned long in_key, std::string& in_val, pm_rpc_t *resp);
		void Exists(const unsigned long key,pm_rpc_t *resp) final;              // does key have a value?

    using PMEngine::get;                                   // pass value to callback
    void get(const unsigned long key,pm_rpc_t *resp) final;

    void put(const unsigned long key, const string& value,pm_rpc_t *resp) final;
    void remove(const unsigned long key,pm_rpc_t *resp) final;              // remove value for key
  private:
    HashMapEngine(const HashMapEngine&);
    void operator=(const HashMapEngine&);
		typedef pmem::obj::experimental::concurrent_hash_map<pstring<MAX_KEY_SIZE>, pstring<MAX_VALUE_SIZE>> hashmap_type;
		struct RootData {
			        persistent_ptr<hashmap_type> hashmap_ptr;
							    }; 
    void Recover();                                        // reload state from persistent pool
    pool<RootData> pmpool;                                 // pool for persistent root
    hashmap_type* my_hashmap;
};

}