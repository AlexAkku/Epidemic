#pragma once

#include <stdint.h>

#if defined(_WIN32)
#define EP_BRIDGE_CALL __cdecl
#else
#define EP_BRIDGE_CALL
#endif

// Process-local ABI v1. No pointer, table or session token is persisted.
#define EP_BRIDGE_ABI_VERSION 1u
#define EP_BRIDGE_MAX_BATCH 4096u

#ifdef __cplusplus
extern "C" {
#endif
typedef enum EpBridgeStatus {
    EP_BRIDGE_OK = 0,
    EP_BRIDGE_INVALID_ARGUMENT = 1,
    EP_BRIDGE_ABI_MISMATCH = 2,
    EP_BRIDGE_INVALID_HANDLE = 3,
    EP_BRIDGE_BUSY = 4,
    EP_BRIDGE_WRONG_THREAD = 5,
    EP_BRIDGE_MANAGED_EXCEPTION = 6,
    EP_BRIDGE_NATIVE_FAILURE = 7,
    EP_BRIDGE_NATIVE_EXCEPTION = 8,
    EP_BRIDGE_OVERFLOW = 9,
    EP_BRIDGE_CAPACITY = 10,
    EP_BRIDGE_INVALID_STATE = 11,
    EP_BRIDGE_INVALID_RESPONSE = 12
} EpBridgeStatus;

typedef int32_t(EP_BRIDGE_CALL *EpBridgeNativeBatch)(void *context, const int64_t *input,
                                                     int64_t *output, uint32_t count,
                                                     uint32_t *completed);

typedef struct EpBridgeNativeApi {
    uint32_t size;
    uint32_t version;
    void *context;
    EpBridgeNativeBatch invoke_batch;
} EpBridgeNativeApi;

typedef int32_t(EP_BRIDGE_CALL *EpBridgeOpen)(const EpBridgeNativeApi *native, uint64_t *session);
typedef int32_t(EP_BRIDGE_CALL *EpBridgeBatch)(uint64_t session, const int64_t *input,
                                               int64_t *output, uint32_t count,
                                               uint32_t *completed);
typedef int32_t(EP_BRIDGE_CALL *EpBridgeClose)(uint64_t session);

typedef struct EpBridgeManagedApi {
    uint32_t size;
    uint32_t version;
    EpBridgeOpen open;
    EpBridgeBatch invoke_batch;
    EpBridgeClose close;
} EpBridgeManagedApi;

typedef int32_t(EP_BRIDGE_CALL *EpBridgeGetApi)(uint32_t version, uint32_t capacity,
                                                EpBridgeManagedApi *api);
#ifdef __cplusplus
}

#endif
