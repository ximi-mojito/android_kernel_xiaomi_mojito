### fgetpriority

A small LKM to filter sys_getpriority / sys_setpriority / sys_ioprio_get / sys_ioprio_set

Usage (if LKM):

```shell
insmod fgetpriority.ko

```

Usage (if built-in):
- just compile it in :^)

Compatibility:
- CONFIG_KALLSYMS=y
- CONFIG_KALLSYMS_ALL=y (if used as LKM)

