# ECEN5623_RealTimeVisualizer

## Authors

1. Ali Sulehria
2. Alexander Rumak
3. Hyounjun Chang

## Hardware Assumptions:
1. Raspberry Pi 4B
2. Real-time cores are cores 2-3
3. Kernel cores are 0-1

## Dependencies:

You must install alsa-lib to compile this project.
```sh
sudo apt install libasound2 libasound2-dev
sudo apt install libfftw3-dev
sudo apt-get install libncurses-dev # Terminal based UI
```

Additionally, you must run `./configure.sh` to set the raspberry pi to the appropriate real time configuration.

Alternatively, you can manually follow these steps:

### Tuning Linux:

**Step 1: Update Boot Config** 

`/boot/firmware/cmdline.txt` on our rasppi to include the following parameters
`isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3 kthread_cpus=0-1 nosoftlockup rcu_nocb_poll`
then rebooted our rasppi.

These parameters do the following:
1. `isolcpus=2-3`: isolates CPU cores 2 and 3 from the kernel scheduler
2. `nohz_full=2-3`: enables full dynticks mode on CPU cores 2 and 3
3. `rcu_nocbs=2-3`: enables RCU no callbacks on CPU cores 2 and 3
4. `kthread_cpus=0-1`: sets the CPU cores for kernel threads to 0 and 1
5. `nosoftlockup`: prevents the kernel from doing 120 second soft lockup
6. `rcu_nocb_poll`: enables RCU no callbacks polling

**Step 2: Set Performance Governor** 
TODO: do this in the program

You can also set the performance governor to `performance` for the real-time core:

```sh
echo performance | sudo tee /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
```

**Step 3: Set Real-time Scheduler**
TODO: do this in the program

You can also write to `/proc/sys/kernel/sched_rt_runtime_us` to prevent the kernel from forcing real-time tasks to share
CPU-time with non-real-time tasks. This is done by setting the runtime maximum for the real-time scheduler to -1

```sh
echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us
```

**Step 4: Other Settings**

Other real-time settings are appropriately set by the program.

**Step 5: Enable SPI Interface and set GPIO pins**
You can enable the SPI interface by running the following command:

```sh
sudo raspi-config
```

Then navigate to `Interfacing Options` -> `SPI` and enable it.

GPIO pin by default is pin 10.

## Done:
1. Create sleep based & isr based sequencer

## Todo:
1. Create Service Deadlines plan
2. Create service implementations and record baseline WCET

## Feature Plans

1. Live audio-input
2. live GPIO PCM (pulse code modulation) output

## Contributing

Please create a branch for your changes and submit a pull request when
the changes are ready to the main branch.
