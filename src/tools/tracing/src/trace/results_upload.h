// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_TRACING_SRC_TRACE_RESULTS_UPLOAD_H_
#define APPS_TRACING_SRC_TRACE_RESULTS_UPLOAD_H_

#include <ostream>
#include <string>
#include <vector>

#include "apps/network/services/network_service.fidl.h"
#include "apps/tracing/lib/measure/results.h"
#include "apps/tracing/src/trace/spec.h"
#include "lib/ftl/functional/closure.h"

namespace tracing {

// Parameters of the dashboard upload. All parameters are required.
struct UploadMetadata {
  // Server running the Catapult performance dashboard to be used.
  std::string server_url;
  // Buildbot master name, this is used by dashboard as the top-level part of
  // the test name.
  std::string master;
  // Buildbot builder name, this is used by dashboard as the mid-level part of
  // the test name.
  std::string bot;
  // Test suite name, this is used by dashboard as the last part of the test
  // name.
  std::string test_suite_name;
  // Sequence number identifying the upload. For example, the length of the
  // commit history of the relevant project repo.
  int point_id;
};

void UploadResults(std::ostream& out,
                   std::ostream& err,
                   network::NetworkServicePtr network_service,
                   const UploadMetadata& upload_metadata,
                   const std::vector<measure::Result>& results,
                   ftl::Closure on_done);

}  // namespace tracing

#endif  // APPS_TRACING_SRC_TRACE_RESULTS_UPLOAD_H_
