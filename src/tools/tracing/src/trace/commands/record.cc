// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <unordered_set>

#include "apps/network/services/network_service.fidl.h"
#include "record.h"
#include "../results_output.h"
#include "lib/ftl/files/file.h"
#include "lib/ftl/files/path.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/strings/split_string.h"
#include "lib/ftl/strings/string_number_conversions.h"
#include "lib/mtl/tasks/message_loop.h"

namespace tracing {

namespace {

// Command line options.
const char kSpecFile[] = "spec-file";
const char kCategories[] = "categories";
const char kOutputFile[] = "output-file";
const char kDuration[] = "duration";
const char kDetach[] = "detach";
const char kDecouple[] = "decouple";
const char kBufferSize[] = "buffer-size";
const char kUploadServerUrl[] = "upload-server-url";
const char kUploadMaster[] = "upload-master";
const char kUploadBot[] = "upload-bot";
const char kUploadPointId[] = "upload-point-id";

bool EnsureNonEmpty(std::ostream& err,
                    const ftl::CommandLine& command_line,
                    size_t index) {
  if (command_line.options()[index].value.empty()) {
    err << "--" << command_line.options()[index].name << " can't be empty";
    return false;
  }
  return true;
}

}  // namespace

bool Record::Options::Setup(const ftl::CommandLine& command_line) {
  const std::unordered_set<std::string> known_options = {
      kSpecFile,     kCategories, kOutputFile,   kDuration,
      kDetach,       kDecouple,   kBufferSize,   kUploadServerUrl,
      kUploadMaster, kUploadBot,  kUploadPointId};

  for (auto& option : command_line.options()) {
    if (known_options.count(option.name) == 0) {
      err() << "Unknown option: " << option.name << std::endl;
      return false;
    }
  }

  size_t index = 0;
  // Read the spec file first. Arguments passed on the command line override the
  // spec.
  // --spec-file=<file>
  if (command_line.HasOption(kSpecFile, &index)) {
    std::string spec_file_path = command_line.options()[index].value;
    if (!files::IsFile(spec_file_path)) {
      err() << spec_file_path << " is not a file" << std::endl;
      return false;
    }
    // By default use the name of the tspec file as the test suite name.
    // TODO(ppi): allow specyfing the test suite name in the tspec file.
    upload_metadata.test_suite_name = files::GetBaseName(spec_file_path);

    std::string content;
    if (!files::ReadFileToString(spec_file_path, &content)) {
      err() << "Can't read " << spec_file_path << std::endl;
      return false;
    }

    Spec spec;
    if (!DecodeSpec(content, &spec)) {
      err() << "Can't decode " << spec_file_path << std::endl;
      return false;
    }
    app = std::move(spec.app);
    args = std::move(spec.args);
    categories = std::move(spec.categories);
    duration = std::move(spec.duration);
    measurements = std::move(spec.measurements);
  }

  // --categories=<cat1>,<cat2>,...
  if (command_line.HasOption(kCategories, &index)) {
    categories =
        ftl::SplitStringCopy(command_line.options()[index].value, ",",
                             ftl::kTrimWhitespace, ftl::kSplitWantNonEmpty);
  }

  // --output-file=<file>
  if (command_line.HasOption(kOutputFile, &index)) {
    output_file_name = command_line.options()[index].value;
  }

  // --duration=<seconds>
  if (command_line.HasOption(kDuration, &index)) {
    uint64_t seconds;
    if (!ftl::StringToNumberWithError(command_line.options()[index].value,
                                      &seconds)) {
      err() << "Failed to parse command-line option duration: "
            << command_line.options()[index].value;
      return false;
    }
    duration = ftl::TimeDelta::FromSeconds(seconds);
  }

  // --detach
  detach = command_line.HasOption(kDetach);

  // --decouple
  decouple = command_line.HasOption(kDecouple);

  // --buffer-size=<megabytes>
  if (command_line.HasOption(kBufferSize, &index)) {
    uint32_t megabytes;
    if (!ftl::StringToNumberWithError(command_line.options()[index].value,
                                      &megabytes)) {
      err() << "Failed to parse command-line option buffer-size: "
            << command_line.options()[index].value;
      return false;
    }
    buffer_size_megabytes_hint = megabytes;
  }

  int upload_param_count = 0;
  upload_param_count += command_line.HasOption(kUploadServerUrl);
  upload_param_count += command_line.HasOption(kUploadMaster);
  upload_param_count += command_line.HasOption(kUploadBot);
  upload_param_count += command_line.HasOption(kUploadPointId);
  if (upload_param_count != 0 && upload_param_count != 4) {
    err() << "All of " << kUploadServerUrl << ", " << kUploadMaster << ", "
          << kUploadBot << ", " << kUploadPointId
          << " are required for results upload" << std::endl;
    return false;
  }
  if (upload_param_count > 0) {
    upload_results = true;
  }

  // --upload-server-url
  if (command_line.HasOption(kUploadServerUrl, &index)) {
    if (!EnsureNonEmpty(err(), command_line, index)) {
      return false;
    }
    upload_metadata.server_url = command_line.options()[index].value;
  }

  // --upload-master
  if (command_line.HasOption(kUploadMaster, &index)) {
    if (!EnsureNonEmpty(err(), command_line, index)) {
      return false;
    }
    upload_metadata.master = command_line.options()[index].value;
  }

  // --upload-bot
  if (command_line.HasOption(kUploadBot, &index)) {
    if (!EnsureNonEmpty(err(), command_line, index)) {
      return false;
    }
    upload_metadata.bot = command_line.options()[index].value;
  }

  // --upload-point-id=<integer>
  if (command_line.HasOption(kUploadPointId, &index)) {
    if (!EnsureNonEmpty(err(), command_line, index)) {
      return false;
    }
    uint64_t point_id;
    if (!ftl::StringToNumberWithError(command_line.options()[index].value,
                                      &point_id)) {
      err() << "Failed to parse command-line option upload-point-id: "
            << command_line.options()[index].value;
      return false;
    }
    upload_results = true;
    upload_metadata.point_id = point_id;
  }

  // <command> <args...>
  const auto& positional_args = command_line.positional_args();
  if (!positional_args.empty()) {
    if (!app.empty() || !args.empty()) {
      FTL_LOG(WARNING) << "The app and args passed on the command line"
                       << "override those from the tspec file.";
    }
    app = positional_args[0];
    args = std::vector<std::string>(positional_args.begin() + 1,
                                    positional_args.end());
  }

  return true;
}

Command::Info Record::Describe() {
  return Command::Info{
      [](faux::ApplicationContext* context) {
        return std::make_unique<Record>(context);
      },
      "record",
      "starts tracing and records data",
      {{"spec-file=[none]", "Tracing specification file"},
       {"output-file=[/tmp/trace.json]", "Trace data is stored in this file"},
       {"duration=[10s]",
        "Trace will be active for this long after the session has been "
        "started"},
       {"categories=[\"\"]", "Categories that should be enabled for tracing"},
       {"detach=[false]",
        "Don't stop the traced program when tracing finished"},
       {"decouple=[false]", "Don't stop tracing when the traced program exits"},
       {"buffer-size=[4]",
        "Maximum size of trace buffer for each provider in megabytes"},
       {"upload-server-url=[none]", "Url of the Catapult dashboard server"},
       {"upload-master=[none]", "Name of the buildbot master"},
       {"upload-bot=[none]", "Buildbot builder name"},
       {"upload-point-id=[none]", "Integer identifier of the sample"},
       {"[command args]",
        "Run program before starting trace. The program is terminated when "
        "tracing ends unless --detach is specified"}}};
}

Record::Record(faux::ApplicationContext* context)
    : CommandWithTraceController(context), weak_ptr_factory_(this) {}

void Record::Run(const ftl::CommandLine& command_line) {
  if (!options_.Setup(command_line)) {
    err() << "Error parsing options from command line - aborting" << std::endl;
    exit(1);
  }

  std::ofstream out_file(options_.output_file_name,
                         std::ios_base::out | std::ios_base::trunc);
  if (!out_file.is_open()) {
    err() << "Failed to open " << options_.output_file_name << " for writing"
          << std::endl;
    exit(1);
  }

  exporter_.reset(new ChromiumExporter(std::move(out_file)));
  tracer_.reset(new Tracer(trace_controller().get()));
  if (!options_.measurements.duration.empty()) {
    aggregate_events_ = true;
    measure_duration_.reset(
        new measure::MeasureDuration(options_.measurements.duration));
  }
  if (!options_.measurements.time_between.empty()) {
    aggregate_events_ = true;
    measure_time_between_.reset(
        new measure::MeasureTimeBetween(options_.measurements.time_between));
  }

  tracing_ = true;

  auto trace_options = TraceOptions::New();
  trace_options->categories =
      fidl::Array<fidl::String>::From(options_.categories);
  trace_options->buffer_size_megabytes_hint =
      options_.buffer_size_megabytes_hint;

  tracer_->Start(
      std::move(trace_options),
      [this](const reader::Record& record) {
        exporter_->ExportRecord(record);

        if (aggregate_events_ && record.type() == RecordType::kEvent) {
          events_.push_back(record.GetEvent());
        }
      },
      [](std::string error) { err() << error; },
      [this] {
        if (!options_.app.empty())
          LaunchApp();
        StartTimer();
      },
      [this] { DoneTrace(); });
}

void Record::StopTrace() {
  if (tracing_) {
    out() << "Stopping trace..." << std::endl;
    tracing_ = false;
    tracer_->Stop();
  }
}

void Record::ProcessMeasurements(ftl::Closure on_done) {
  if (!events_.empty()) {
    std::sort(
        std::begin(events_), std::end(events_),
        [](const reader::Record::Event& e1, const reader::Record::Event& e2) {
          return e1.timestamp < e2.timestamp;
        });
  }

  for (const auto& event : events_) {
    if (measure_duration_) {
      measure_duration_->Process(event);
    }
    if (measure_time_between_) {
      measure_time_between_->Process(event);
    }
  }

  std::unordered_map<uint64_t, std::vector<Ticks>> ticks;
  if (measure_duration_) {
    ticks.insert(measure_duration_->results().begin(),
                 measure_duration_->results().end());
  }
  if (measure_time_between_) {
    ticks.insert(measure_time_between_->results().begin(),
                 measure_time_between_->results().end());
  }

  uint64_t ticks_per_second = GetTicksPerSecond();
  FTL_DCHECK(ticks_per_second);
  std::vector<measure::Result> results =
      measure::ComputeResults(options_.measurements, ticks, ticks_per_second);
  OutputResults(out(), results);

  if (options_.upload_results) {
    network::NetworkServicePtr network_service =
        context()->ConnectToEnvironmentService<network::NetworkService>();
    UploadResults(out(), err(), std::move(network_service),
                  options_.upload_metadata, results, std::move(on_done));
  } else {
    on_done();
  }
}

void Record::DoneTrace() {
  tracer_.reset();
  exporter_.reset();

  out() << "Trace file written to " << options_.output_file_name << std::endl;

  if (measure_duration_ || measure_time_between_) {
    ProcessMeasurements([] { mtl::MessageLoop::GetCurrent()->QuitNow(); });
  } else {
    mtl::MessageLoop::GetCurrent()->QuitNow();
  }
}

void Record::LaunchApp() {
  printf("This tracer can't launch apps\n");
  // auto launch_info = app::ApplicationLaunchInfo::New();
  // launch_info->url = fidl::String::From(options_.app);
  // launch_info->arguments = fidl::Array<fidl::String>::From(options_.args);

  // out() << "Launching " << launch_info->url << std::endl;
  // context()->launcher()->CreateApplication(std::move(launch_info),
  //                                          GetProxy(&application_controller_));
  // application_controller_.set_connection_error_handler([this] {
  //   out() << "Application terminated" << std::endl;
  //   if (!options_.decouple)
  //     StopTrace();
  // });
  // if (options_.detach)
  //   application_controller_->Detach();
}

void Record::StartTimer() {
  mtl::MessageLoop::GetCurrent()->task_runner()->PostDelayedTask(
      [weak = weak_ptr_factory_.GetWeakPtr()] {
        if (weak)
          weak->StopTrace();
      },
      options_.duration);
  out() << "Starting trace; will stop in " << options_.duration.ToSecondsF()
        << " seconds..." << std::endl;
}

}  // namespace tracing
