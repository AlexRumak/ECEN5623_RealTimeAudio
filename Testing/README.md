# Testing

This directory is for C / C++ files that are testing simple concepts, for proof of concept and later integration
into the full project.

It can be later removed in the final project.

## Tuning Linux:

Tuning did not work on CPU core 3, so we isolated the core by updating 
`/boot/firmware/cmdline.txt` on our rasppi to include `isolcpus=1-3`, and
then rebooted our rasppi.

We also set the following parameters:
`isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3 kthread_cpus=0-1 nosoftlockup rcu_nocb_poll`

Each of these parameters explained:
1. `rcu_nocb_poll`: 
2. `nosoftlockup`: prevents the kernel from doing 120 second soft lockup

Another thing we can set is the performance governor to `performance`:

You can also write to `/proc/sys/kernel/sched_rt_runtime_us` to set the runtime maximum for the real-time scheduler.
```sh
echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us
```

If you need to read about LINUX, see 
[LINUX documentation](https://github.com/torvalds/linux/blob/master/Documentation/).
There is also [The Linux Kernel](https://www.kernel.org/doc/html/v5.5/process/volatile-considered-harmful.html)
documentation.