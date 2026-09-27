#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include "prx/libc/include/Shutdown.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceSystemService/SystemService.hpp"

struct SystemServicePlayerDialogParam {
 uint64_t unknown0;
 uint32_t unknown1;
 uint32_t unknown2;
 uint64_t unknown3;
 uint64_t unknown4;
 uint64_t unknown5;
 uint64_t unknown6;
 uint64_t unknown7;
 uint64_t unknown8;
 uint64_t unknown9;
};

static_assert(sizeof(SystemServicePlayerDialogParam) == 0x48);

extern "C" {

int APS5_VABI sceSystemServiceLoadExec(const char* path, const char* const* arguments) {
    if (!path || !*path) return SYSTEM_SERVICE_ERROR_PARAMETER;
    if (std::strcmp(path, "exit") != 0) {
        NotImplemented_nid_no_patch("sceSystemServiceLoadExec: executable replacement");
    }
    (void)arguments;
    LibcRunShutdown_nid_postfix();
    std::exit(0);
}

int APS5_VABI sceSystemServiceDisableNoticeScreenSkipFlagAutoSet(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceGetDisplaySafeAreaInfo(SystemServiceDisplaySafeAreaInfo* info) {
 (void)info;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceGetHdrToneMapLuminance(SystemServiceHdrToneMapLuminance* luminance) {
 (void)luminance;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceGetNoticeScreenSkipFlag(bool* value) {
 (void)value;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceGetStatus(SystemServiceStatus* status) {
 if (status == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 *status = SystemServiceStatus{};
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceHideSplashScreen(void) {
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceParamGetInt(int paramId, int* value) {
 if (value == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 switch (paramId) {
  case SYSTEM_SERVICE_PARAM_ID_LANG: *value = SYSTEM_SERVICE_PARAM_LANG_ENGLISH_US; break;
  case SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT: *value = SYSTEM_SERVICE_PARAM_DATE_FORMAT_DDMMYYYY; break;
  case SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT: *value = SYSTEM_SERVICE_PARAM_TIME_FORMAT_24HOUR; break;
  case SYSTEM_SERVICE_PARAM_ID_TIME_ZONE: *value = 0; break;
  case SYSTEM_SERVICE_PARAM_ID_SUMMERTIME: *value = 0; break;
  case SYSTEM_SERVICE_PARAM_ID_GAME_PARENTAL_LEVEL: *value = SYSTEM_SERVICE_PARAM_GAME_PARENTAL_OFF; break;
  case SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN: *value = SYSTEM_SERVICE_PARAM_ENTER_BUTTON_CROSS; break;
  default: *value = 0; break;
 }
 return SYSTEM_SERVICE_OK;
}

int APS5_VABI sceSystemServiceParamGetString(int param_id, char* buf, size_t buf_size) {
 (void)param_id;
 (void)buf;
 (void)buf_size;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServicePowerTick(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceReceiveEvent(SystemServiceEvent* event) {
 if (event == nullptr) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 event->event_type = -1;
 std::memset(event->data, 0, sizeof(event->data));
 return SYSTEM_SERVICE_ERROR_NO_EVENT;
}

int APS5_VABI sceSystemServiceReportAbnormalTermination(const void* info) {
 (void)info;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemServiceSetNoticeScreenSkipFlag(void) {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

void APS5_VABI sceSystemServiceInitializePlayerDialogParam(SystemServicePlayerDialogParam* param) {
 if (param == nullptr) {
  return;
 }
 std::memset(param, 0, sizeof(*param));
 param->unknown0 = 0x48;
 param->unknown2 = 0xFFFFFFFFu;
}

int APS5_VABI sceSystemServiceLaunchPlayerDialog(const SystemServicePlayerDialogParam* param) {
 if (param == nullptr || param->unknown2 == 0) {
  return SYSTEM_SERVICE_ERROR_PARAMETER;
 }
 return SYSTEM_SERVICE_OK;
}

}
