#include <cstdlib>

#include "envoy/extensions/resource_monitors/cpu_utilization/v3/cpu_utilization.pb.h"

#include "source/extensions/resource_monitors/cpu_utilization/cpu_stats_reader.h"
#include "source/extensions/resource_monitors/cpu_utilization/linux_cpu_stats_reader.h"

#include "test/test_common/environment.h"

#include "absl/types/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace ResourceMonitors {
namespace CpuUtilizationMonitor {
namespace {

TEST(LinuxCpuStatsReader, ReadsCpuStats) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");
  AtomicFileUpdater file_updater_proc_stat(temp_path_proc_stat);
  const std::string contents_proc_stat =
      R"EOF(cpu  14987204 4857 3003536 11594988 53631 0 759314 2463 0 0
cpu0 1907532 599 369969 1398344 5970 0 121763 18 0 0
cpu1 1883161 620 375962 1448133 5963 0 85914 10 0 0
cpu2 1877318 610 376223 1458160 5713 0 81227 10 0 0
cpu3 1844673 653 373370 1493333 6063 0 80492 1124 0 0
cpu4 1879904 572 380089 1440757 7348 0 91022 10 0 0
cpu5 1873470 607 377632 1449005 6359 0 94092 8 0 0
cpu6 1878276 576 375458 1423527 7995 0 115756 8 0 0
cpu7 1842866 615 374829 1483725 8218 0 89044 1272 0 0
intr 1219233916 0 10 0 0 555 0 0 0 0 0 0 0 154 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 15 12 20859 24167 1125607 1182080 40577 52852415 108966441 51142451 43742777 52285969 56216800 52419266 95242197 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
ctxt 5657586121
btime 1700654901
processes 34530
procs_running 7
procs_blocked 0
softirq 1714610175 0 8718679 72686 1588388544 32 0 293214 77920941 12627 39203452
)EOF";
  file_updater_proc_stat.update(contents_proc_stat);

  LinuxCpuStatsReader cpu_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::HOST,
      temp_path_proc_stat);
  CpuTimes cpu_times = cpu_stats_reader.getCpuTimes();

  EXPECT_EQ(cpu_times.work_time, 17995597);
  EXPECT_EQ(cpu_times.total_time, 29590585);
}

TEST(LinuxCpuStatsReader, CannotReadFile) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat_not_exists");
  LinuxCpuStatsReader cpu_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::HOST,
      temp_path_proc_stat);
  CpuTimes cpu_times = cpu_stats_reader.getCpuTimes();
  EXPECT_FALSE(cpu_times.is_valid);
  EXPECT_EQ(cpu_times.work_time, 0);
  EXPECT_EQ(cpu_times.total_time, 0);
}

TEST(LinuxCpuStatsReader, UnexpectedFormatCpuLine) {
  const std::string temp_path_proc_stat =
      TestEnvironment::temporaryPath("proc_stat_unexpected_format");
  AtomicFileUpdater file_updater_proc_stat(temp_path_proc_stat);
  const std::string contents = R"EOF(cpu0 1907532 599 369969 1398344 5970 0 121763 18 0 0
cpu1 1883161 620 375962 1448133 5963 0 85914 10 0 0
cpu  14987204 4857 3003536 11594988 53631 0 759314 2463 0 0
)EOF";
  file_updater_proc_stat.update(contents);

  LinuxCpuStatsReader cpu_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::HOST,
      temp_path_proc_stat);
  CpuTimes cpu_times = cpu_stats_reader.getCpuTimes();
  EXPECT_FALSE(cpu_times.is_valid);
  EXPECT_EQ(cpu_times.work_time, 0);
  EXPECT_EQ(cpu_times.total_time, 0);
}

TEST(LinuxCpuStatsReader, UnexpectedFormatMissingTokens) {
  const std::string temp_path_proc_stat =
      TestEnvironment::temporaryPath("proc_stat_unexpected_format");
  AtomicFileUpdater file_updater_proc_stat(temp_path_proc_stat);
  const std::string contents_proc_stat = R"EOF(cpu  14987204 4857 3003536
cpu0 1907532 599 369969 1398344 5970 0 121763 18 0 0
cpu1 1883161 620 375962 1448133 5963 0 85914 10 0 0
)EOF";
  file_updater_proc_stat.update(contents_proc_stat);

  LinuxCpuStatsReader cpu_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::HOST,
      temp_path_proc_stat);
  CpuTimes cpu_times = cpu_stats_reader.getCpuTimes();
  EXPECT_FALSE(cpu_times.is_valid);
  EXPECT_EQ(cpu_times.work_time, 0);
  EXPECT_EQ(cpu_times.total_time, 0);
}

