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

function check_root {
  if [ "$(id -u)" -ne 0 ]; then
      echo "This script must be run as root. Please use sudo."
      exit 1
  fi
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

parse_args "$@"

check_root
check_cpu_settings