#include <chrono>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include "SceTypes.hpp"
#include "../include/Pthread.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI scePthreadRwlockInit(PthreadRwlock* rwlock, const PthreadRwlockattr* attr, const char* name);
int APS5_VABI scePthreadRwlockDestroy(PthreadRwlock* rwlock);
int APS5_VABI scePthreadRwlockRdlock(PthreadRwlock* rwlock);
int APS5_VABI scePthreadRwlockTryrdlock(PthreadRwlock* rwlock);
int APS5_VABI scePthreadRwlockTimedrdlock(PthreadRwlock* rwlock, KernelUseconds usec);
int APS5_VABI scePthreadRwlockWrlock(PthreadRwlock* rwlock);
int APS5_VABI scePthreadRwlockTrywrlock(PthreadRwlock* rwlock);
int APS5_VABI scePthreadRwlockTimedwrlock(PthreadRwlock* rwlock, KernelUseconds usec);
int APS5_VABI scePthreadRwlockUnlock(PthreadRwlock* rwlock);
int APS5_VABI scePthreadRwlockattrInit(PthreadRwlockattr* attr);
int APS5_VABI scePthreadRwlockattrDestroy(PthreadRwlockattr* attr);

static KernelUseconds AbsToRelativeUsec(const KernelTimespec* abstime) {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto deadline =
        system_clock::time_point(seconds(abstime->tv_sec) + nanoseconds(abstime->tv_nsec));
    if (deadline <= now) return 0;
    return static_cast<KernelUseconds>(duration_cast<microseconds>(deadline - now).count());
}

int APS5_VABI pthread_rwlock_init_nid_postfix(PthreadRwlock* rwlock, const PthreadRwlockattr* attr) {
    return scePthreadRwlockInit(rwlock, attr, nullptr);
}

int APS5_VABI pthread_rwlock_destroy_nid_postfix(PthreadRwlock* rwlock) {
    return scePthreadRwlockDestroy(rwlock);
}

int APS5_VABI pthread_rwlock_rdlock_nid_postfix(PthreadRwlock* rwlock) {
    return scePthreadRwlockRdlock(rwlock);
}

int APS5_VABI pthread_rwlock_tryrdlock_nid_postfix(PthreadRwlock* rwlock) {
    return scePthreadRwlockTryrdlock(rwlock);
}

int APS5_VABI pthread_rwlock_timedrdlock_nid_postfix(PthreadRwlock* rwlock, const KernelTimespec* abstime) {
    if (!abstime) throw std::runtime_error("pthread_rwlock_timedrdlock: null abstime");
    return scePthreadRwlockTimedrdlock(rwlock, AbsToRelativeUsec(abstime));
}

int APS5_VABI pthread_rwlock_wrlock_nid_postfix(PthreadRwlock* rwlock) {
    return scePthreadRwlockWrlock(rwlock);
}

int APS5_VABI pthread_rwlock_trywrlock_nid_postfix(PthreadRwlock* rwlock) {
    return scePthreadRwlockTrywrlock(rwlock);
}

int APS5_VABI pthread_rwlock_timedwrlock_nid_postfix(PthreadRwlock* rwlock, const KernelTimespec* abstime) {
    if (!abstime) throw std::runtime_error("pthread_rwlock_timedwrlock: null abstime");
    return scePthreadRwlockTimedwrlock(rwlock, AbsToRelativeUsec(abstime));
}

int APS5_VABI pthread_rwlock_unlock_nid_postfix(PthreadRwlock* rwlock) {
    return scePthreadRwlockUnlock(rwlock);
}

int APS5_VABI pthread_rwlockattr_init_nid_postfix(PthreadRwlockattr* attr) {
    return scePthreadRwlockattrInit(attr);
}

int APS5_VABI pthread_rwlockattr_destroy_nid_postfix(PthreadRwlockattr* attr) {
    return scePthreadRwlockattrDestroy(attr);
}

int APS5_VABI pthread_rwlockattr_getpshared_nid_postfix(const PthreadRwlockattr* attr, int* pshared) {
    if (!attr || !*attr || !pshared) throw std::runtime_error("pthread_rwlockattr_getpshared: null arg");
    *pshared = (*attr)->_pshared;
    return 0;
}

int APS5_VABI pthread_rwlockattr_setpshared_nid_postfix(PthreadRwlockattr* attr, int pshared) {
    if (!attr || !*attr) throw std::runtime_error("pthread_rwlockattr_setpshared: null attr");
    (*attr)->_pshared = pshared;
    return 0;
}

}
