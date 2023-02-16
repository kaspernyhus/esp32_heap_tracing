# Heap tracing to host example


[link to documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32s3/api-reference/system/heap_debug.html#host-based-mode)

### Prepare project
---
menuconfig -> Component config -> Application level Tracing -> Data Destination -> JTAG
menuconfig -> Component config -> Application level Tracing -> Data Destination -> FreeRTOS SystemView Tracing -> Enable
menuconfig -> Component config -> Heap memory debugging -> Heap tracing -> Host-Based

Must be called early
```c
ESP_ERROR_CHECK(heap_trace_init_tohost());
ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_ALL));
```


### How to
---
1. put gdbinit file in project root

### gdbinit
```config
target extended-remote :3333

mon reset halt
maintenance flush register-cache

tb heap_trace_start
commands
mon esp sysview start file:///tmp/heap_cpu0.svdat file:///tmp/heap_cpu1.svdat
c
end

tb heap_trace_stop
commands
mon esp sysview stop
end

c
```


2. Run OpenOCD with correct board config file
3. Run gdb
```shell
xtensa-esp32s3-elf-gdb -x gdbinit </path/to/elf>
```

4. Start tracing
``` gdb
mon esp sysview start file:///tmp/heap_cpu0.svdat file:///tmp/heap_cpu1.svdat
```

5. Stop tracing
```gdb
mon esp sysview stop
```


### Interpret data
---
```bash
$IDF_PATH/tools/esp_app_trace/sysviewtrace_proc.py -b esp/heap_trace_host_example/build/heap-trace-host-example.elf /tmp/heap_cpu0.svdat
```



