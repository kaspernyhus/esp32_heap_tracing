target remote :3333

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