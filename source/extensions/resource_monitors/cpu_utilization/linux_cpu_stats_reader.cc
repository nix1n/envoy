#include "source/extensions/resource_monitors/cpu_utilization/linux_cpu_stats_reader.h"

#include <algorithm>
#include <vector>

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace CpuUtilizationMonitor {

constexpr uint64_t NUMBER_OF_CPU_TIMES_TO_PARSE =
    4; // we are interested in user, nice, system and idle times.

LinuxCpuStatsReader::LinuxCpuStatsReader(envoy::extensions::resource_monitors::cpu_utilization::v3::
                                             CpuUtilizationConfig::UtilizationComputeStrategy mode,
                                         const std::string& cpu_stats_filename,
                                         const std::string& linux_cgroup_cpu_allocated_file,
                                         const std::string& linux_cgroup_cpu_times_file,
                                         const std::string& linux_uptime_file)
    : mode_(mode), cpu_stats_filename_(cpu_stats_filename),
      linux_cgroup_cpu_allocated_file_(linux_cgroup_cpu_allocated_file),
      linux_cgroup_cpu_times_file_(linux_cgroup_cpu_times_file),
      linux_uptime_file_(linux_uptime_file) {}

CpuTimes LinuxCpuStatsReader::getCpuTimes() {
  if (mode_ ==
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER) {
    return getContainerCpuTimes();
  }
  return getHostCpuTimes();
}

CpuTimes LinuxCpuStatsReader::getHostCpuTimes() {
  // Existing logic for reading host CPU times.
  std::ifstream cpu_stats_file;
  cpu_stats_file.open(cpu_stats_filename_);
  if (!cpu_stats_file.is_open()) {
    ENVOY_LOG_MISC(error, "Can't open linux cpu stats file {}", cpu_stats_filename_);
    return {false, 0, 0};
  }

  // The first 5 bytes should be 'cpu ' without a cpu index.
  std::string buffer(5, '\0');
  cpu_stats_file.read(buffer.data(), 5);
  const std::string target = "cpu  ";
  if (!cpu_stats_file || buffer != target) {
    ENVOY_LOG_MISC(error, "Unexpected format in linux cpu stats file {}", cpu_stats_filename_);
    return {false, 0, 0};
  }

  std::array<uint64_t, NUMBER_OF_CPU_TIMES_TO_PARSE> times;
  for (uint64_t time, i = 0; i < NUMBER_OF_CPU_TIMES_TO_PARSE; ++i) {
    cpu_stats_file >> time;
    if (!cpu_stats_file) {
      ENVOY_LOG_MISC(error, "Unexpected format in linux cpu stats file {}", cpu_stats_filename_);
      return {false, 0, 0};
    }
    times[i] = time;
  }

  double work_time, total_time;
  work_time = times[0] + times[1] + times[2]; // user + nice + system
  total_time = work_time + times[3];          // idle
  return {true, work_time, total_time};
}

CpuTimes LinuxCpuStatsReader::getContainerCpuTimes() {
  std::ifstream cpu_allocated_file, cpu_times_file, linux_uptime_file;
  double cpu_allocated_value, cpu_times_value, linux_uptime_value;

  cpu_allocated_file.open(linux_cgroup_cpu_allocated_file_);
  if (!cpu_allocated_file.is_open()) {
    ENVOY_LOG_MISC(error, "Can't open linux cpu allocated file {}",
                   linux_cgroup_cpu_allocated_file_);
    return {false, 0, 0};
  }

  cpu_times_file.open(linux_cgroup_cpu_times_file_);
  if (!cpu_times_file.is_open()) {
    ENVOY_LOG_MISC(error, "Can't open linux cpu usage seconds file {}",
                   linux_cgroup_cpu_times_file_);
    return {false, 0, 0};
  }

  linux_uptime_file.open(linux_uptime_file_);
  if (!linux_uptime_file.is_open()) {
    ENVOY_LOG_MISC(error, "Can't open linux uptime file {}", linux_uptime_file_);
    return {false, 0, 0};
  }

  cpu_allocated_file >> cpu_allocated_value;
  if (!cpu_allocated_file) {
    ENVOY_LOG_MISC(error, "Unexpected format in linux cpu allocated file {}",
                   linux_cgroup_cpu_allocated_file_);
    return {false, 0, 0};
  }

  cpu_times_file >> cpu_times_value;
  if (!cpu_times_file) {
    ENVOY_LOG_MISC(error, "Unexpected format in linux cpu usage seconds file {}",
                   linux_cgroup_cpu_times_file_);
    return {false, 0, 0};
  }

  linux_uptime_file >> linux_uptime_value; // First value of /proc/uptime is uptime in seconds
  if (!linux_uptime_file) {
    ENVOY_LOG_MISC(error, "Unexpected format in linux proc uptime file {}", linux_uptime_file_);
    return {false, 0, 0};
  }

  return {true, cpu_times_value / (cpu_allocated_value * 1000000), linux_uptime_value};
}

} // namespace CpuUtilizationMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
