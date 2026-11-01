# In setup(), if we run the xrandr autostart command synchronously:
# long pid = sys_fork();
# if (pid == 0) { sys_execve("/bin/sh", (char *const *)autostart_cmd, envp_global); sys_exit(1); }
# long st; sys_wait4(pid, &st, 0, 0);
