// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Logger implementation that can be shared by all environments
// where enough posix functionality is available.

#ifndef STORAGE_LEVELDB_UTIL_POSIX_LOGGER_H_
#define STORAGE_LEVELDB_UTIL_POSIX_LOGGER_H_

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include <algorithm>

#include "leveldb/env.h"

namespace leveldb {

class PosixLogger : public Logger {
private:
  FILE* file_;
  uint64_t (*gettid_)();

public:
  PosixLogger(FILE* f, uint64_t (*gettid)()) : file_(f), gettid_(gettid) { }

  virtual ~PosixLogger() {
    fclose(file_);
  }

  virtual void Logv(const char* format, va_list ap) {
    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    const time_t seconds = now_tv.tv_sec;
    struct tm t;
    localtime_r(&seconds, &t);

    const uint64_t thread_id = (*gettid_)();

    // We try twice: the first time with a fixed-size stack allocated buffer,
    // and the second time with a much larger dynamically allocated buffer.
    char buffer[512];
    unsigned int iter;
    for(iter = 0; iter < 2; iter++) {
      char *base;
      unsigned int bufsize;
      if(!iter) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 30000;
        base = new char[bufsize];
      }
      char *offset = base;
      char *limit = base + bufsize;

      offset += snprintf(offset, limit - offset, "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llu ",
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
        static_cast<int>(now_tv.tv_usec), static_cast<unsigned long long>(thread_id));

      // Print the message
      if(offset < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        offset += vsnprintf(offset, limit - offset, format, backup_ap);
        va_end(backup_ap);
      }

      // Truncate to available space if necessary
      if(offset >= limit) {
          /* Try again with a larger buffer */
          if(!iter) continue;
          else offset = limit - 1;
      }

      // Add newline if necessary
      if((offset == base) || offset[-1] != '\n') {
        *offset++ = '\n';
      }

      assert(offset <= limit);
      fwrite(base, 1, offset - base, file_);
      fflush(file_);

      /* Delete dynamic buffer if any */
      if(iter) delete[] base;

      break;
    }
  }
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_POSIX_LOGGER_H_
