//
// Find all the required libraries and define all the global constants

#define OK 0


static void _do_work_fcn(void) {
  for(int i = 0; true; i++) {
    syslog(LOG_INFO, "iteration: %d", i);
    sleep(1);
  }
}


static void _signal_handler(const int signal) {
  switch(signal) {
    case SIGHUP:
      break;
    case SIGTERM:
      syslog(LOG_INFO, "received SIGTHERM, exiting.");
      closelog();
      exit(OK);
      break;
    default:
      syslog(LOG_INFO, "received unhandled signal");
  }
}

int main(void) {

  openlog(DAEMON_NAME, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON);
  syslog(LOG_INFO, "starting sampled");

  pid_t pid = fork();

  if(pid < 0) {
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
    return ERR_FORK;
  }

  if(pid > 0) {
    return OK;
  }

  if(setsid() < -1) {
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
    return ERR_SETSID;
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if(chdir("/") < 0) {
    syslog(LOG_ERR, ERROR_FORMAT, strerror(errno));
    return ERR_CHDIR;
  }

  signal(SIGTERM, _signal_handler);
  signal(SIGHUP, _signal_handler);

  _do_work_fcn();

  return ERR_WTF;
}
