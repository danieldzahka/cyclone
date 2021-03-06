#include "vector/adjvector-client.h"

namespace pmemdsclient{
    int AdjVectorEngine::add_edge(unsigned long from_node, unsigned long to_node) {
        pm_rpc_t *response;
        pm_rpc_t payload = {0,0,"\0"};
        SET_DS_ID(payload.meta, this->ds_id);
        SET_OP_ID(payload.meta,ADD_EDGE);
        SET_TYPE_ID(payload.meta,this->type_id);
        payload.key = from_node;
        payload.key2 = to_node;
        if(!client->sendmsg(&payload,&response,this->core_mask, UPDATE_OPERATION)){
            LOG_ERROR("adjvector add_edge sync call");
        }
        if(STATUS(response->meta) != OK){
            LOG_ERROR("adjvector add_edge");
            return FAILED;
        }
        return OK;
    }

	int AdjVectorEngine::add_edge(unsigned long fromnode_id,
			unsigned long tonode_id, void (*cb)(void *)){
        pm_rpc_t payload = {0,0,"\0"};
        SET_DS_ID(payload.meta, this->ds_id);
        SET_OP_ID(payload.meta,ADD_EDGE);
        SET_TYPE_ID(payload.meta,this->type_id);
        payload.key = fromnode_id;
        payload.key2 = tonode_id;
        if (client->sendmsg_async(&payload, this->core_mask, UPDATE_OPERATION ,cb) != 0) {
            LOG_ERROR("adjvector add_edge async call");
        }
        return OK;
	}

    unsigned long AdjVectorEngine::vertex_outdegree(unsigned long node_id) {
        pm_rpc_t *response;
        pm_rpc_t payload = {0,0,"\0"};
        SET_DS_ID(payload.meta, this->ds_id);
        SET_OP_ID(payload.meta,VERTEX_OUTDEGREE);
        SET_TYPE_ID(payload.meta,this->type_id);
        payload.key = node_id;
        if(!client->sendmsg(&payload,&response,this->core_mask, READ_OPERATION)){
            LOG_ERROR("vertex_outdegree sync call");
        }
        if(STATUS(response->meta) != OK){
            LOG_ERROR("vertex_outdegree");
            return FAILED;
        }
        return response->key;
    }


    int AdjVectorEngine::vertex_outdegree(unsigned long node_id, void (*cb)(void *)){
        pm_rpc_t payload = {0,0,"\0"};
        SET_DS_ID(payload.meta, this->ds_id);
        SET_OP_ID(payload.meta,VERTEX_OUTDEGREE);
        SET_TYPE_ID(payload.meta,this->type_id);
        payload.key = node_id;
        if (client->sendmsg_async(&payload, this->core_mask,READ_OPERATION, cb) != 0) {
            LOG_ERROR("vertex_outdegree async call");
        }
        return OK;
    }


    AdjVectorEngine::AdjVectorEngine(PMClient *handle, const uint16_t ds_id, size_t size, unsigned long core_mask){
        this->ds_id = ds_id;
        this->size = size;
        this->client = handle;
        this->core_mask = core_mask;
        this->type_id = ADJACENCY_VECTOR;
    }

    int AdjVectorEngine::create(uint8_t flags) {
        pm_rpc_t *response;
        pm_rpc_t payload = {0, 0, "\0"};
        SET_OP_ID(payload.meta, CREATE_DS);
        SET_TYPE_ID(payload.meta, this->type_id);
        SET_DS_ID(payload.meta, this->ds_id);
        if (!client->sendmsg(&payload, &response, this->core_mask, UPDATE_OPERATION)) {
            LOG_ERROR("adjvector create");
        }
        if (STATUS(response->meta) != OK) {
            LOG_ERROR("adjvector create");
            return FAILED;
        }
        return OK;
    }

    int AdjVectorEngine::create(uint8_t flags, void (*cb)(void *)) {
        pm_rpc_t payload = {0, 0, "\0"};
        SET_OP_ID(payload.meta, CREATE_DS);
        SET_TYPE_ID(payload.meta, this->type_id);
        SET_DS_ID(payload.meta, this->ds_id);
        if (client->sendmsg_async(&payload, this->core_mask, UPDATE_OPERATION,cb) != 0) {
            LOG_ERROR("adjvector create async call failed");
        }
        return OK;
    }

    int AdjVectorEngine::close() {
        pm_rpc_t *response;
        pm_rpc_t payload = {0, 0, "\0"};
        SET_OP_ID(payload.meta, CLOSE_DS);
        SET_TYPE_ID(payload.meta, this->type_id);
        SET_DS_ID(payload.meta, this->ds_id);
        if (!client->sendmsg(&payload, &response, this->core_mask, UPDATE_OPERATION)) {
            LOG_ERROR("adjvector close");
        }
        if (STATUS(response->meta) != OK) {
            LOG_ERROR("adjvector close");
            return FAILED;
        }
        return OK;
    }

    int AdjVectorEngine::close(void (*cb)(void *)) {
        pm_rpc_t payload = {0, 0, "\0"};
        SET_OP_ID(payload.meta, CLOSE_DS);
        SET_TYPE_ID(payload.meta, this->type_id);
        SET_DS_ID(payload.meta, this->ds_id);
        if (client->sendmsg_async(&payload, this->core_mask,UPDATE_OPERATION ,cb) != 0) {
            LOG_ERROR("adjvector create async call failed");
        }
        return OK;
    }

    int AdjVectorEngine::remove() {
        return 0;
    }

    int AdjVectorEngine::remove(void (*cb)(void *)) {
        return 0;
    }

}
