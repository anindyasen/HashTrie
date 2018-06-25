#ifndef USERPLANE_LOCK_RCU_HPP_
#define USERPLANE_LOCK_RCU_HPP_

/**
 * This is a Read Copy Update type lock
 *
 */
#include <iostream>
#include <atomic>
#include <thread>  //  NOLINT
#include <type_traits>


namespace lock {
const uint32_t kRCUPauseRepeatCount       =  0x0;     /* Repeat Pause and then yield */

class RCU {
 public:
    inline RCU() : cntr_(0) { }
    virtual ~RCU() {
    }

    inline void rcu_read_lock(void) {
        cntr_.fetch_add(1, std::memory_order_acq_rel);
    }
    inline void rcu_read_unlock(void) {
        cntr_.fetch_sub(1, std::memory_order_acq_rel);
    }
    inline void synchronize_rcu(void) {
        unsigned rep = 0;
        while (1) {
            if (0 == cntr_.load(std::memory_order_acquire)) {
                break;
            } else {
                utils::pause();
                if (kRCUPauseRepeatCount &&
                    ++rep == kRCUPauseRepeatCount) {
                    rep = 0;
                    std::this_thread::yield();
                }
            }
        }
    }

 private:
    std::atomic<uint64_t> cntr_;
};

template <typename T>
class RCUProtected : public RCU {
 public:
    RCUProtected() {}
    inline explicit RCUProtected(uint8_t coreID) : RCU() {
        data_ptr_ = NULL;
        coreId = coreID;
    }
    virtual ~RCUProtected() {
        T* old_data_ptr = data_ptr_.load(std::memory_order_relaxed);
        data_ptr_.store(0, std::memory_order_relaxed);
        synchronize_rcu();
        delete old_data_ptr;
    }
    inline void setCoreId(uint8_t coreID) {
        coreId = coreID;
    }

    inline T* get_reading_copy_protected() {
        rcu_read_lock();
        return (data_ptr_.load(std::memory_order_acquire));
    }

    inline void initialize_reading() {
        rcu_read_lock();
    }

    inline T* get_reading_copy() {
        return (data_ptr_.load(std::memory_order_acquire));
    }

    inline void finalize_reading() {
        rcu_read_unlock();
    }

    inline T* get_updating_copy() {
        return data_ptr_.load(std::memory_order_relaxed);
    }

    inline T* update(T* new_data_ptr) {
        T* old_data_ptr = data_ptr_.load(std::memory_order_relaxed);
        data_ptr_.store(new_data_ptr, std::memory_order_relaxed);
        return old_data_ptr;
    }

    inline void sync_update(T* new_data_ptr) {
        T* old_data_ptr = data_ptr_.load(std::memory_order_relaxed);
        data_ptr_.store(new_data_ptr, std::memory_order_relaxed);
        synchronize_rcu();
        delete old_data_ptr;
    }

    inline void synchronize_writing() {
        synchronize_rcu();
    }

 private:
    std::atomic<T*> data_ptr_;
    uint8_t coreId;
};



template <typename T>
class RCUProtectedTwoCopy : public RCU {
 public:
    RCUProtectedTwoCopy() = delete;
    inline RCUProtectedTwoCopy(T* ptr1, T* ptr2) : RCU() {
       data_ptr1_ = ptr1;
       data_ptr2_ = ptr2;
       data_ptr_.store(data_ptr1_, std::memory_order_relaxed);
    }
    virtual ~RCUProtectedTwoCopy() {
       data_ptr1_ = nullptr;
       data_ptr2_ = nullptr;
       data_ptr_ = nullptr;
    }
    //  Read Call Sequence
    //  ptr = get_reading_copy_protected();
    //  Read ptr
    //  finalize_reading

    inline const T* get_reading_copy_protected() {
       rcu_read_lock();
       return (data_ptr_.load(std::memory_order_acquire));
    }

    inline void finalize_reading() {
       rcu_read_unlock();
    }
    //  Or else
    //  initialize_reading
    //  get_reading_copy
    //  finalize_reading
    inline void initialize_reading() {
       rcu_read_lock();
    }

    inline const T* get_reading_copy() {
       return (data_ptr_.load(std::memory_order_acquire));
    }

    //  Write Call Sequence
    //  ptr = get_updating_copy1();
    //  Modify ptr
    //  ptr = get_updating_copy2(ptr)
    //  Modify ptr
    inline T* get_updating_copy1() {
       return data_ptr2_;
    }

    inline T* get_updating_copy2(void) {
       data_ptr_.store(data_ptr2_, std::memory_order_relaxed);
       synchronize_rcu();
       return data_ptr1_;
    }

 private:
    inline void rcu_read_lock(void) {
       cntr_.fetch_add(1, std::memory_order_acq_rel);
    }
    inline void rcu_read_unlock(void) {
       cntr_.fetch_sub(1, std::memory_order_acq_rel);
    }
    inline void synchronize_rcu(void) {
       unsigned rep = 0;
       while (1) {
           if (0 == cntr_.load(std::memory_order_acquire)) {
               break;
           } else {
               utils::pause();
               if (kRCUPauseRepeatCount &&
                   ++rep == kRCUPauseRepeatCount) {
                   rep = 0;
                   std::this_thread::yield();
               }
           }
       }
    }
    T* data_ptr1_;
    T* data_ptr2_;
    std::atomic<T*> data_ptr_;
};
}  //  namespace lock
#endif  // USERPLANE_LOCK_RCU_HPP_
