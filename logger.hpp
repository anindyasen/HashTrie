// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_LOGGER_HPP_
#define USERPLANE_LOGGER_HPP_

#include <cstdio>
#include <cstdarg>
#include <iostream>

#include <string>

#include "singleton.hpp"

namespace logger {
const uint16_t kMaxLogLen = 512;

class Logger {
 public:
     void operator() (const char *fmt, ... ) {
#ifdef DEBUG
         va_list args;
         char message[logger::kMaxLogLen] = {0};
         va_start(args, fmt);
         vsnprintf(message, logger::kMaxLogLen, fmt, args);
         va_end(args);
         // TODO(nayan) Mutex lock required to be called from multiple threads
         printf("%s", message);
#else
         return;
#endif
     }

     Logger()  = default;

     ~Logger() = default;
};
using LOGGER = Singleton<logger::Logger>;
static logger::Logger LOG = logger::LOGGER::Instance();
}  //  namespace logger
#endif   //  USERPLANE_LOGGER_HPP_