TEST(LinuxContainerCpuStatsReader, ReadsCgroupContainerStats) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("cgroup_cpu_allocated_stats");
  AtomicFileUpdater file_updater_cpu_allocated(temp_path_cpu_allocated);
  const std::string mock_contents_cpu_allocated = R"EOF(1000
)EOF";
  file_updater_cpu_allocated.update(mock_contents_cpu_allocated);

  const std::string temp_path_cpu_times = TestEnvironment::temporaryPath("cgroup_cpu_times_stats");
  AtomicFileUpdater file_updater_cpu_times(temp_path_cpu_times);
  const std::string mock_contents_cpu_times = R"EOF(10000000000
)EOF";
  file_updater_cpu_times.update(mock_contents_cpu_times);

  const std::string temp_path_proc_uptime = TestEnvironment::temporaryPath("linux_uptime");
  AtomicFileUpdater file_updater_linux_uptime(temp_path_proc_uptime);
  const std::string mock_contents_linux_uptime = R"EOF(10000.0 123788.9
)EOF";
  file_updater_linux_uptime.update(mock_contents_linux_uptime);
  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated, temp_path_cpu_times, temp_path_proc_uptime);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_EQ(envoy_container_stats.work_time, 10);
  EXPECT_EQ(envoy_container_stats.total_time, 10000.0);
}

TEST(LinuxContainerCpuStatsReader, CannotReadFileCpuAllocated) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("container_cpu_allocated_not_exists");

  const std::string temp_path_cpu_times = TestEnvironment::temporaryPath("container_cpu_times");
  AtomicFileUpdater file_updater_cpu_times(temp_path_cpu_times);
  const std::string cpu_times_contents = R"EOF(100000
)EOF";
  file_updater_cpu_times.update(cpu_times_contents);

  const std::string temp_path_linux_uptime =
      TestEnvironment::temporaryPath("container_linux_uptime");
  AtomicFileUpdater file_updater_linux_uptime(temp_path_linux_uptime);
  const std::string linux_uptime_contents = R"EOF(notnumb3r
)EOF";
  file_updater_linux_uptime.update(linux_uptime_contents);

  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated, temp_path_cpu_times, temp_path_linux_uptime);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_FALSE(envoy_container_stats.is_valid);
  EXPECT_EQ(envoy_container_stats.work_time, 0);
  EXPECT_EQ(envoy_container_stats.total_time, 0);
}

TEST(LinuxContainerCpuStatsReader, CannotReadFileCpuTimes) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("container_cpu_allocated");
  AtomicFileUpdater file_updater_cpu_allocated(temp_path_cpu_allocated);
  const std::string cpu_allocated_contents = R"EOF(1000101
)EOF";
  file_updater_cpu_allocated.update(cpu_allocated_contents);

  const std::string temp_path_cpu_times =
      TestEnvironment::temporaryPath("container_cpu_times_not_exists");

  const std::string temp_path_linux_uptime =
      TestEnvironment::temporaryPath("container_linux_uptime");
  AtomicFileUpdater file_updater_linux_uptime(temp_path_linux_uptime);
  const std::string linux_uptime_contents = R"EOF(1000.0 123445.0
)EOF";
  file_updater_linux_uptime.update(linux_uptime_contents);

  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated, temp_path_cpu_times, temp_path_linux_uptime);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_FALSE(envoy_container_stats.is_valid);
  EXPECT_EQ(envoy_container_stats.work_time, 0);
  EXPECT_EQ(envoy_container_stats.total_time, 0);
}

