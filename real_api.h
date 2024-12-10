/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Distributed under BSD 3-Clause license.                                   *
* Copyright by The HDF Group.                                               *
* Copyright by the Illinois Institute of Technology.                        *
* All rights reserved.                                                      *
*                                                                           *
* This file is part of Hermes. The full Hermes copyright notice, including  *
* terms governing use, modification, and redistribution, is contained in    *
* the COPYING file, which can be found at the top directory. If you do not  *
* have access to the file, you may request a copy from help@hdfgroup.org.   *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HERMES_ADAPTER_API_H
#define HERMES_ADAPTER_API_H

#undef DEPRECATED

#include <dlfcn.h>
#include <link.h>
#include <string>
#include <cstring>

#define HERMES_DECL(F) F

#define REQUIRE_API(api_name)                                                  \
  if (!(api_name)) {                                                           \
    printf("Failed to map symbol: %s\n", #api_name);                           \
    exit(1); \
  }

namespace hermes::adapter {

struct RealApiIter {
  const char *symbol_name_;
  const char *is_intercepted_;
  const char *lib_path_;

  RealApiIter(const char *symbol_name, const char *is_intercepted)
      : symbol_name_(symbol_name), is_intercepted_(is_intercepted),
        lib_path_(nullptr) {}
};

struct RealApi {
  void *real_lib_;
  bool is_intercepted_;

  static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto iter = (RealApiIter*)data;
    auto lib = dlopen(info->dlpi_name, RTLD_GLOBAL | RTLD_NOW);
    // auto lib = dlopen(info->dlpi_name, RTLD_NOLOAD | RTLD_NOW);
    auto exists = dlsym(lib, iter->symbol_name_);
    void *is_intercepted =
        (void*)dlsym(lib, iter->is_intercepted_);
    if (!is_intercepted && exists && !iter->lib_path_) {
      iter->lib_path_ = info->dlpi_name;
    }
    return 0;
  }

  RealApi(const char *symbol_name,
          const char *is_intercepted) {
    RealApiIter iter(symbol_name, is_intercepted);
    dl_iterate_phdr(callback, (void*)&iter);
    if (iter.lib_path_) {
      real_lib_ = dlopen(iter.lib_path_, RTLD_GLOBAL | RTLD_NOW);
    }
    void *is_intercepted_ptr = (void*)dlsym(RTLD_DEFAULT,
                                             is_intercepted);
    is_intercepted_ = is_intercepted_ptr != nullptr;
    /* if (is_intercepted_) {
      real_lib_ = RTLD_NEXT;
    } else {
      real_lib_ = RTLD_DEFAULT;
    }*/
  }
};

template<typename PosixT>
struct InterceptorApi {
  PosixT *posix_;
  std::string lib_path_;
  bool is_loaded_;
  int lib_fd_ = -1;

  static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto iter = (RealApiIter*)data;
    auto lib = dlopen(info->dlpi_name, RTLD_GLOBAL | RTLD_NOW);
    // auto lib = dlopen(info->dlpi_name, RTLD_NOLOAD | RTLD_NOW);
    auto exists = dlsym(lib, iter->symbol_name_);
    void *is_intercepted =
        (void*)dlsym(lib, iter->is_intercepted_);
    if (is_intercepted && exists && !iter->lib_path_) {
      iter->lib_path_ = info->dlpi_name;
      if (iter->lib_path_ == nullptr || strlen(iter->lib_path_) == 0) {
        Dl_info dl_info;
        if (dladdr(is_intercepted, &dl_info) == 0) {
          iter->lib_path_ = "";
        } else {
          iter->lib_path_ = dl_info.dli_fname;
        }
      }
    }
    return 0;
  }


  InterceptorApi(const char *symbol_name,
                 const char *is_intercepted) : is_loaded_(false) {
    is_loaded_ = true;
  }
};

}  // namespace hermes::adapter

#undef DEPRECATED

#endif  // HERMES_ADAPTER_API_H
