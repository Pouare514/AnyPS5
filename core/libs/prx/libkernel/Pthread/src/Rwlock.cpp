#include "../include/Pthread.hpp"
#include "prx/libc/include/General.hpp"
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>

static constexpr int SCE_OK = 0;
static constexpr int SCE_KERNEL_ERROR_ENOMEM = 0x8002000C;
static constexpr int SCE_KERNEL_ERROR_EINVAL = 0x80020016;
static constexpr int SCE_KERNEL_ERROR_EBUSY = 0x80020010;
static constexpr int SCE_KERNEL_ERROR_EPERM = 0x80020001;
static constexpr int SCE_KERNEL_ERROR_ETIMEDOUT = 0x80020062;

extern "C" {

int APS5_VABI scePthreadRwlockattrInit(PthreadRwlockattr* attr) {
    if (!attr) throw std::runtime_error("scePthreadRwlockattrInit: null attr");
    auto* p = new (std::nothrow) PthreadRwlockattrPrivate{};
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    *attr = p;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockattrDestroy(PthreadRwlockattr* attr) {
    if (!attr || !*attr) throw std::runtime_error("scePthreadRwlockattrDestroy: null attr");
    delete *attr;
    *attr = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockattrSettype(PthreadRwlockattr* attr, int type) {
    if (!attr || !*attr) throw std::runtime_error("scePthreadRwlockattrSettype: null attr");
    (*attr)->_type = type;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockattrGettype(const PthreadRwlockattr* attr, int* type) {
    if (!attr || !*attr || !type) throw std::runtime_error("scePthreadRwlockattrGettype: null arg");
    *type = (*attr)->_type;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockInit(PthreadRwlock* rwlock, const PthreadRwlockattr* attr, const char*) {
    if (!rwlock) throw std::runtime_error("scePthreadRwlockInit: null rwlock");
    (void)attr;
    auto* p = new (std::nothrow) PthreadRwlockPrivate{};
    if (!p) return SCE_KERNEL_ERROR_ENOMEM;
    *rwlock = p;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockDestroy(PthreadRwlock* rwlock) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockDestroy: null rwlock");
    delete *rwlock;
    *rwlock = nullptr;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockRdlock(PthreadRwlock* rwlock) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockRdlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::unique_lock<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid == self) {
        ++r->_readers;
        return SCE_OK;
    }
    r->_cv.wait(lk, [&] { return !r->_writer; });
    ++r->_readers;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockTryrdlock(PthreadRwlock* rwlock) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockTryrdlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::lock_guard<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid != self) return SCE_KERNEL_ERROR_EBUSY;
    ++r->_readers;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockTimedrdlock(PthreadRwlock* rwlock, KernelUseconds usec) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockTimedrdlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::unique_lock<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid == self) {
        ++r->_readers;
        return SCE_OK;
    }
    const bool acquired =
        r->_cv.wait_for(lk, std::chrono::microseconds(usec), [&] { return !r->_writer; });
    if (!acquired) return SCE_KERNEL_ERROR_ETIMEDOUT;
    ++r->_readers;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockWrlock(PthreadRwlock* rwlock) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockWrlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::unique_lock<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid == self) {
        ++r->_writerRecursion;
        return SCE_OK;
    }
    r->_cv.wait(lk, [&] { return !r->_writer && r->_readers == 0; });
    r->_writer = true;
    r->_writerTid = self;
    r->_writerRecursion = 1;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockTrywrlock(PthreadRwlock* rwlock) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockTrywrlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::lock_guard<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid == self) {
        ++r->_writerRecursion;
        return SCE_OK;
    }
    if (r->_writer || r->_readers != 0) return SCE_KERNEL_ERROR_EBUSY;
    r->_writer = true;
    r->_writerTid = self;
    r->_writerRecursion = 1;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockTimedwrlock(PthreadRwlock* rwlock, KernelUseconds usec) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockTimedwrlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::unique_lock<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid == self) {
        ++r->_writerRecursion;
        return SCE_OK;
    }
    const bool acquired = r->_cv.wait_for(lk, std::chrono::microseconds(usec),
                                           [&] { return !r->_writer && r->_readers == 0; });
    if (!acquired) return SCE_KERNEL_ERROR_ETIMEDOUT;
    r->_writer = true;
    r->_writerTid = self;
    r->_writerRecursion = 1;
    return SCE_OK;
}

int APS5_VABI scePthreadRwlockUnlock(PthreadRwlock* rwlock) {
    if (!rwlock || !*rwlock) throw std::runtime_error("scePthreadRwlockUnlock: null rwlock");
    auto* r = *rwlock;
    const auto self = std::this_thread::get_id();
    std::lock_guard<std::mutex> lk(r->_mtx);
    if (r->_writer && r->_writerTid == self) {
        if (--r->_writerRecursion == 0) {
            r->_writer = false;
            r->_writerTid = std::thread::id{};
            r->_cv.notify_all();
        }
        return SCE_OK;
    }
    if (r->_readers > 0) {
        if (--r->_readers == 0) r->_cv.notify_all();
        return SCE_OK;
    }
    return SCE_KERNEL_ERROR_EPERM;
}

}