TEST(LinuxContainerCpuStatsReader, CannotReadFileLinuxUptime) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("container_cpu_allocated");
  AtomicFileUpdater file_updater_cpu_allocated(temp_path_cpu_allocated);
  const std::string cpu_allocated_contents = R"EOF(1000101
)EOF";
  file_updater_cpu_allocated.update(cpu_allocated_contents);

  const std::string temp_path_cpu_times = TestEnvironment::temporaryPath("container_cpu_times");
  AtomicFileUpdater file_updater_cpu_times(temp_path_cpu_times);
  const std::string cpu_times_contents = R"EOF(100000
)EOF";
  file_updater_cpu_times.update(cpu_times_contents);

  const std::string temp_path_linux_uptime =
      TestEnvironment::temporaryPath("linux_uptime_not_exists");
  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated, temp_path_cpu_times, temp_path_linux_uptime);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_FALSE(envoy_container_stats.is_valid);
  EXPECT_EQ(envoy_container_stats.work_time, 0);
  EXPECT_EQ(envoy_container_stats.total_time, 0);
}

TEST(LinuxContainerCpuStatsReader, UnexpectedFormatCpuAllocatedLine) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("container_cpu_allocated_unexpected_format");
  AtomicFileUpdater file_updater_cpu_allocated(temp_path_cpu_allocated);
  const std::string cpu_allocated_contents = R"EOF(notanumb3r
)EOF";
  file_updater_cpu_allocated.update(cpu_allocated_contents);

  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_FALSE(envoy_container_stats.is_valid);
  EXPECT_EQ(envoy_container_stats.work_time, 0);
  EXPECT_EQ(envoy_container_stats.total_time, 0);
}

TEST(LinuxContainerCpuStatsReader, UnexpectedFormatCpuTimesLine) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("container_cpu_allocated");
  AtomicFileUpdater file_updater_cpu_allocated(temp_path_cpu_allocated);
  const std::string cpu_allocated_contents = R"EOF(1000101
)EOF";
  file_updater_cpu_allocated.update(cpu_allocated_contents);

  const std::string temp_path_cpu_times =
      TestEnvironment::temporaryPath("container_cpu_times_unexpected_format");
  AtomicFileUpdater file_update_cpu_times(temp_path_cpu_times);
  const std::string cpu_times_contents = R"EOF(notanumb3r
)EOF";
  file_update_cpu_times.update(cpu_times_contents);

  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated, temp_path_cpu_times);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_FALSE(envoy_container_stats.is_valid);
  EXPECT_EQ(envoy_container_stats.work_time, 0);
  EXPECT_EQ(envoy_container_stats.total_time, 0);
}

TEST(LinuxContainerCpuStatsReader, UnexpectedFormatLinuxUptime) {
  const std::string temp_path_proc_stat = TestEnvironment::temporaryPath("proc_stat");

  const std::string temp_path_cpu_allocated =
      TestEnvironment::temporaryPath("container_cpu_allocated");
  AtomicFileUpdater file_updater_cpu_allocated(temp_path_cpu_allocated);
  const std::string cpu_allocated_contents = R"EOF(1000101
)EOF";
  file_updater_cpu_allocated.update(cpu_allocated_contents);

  const std::string temp_path_cpu_times = TestEnvironment::temporaryPath("container_cpu_times");
  AtomicFileUpdater file_updater_cpu_times(temp_path_cpu_times);
  const std::string cpu_times_contents = R"EOF(100000
)EOF";
  file_updater_cpu_times.update(cpu_times_contents);

  const std::string temp_path_linux_uptime =
      TestEnvironment::temporaryPath("container_linux_uptime_unexpected_format");
  AtomicFileUpdater file_updater_linux_uptime(temp_path_linux_uptime);
  const std::string linux_uptime_contents = R"EOF(notnumb3r
)EOF";
  file_updater_linux_uptime.update(linux_uptime_contents);

  LinuxCpuStatsReader container_stats_reader(
      envoy::extensions::resource_monitors::cpu_utilization::v3::CpuUtilizationConfig::CONTAINER,
      temp_path_proc_stat, temp_path_cpu_allocated, temp_path_cpu_times, temp_path_linux_uptime);
  CpuTimes envoy_container_stats = container_stats_reader.getCpuTimes();
  EXPECT_FALSE(envoy_container_stats.is_valid);
  EXPECT_EQ(envoy_container_stats.work_time, 0);
  EXPECT_EQ(envoy_container_stats.total_time, 0);
}

} // namespace
} // namespace CpuUtilizationMonitor
} // namespace ResourceMonitors
} // namespace Extensions
} // namespace Envoy
