// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_SINGLETON_HPP_
#define USERPLANE_SINGLETON_HPP_

template <typename T>
class Singleton {
 public:
    static T& Instance() {
        if (Singleton::instance_ == 0) {
            Singleton::instance_ = CreateInstance();
        }
        return (*Singleton::instance_);
    }

 protected:
    virtual ~Singleton() {
        if (Singleton::instance_ != 0) {
            delete Singleton::instance_;
        }
        Singleton::instance_ = 0;
    }
    inline Singleton() {
        assert(Singleton::instance_ == 0);
        Singleton::instance_ = static_cast<T*>(this);
    }

 private:
    static T* instance_;
    static T* CreateInstance() {
        return new T();
    }
};

template<typename T>
T* Singleton<T>::instance_ = 0;

/**
 *  Usage
 *  using SINGLETONOBJ = Singleton<Target_class>;
 *  SINGLETONOBJ::Instance().target_class_init_func(); will be called once with try/catch
 *  SINGLETONOBJ::Instance().target_class_func();
 */
#endif  // USERPLANE_SINGLETON_HPP_
