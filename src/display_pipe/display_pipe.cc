// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h> // for close

#include <magenta/syscalls.h>
#include <mx/vmar.h>
#include <mx/vmo.h>

#include "lib/app/cpp/application_context.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/log_settings_command_line.h"
#include "lib/ftl/logging.h"
#include "lib/mtl/tasks/message_loop.h"
#include "magma.h"
#include "magma_util/macros.h"
#include "magma_util/platform/magenta/magenta_platform_ioctl.h"

#include "display_provider_impl.h"
#include "magma_connection.h"

namespace display_pipe {

class App {
 public:
  App() : context_(app::ApplicationContext::CreateFromStartupInfo()) {
    context_->outgoing_services()->AddService<DisplayProvider>(
        [this](fidl::InterfaceRequest<DisplayProvider> request) {
          display_provider_.AddBinding(std::move(request));
        });
  }

 private:
  std::unique_ptr<app::ApplicationContext> context_;
  DisplayProviderImpl display_provider_;

  FTL_DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace display_pipe

int main(int argc, const char** argv) {
  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);
  if (!ftl::SetLogSettingsFromCommandLine(command_line))
    return 1;

  FTL_DLOG(INFO) << "display_pipe started";
  mtl::MessageLoop loop;
  display_pipe::App app;
  loop.Run();
  return 0;
}
