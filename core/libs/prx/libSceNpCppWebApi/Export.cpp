#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int NP_WEBAPI2_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80553402u);

constexpr int NP_COMMERCE_DIALOG_RESULT_USER_CANCELED = 1;

constexpr int NP_COMMERCE_DIALOG_STATUS_NONE = 0;
constexpr int NP_COMMERCE_DIALOG_STATUS_FINISHED = 3;

constexpr int NP_COMMERCE_DIALOG_ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);
constexpr int NP_COMMERCE_DIALOG_ERROR_ARG_NULL = static_cast<int>(0x80B80005u);
constexpr int NP_COMMERCE_DIALOG_ERROR_NOT_FINISHED = static_cast<int>(0x80B80006u);
constexpr int NP_COMMERCE_DIALOG_ERROR_PARAM_INVALID = static_cast<int>(0x80B80007u);
constexpr int NP_COMMERCE_DIALOG_ERROR_NOT_RUNNING = static_cast<int>(0x80B80009u);

constexpr int NP_COMMERCE_DIALOG_MODE_CATEGORY = 0;
constexpr int NP_COMMERCE_DIALOG_MODE_PLUS = 5;

struct NpWebApi2PushContextId {
 char uuid[37];
};

struct NpCommerceDialogParam {
 uint8_t base_param[48];
 int32_t size;
 int32_t user_id;
 int32_t mode;
 uint32_t service_label;
 const char* const* targets;
 uint32_t num_targets;
 int32_t reserved0;
 uint64_t features;
 void* user_data;
 uint8_t reserved[32];
};

struct NpCommerceDialogResult {
 int32_t result;
 bool authorized;
 uint8_t reserved0[3];
 void* user_data;
 uint8_t reserved[32];
};

static_assert(sizeof(NpCommerceDialogParam) == 0x80);
static_assert(sizeof(NpCommerceDialogResult) == 0x30);

int g_push_context_callback_id = 0;
int g_dialog_status = NP_COMMERCE_DIALOG_STATUS_NONE;
int g_dialog_result = NP_COMMERCE_DIALOG_RESULT_USER_CANCELED;
void* g_dialog_user_data = nullptr;

}

extern "C" {

int APS5_VABI sceNpWebApi2PushEventCreatePushContext(int user_context_id, const NpWebApi2PushContextId* push_context_id) {
 (void)user_context_id;
 if (push_context_id == nullptr) {
  return NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
 }
 return 0;
}

int APS5_VABI sceNpWebApi2PushEventDeleteFilter(int filter_id) {
 (void)filter_id;
 return 0;
}

int APS5_VABI sceNpWebApi2PushEventRegisterPushContextCallback(int user_context_id, int filter_id, void* callback, void* user_arg) {
 (void)user_context_id;
 (void)filter_id;
 (void)user_arg;
 if (callback == nullptr) {
  return NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
 }
 return ++g_push_context_callback_id;
}

int APS5_VABI sceNpWebApi2PushEventStartPushContextCallback(int user_context_id, const NpWebApi2PushContextId* push_context_id) {
 (void)user_context_id;
 if (push_context_id == nullptr) {
  return NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
 }
 return 0;
}

int APS5_VABI sceNpWebApi2PushEventUnregisterCallback(int user_context_id, int callback_id) {
 (void)user_context_id;
 (void)callback_id;
 return 0;
}

int APS5_VABI sceNpWebApi2PushEventUnregisterPushContextCallback(int user_context_id, const NpWebApi2PushContextId* push_context_id) {
 (void)user_context_id;
 if (push_context_id == nullptr) {
  return NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
 }
 return 0;
}

int APS5_VABI sceNpCommerceDialogOpen(const NpCommerceDialogParam* param) {
 if (param == nullptr) {
  return NP_COMMERCE_DIALOG_ERROR_ARG_NULL;
 }
 if (param->mode < NP_COMMERCE_DIALOG_MODE_CATEGORY || param->mode > NP_COMMERCE_DIALOG_MODE_PLUS) {
  return NP_COMMERCE_DIALOG_ERROR_PARAM_INVALID;
 }
 g_dialog_user_data = param->user_data;
 g_dialog_result = NP_COMMERCE_DIALOG_RESULT_USER_CANCELED;
 g_dialog_status = NP_COMMERCE_DIALOG_STATUS_FINISHED;
 return 0;
}

int APS5_VABI sceNpCommerceDialogClose(void) {
 if (g_dialog_status == NP_COMMERCE_DIALOG_STATUS_NONE) {
  return NP_COMMERCE_DIALOG_ERROR_NOT_INITIALIZED;
 }
 if (g_dialog_status != NP_COMMERCE_DIALOG_STATUS_FINISHED) {
  return NP_COMMERCE_DIALOG_ERROR_NOT_RUNNING;
 }
 g_dialog_status = NP_COMMERCE_DIALOG_STATUS_NONE;
 return 0;
}

int APS5_VABI sceNpCommerceDialogGetResult(NpCommerceDialogResult* result) {
 if (result == nullptr) {
  return NP_COMMERCE_DIALOG_ERROR_ARG_NULL;
 }
 if (g_dialog_status != NP_COMMERCE_DIALOG_STATUS_FINISHED) {
  return NP_COMMERCE_DIALOG_ERROR_NOT_FINISHED;
 }
 result->result = g_dialog_result;
 result->authorized = false;
 result->user_data = g_dialog_user_data;
 return 0;
}

int APS5_VABI sceNpCommerceHidePsStoreIcon(void) {
 return 0;
}

int APS5_VABI sceNpCommerceShowPsStoreIcon(int position) {
 (void)position;
 return 0;
}

}
