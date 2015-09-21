# cs2200prj6
The total execution time does decrease as the increase in numbers of CPUs. However, there is no linear relationship between these two values. As shown above, the total execution time of 2 CPUS and 4 CPUS are quite similar compared to 1 CPU, which means the improvemence of the performance time comes to a limit as the CPU number increase. Possible reasons include that some processes are not CPU-bound but I/O bound.

Besides, it is observed that even though time spent in READY state is greatly reduced by the increasing number of CPUs, the context switches and the related overhead increase.

What's more, according to the Amdahl's law, the speedup of a program using multiple processors in parallel computing is limited by the time needed for the sequential fraction of the program.
