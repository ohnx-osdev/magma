// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>

// TODO(MA-5): disable logging by default
#define DLOG_ENABLE

#ifdef DLOG_ENABLE
#define DLOG(...)                                                                                  \
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__);                                                 \
    fprintf(stderr, __VA_ARGS__);
#else
#define DLOG
#endif
