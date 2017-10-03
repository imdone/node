// Copyright 2009 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_VERSION_H_
#define V8_VERSION_H_

#include "src/base/functional.h"
#include "src/vector.h"

namespace v8 {
namespace internal {

class Version {
 public:
  // Return the various version components.
  static int GetMajor() { return major_; }
  static int GetMinor() { return minor_; }
  static int GetBuild() { return build_; }
  static int GetPatch() { return patch_; }
  static bool IsCandidate() { return candidate_; }
  static uint32_t Hash() {
    return static_cast<uint32_t>(
        base::hash_combine(major_, minor_, build_, patch_));
  }

  // Calculate the V8 version string.
  static void GetString(Vector<char> str);

  // Calculate the SONAME for the V8 shared library.
  static void GetSONAME(Vector<char> str);

  static const char* GetVersion() { return version_string_; }

 private:
  // NOTE: can't make these really const because of test-version.cc. id:3244
  static int major_;
  static int minor_;
  static int build_;
  static int patch_;
  static bool candidate_;
  static const char* soname_;
  static const char* version_string_;

  // In test-version.cc.
  friend void SetVersion(int major, int minor, int build, int patch,
                         bool candidate, const char* soname);
};

}  // namespace internal
}  // namespace v8

#endif  // V8_VERSION_H_
