#include "source/extensions/resource_monitors/cpu_utilization/linux_cpu_stats_reader.h"

#include <algorithm>
#include <vector>

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace CpuUtilizationMonitor {

constexpr uint64_t NUMBER_OF_CPU_TIMES_TO_PARSE =
    4; // we are interested in user, nice, system and idle times.

LinuxCpuStatsReader::LinuxCpuStatsReader(const std::string& cpu_stats_filename)
    : cpu_stats_filename_(cpu_stats_filename) {}

CpuTimes LinuxCpuStatsReader::getCpuTimes() {
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

  uint64_t work_time, total_time;
  work_time = times[0] + times[1] + times[2]; // user + nice + system
  total_time = work_time + times[3];          // idle
  return {true, work_time, total_time};
}


LinuxContainerCpuStatsReader::LinuxContainerCpuStatsReader(const std::string& linux_cgroup_cpu_allocated_file, const std::string& linux_cgroup_cpu_times_file)
:linux_cgroup_cpu_allocated_file_(linux_cgroup_cpu_allocated_file),linux_cgroup_cpu_times_file_(linux_cgroup_cpu_times_file){}

CgroupStats LinuxContainerCpuStatsReader::getCgroupStats() {
  std::ifstream cpu_allocated_file, cpu_times_file;
  uint64_t cpu_allocated_value, cpu_times_value;
  bool stats_valid = true;
  cpu_allocated_file.open(linux_cgroup_cpu_allocated_file_);
  if (!cpu_allocated_file.is_open()) {
      ENVOY_LOG_MISC(error, "Can't open linux cpu allocated file {}", linux_cgroup_cpu_allocated_file_);
      stats_valid = false;
      cpu_allocated_value = 0;
  }else{
      cpu_allocated_file >> cpu_allocated_value;
      if (!cpu_allocated_file) {
          ENVOY_LOG_MISC(error, "Unexpected format in linux cpu allocated file {}", linux_cgroup_cpu_allocated_file_);
          stats_valid = false;
          cpu_allocated_value = 0;
      }
  }

  cpu_times_file.open(linux_cgroup_cpu_times_file_);
  if (!cpu_times_file.is_open()) {
      ENVOY_LOG_MISC(error, "Can't open linux cpu usage seconds file {}", linux_cgroup_cpu_times_file_);
      stats_valid = false;
      cpu_times_value = 0;
  }else{
      cpu_times_file >> cpu_times_value;
      if(!cpu_times_file) {
          ENVOY_LOG_MISC(error, "Unexpected format in linux cpu usage seconds file {}", linux_cgroup_cpu_times_file_);
          stats_valid = false;
          cpu_times_value = 0;
      }
  }

  return {stats_valid,cpu_allocated_value, cpu_times_value};
}

} // namespace CpuUtilizationMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
