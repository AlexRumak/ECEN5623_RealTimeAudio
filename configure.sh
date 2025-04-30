#!/bin/bash

# Configuration script for rasppi to ensure that real-time settings are applied
# and that the system is set up for optimal performance.
# This script should be run with root privileges.

# cli options
# -h, --help: Show help message
# -v, --verbose: Enable verbose output

function parse_args {
  while [[ "$#" -gt 0 ]]; do
    case $1 in
      -h|--help) show_help; exit 0 ;;
      -v|--verbose) VERBOSE=true ;;
      *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
  done
}

function show_help {
  echo "Usage: $0 [options]"
  echo
  echo "Options:"
  echo "  -h, --help      Show this help message"
  echo "  -v, --verbose   Enable verbose output"
}

function check_cpu_settings {
  # Check governor for CPU 3 is set to "performance"
  local cpu3_governor
  cpu3_governor=$(cat /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor)
  if [ "$cpu3_governor" != "performance" ]; then
      echo "Setting CPU 3 governor to performance..."
      echo "performance" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
  else
      echo "CPU 3 governor is already set to performance."
  fi
}

function contains() {
  local item="$1"
  shift
  for element; do
    if [[ "$element" == "$item" ]]; then
      return 0
    fi
  done
  return 1
}

function check_boot_config {
  # Check if boot config file exists
  local boot_config="/boot/firmware/cmdline.txt"

  if [ ! -f "$boot_config" ]; then
      echo "Boot config file not found at $boot_config. Please check your system."
      exit 1
  fi

  # check if file contains `isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3 kthread_cpus=0-1 nosoftlockup rcu_nocb_poll`
  # first parse all options in file
  options=()
  while read -r item; do
    options+=("$item")
  done < <(grep -oP '(\S+=\S+)|(\S+)' "$boot_config")

  # check if all options are present
  local required_options=("isolcpus=2-3" "nohz_full=2-3" "rcu_nocbs=2-3" "kthread_cpus=0-1" "nosoftlockup" "rcu_nocb_poll")

  for item in "${required_options[@]}"; do
    if ! contains "$item" "${options[@]}"; then
      echo "$item is not present in boot config. Add it by editing $boot_config."
    else
      echo "$item is already present in boot config."
    fi
  done
}

function check_realtime_settings {
  # echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

  if [ ! -f /proc/sys/kernel/sched_rt_runtime_us ]; then
      echo "sched_rt_runtime_us file not found. Please check your system."
      exit 1
  fi

  if [ $(cat /proc/sys/kernel/sched_rt_runtime_us) -ne -1 ]; then
      echo "Setting sched_rt_runtime_us to -1..."
      echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us
  else
      echo "sched_rt_runtime_us is already set to -1."
  fi
}

parse_args "$@"

check_cpu_settings
check_boot_config
check_realtime_settings

echo "Configuration complete. You may need to reboot your system for it to take effect."