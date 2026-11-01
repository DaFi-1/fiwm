#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                           int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando> [args...]\n", argv[0]);
        return 1;
    }

    struct perf_event_attr pe;
    
    /* Contador 1: CPU Cycles */
    memset(&pe, 0, sizeof(pe));
    pe.size = sizeof(pe);
    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1;

    int fd_cycles = perf_event_open(&pe, 0, -1, -1, 0);

    /* Contador 2: Instructions */
    memset(&pe, 0, sizeof(pe));
    pe.size = sizeof(pe);
    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1;

    int fd_instr = perf_event_open(&pe, 0, -1, -1, 0);

    /* Contador 3: Branch Instructions */
    memset(&pe, 0, sizeof(pe));
    pe.size = sizeof(pe);
    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1;

    int fd_branches = perf_event_open(&pe, 0, -1, -1, 0);

    /* Contador 4: Branch Misses */
    memset(&pe, 0, sizeof(pe));
    pe.size = sizeof(pe);
    pe.type = PERF_TYPE_HARDWARE;
    pe.config = PERF_COUNT_HW_BRANCH_MISSES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1;

    int fd_branch_miss = perf_event_open(&pe, 0, -1, -1, 0);

    /* Iniciar contadores */
    if (fd_cycles >= 0) ioctl(fd_cycles, PERF_EVENT_IOC_ENABLE, 0);
    if (fd_instr >= 0) ioctl(fd_instr, PERF_EVENT_IOC_ENABLE, 0);
    if (fd_branches >= 0) ioctl(fd_branches, PERF_EVENT_IOC_ENABLE, 0);
    if (fd_branch_miss >= 0) ioctl(fd_branch_miss, PERF_EVENT_IOC_ENABLE, 0);

    uint64_t t0 = rdtsc();

    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    uint64_t t1 = rdtsc();

    if (fd_cycles >= 0) ioctl(fd_cycles, PERF_EVENT_IOC_DISABLE, 0);
    if (fd_instr >= 0) ioctl(fd_instr, PERF_EVENT_IOC_DISABLE, 0);
    if (fd_branches >= 0) ioctl(fd_branches, PERF_EVENT_IOC_DISABLE, 0);
    if (fd_branch_miss >= 0) ioctl(fd_branch_miss, PERF_EVENT_IOC_DISABLE, 0);

    uint64_t count_cycles = 0, count_instr = 0, count_branches = 0, count_bmiss = 0;
    if (fd_cycles >= 0) read(fd_cycles, &count_cycles, sizeof(count_cycles));
    if (fd_instr >= 0) read(fd_instr, &count_instr, sizeof(count_instr));
    if (fd_branches >= 0) read(fd_branches, &count_branches, sizeof(count_branches));
    if (fd_branch_miss >= 0) read(fd_branch_miss, &count_bmiss, sizeof(count_bmiss));

    printf("\n=========================================================\n");
    printf("         Hardware PMU Performance Metrics                \n");
    printf("=========================================================\n");
    if (count_cycles > 0)
        printf("  CPU Cycles (PMU)       : %lu\n", (unsigned long)count_cycles);
    if (count_instr > 0) {
        printf("  Instructions (PMU)     : %lu\n", (unsigned long)count_instr);
        if (count_cycles > 0)
            printf("  IPC (Instr / Cycle)    : %.2f\n", (double)count_instr / (double)count_cycles);
    }
    if (count_branches > 0) {
        printf("  Branches (PMU)         : %lu\n", (unsigned long)count_branches);
        if (count_bmiss > 0)
            printf("  Branch Misses (PMU)    : %lu (%.2f%% miss rate)\n",
                   (unsigned long)count_bmiss, (double)count_bmiss * 100.0 / (double)count_branches);
    }
    printf("  RDTSC Wall Cycles      : %lu\n", (unsigned long)(t1 - t0));
    printf("=========================================================\n");

    if (fd_cycles >= 0) close(fd_cycles);
    if (fd_instr >= 0) close(fd_instr);
    if (fd_branches >= 0) close(fd_branches);
    if (fd_branch_miss >= 0) close(fd_branch_miss);

    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
