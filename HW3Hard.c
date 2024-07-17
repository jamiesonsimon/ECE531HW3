// Jamieson Simon - ECE 531

// To compile for non-ARM:  gcc -o HW3h HW3Hard.c
// To compile for ARM: arm-linux-gnueabi-gcc -static -o HW3h HW3Hard.c
// Or use the associated make file provided for each envoirnment


// Included Libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

// Error Handling Global Constants
#define OK		0
#define ERR_SETSID	1
#define ERR_FORK	4
#define ERR_CHDIR	5
#define ERR_WTF		9
#define DAEMON_NAME	"ECE531_jsimon_daemon"
#define PID_DIR		"/var/run/jsd"
#define PID_FILE	"/var/run/jsd/ECE531_jsimon_daemon.pid"

// ADDED - Constants to ensure daemon runs as a non-root user
#define UID 1000
#define GID 1000

// Signal Handling Function
static void signal_handler(int signal) {
  switch (signal) {
    case SIGHUP:
      break;
    case SIGTERM:
      syslog(LOG_INFO, "Received SIGTERM, exiting...");
      remove(PID_FILE);
      closelog();
      exit(OK);
      break;
    default:
      syslog(LOG_INFO, "Received unhandled signal!");
  }
}

// FCN: Daemonizing the process rather than in main fcn
void daemonize() {
  pid_t sid;
  int pid_file;

// ADDED - Check if daemon is running as root user
  if (getuid() == 0 || geteuid() == 0) {
    fprintf(stderr, "Cannot run this daemon as root - exiting...\n");
    exit(EXIT_FAILURE);
  }

// Fork off the parent process to make child process
  pid_t pid = fork();

// Check if the PID is the parent, if so error out
  if (pid < 0) {
    exit(ERR_FORK);
  }

// Make sure PID is good before exiting parent process
  if (pid > 0) {
    exit(OK);
  }

// Create a new session ID for child process
  sid = setsid();
  if (sid < 0) {
    exit(ERR_SETSID);
  }

// Close standard input, output and error file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

// Set file mode creation mask
  umask(0);

// Change working directory to root
  if (chdir("/") < 0) {
    exit(ERR_CHDIR);
  }

// Write the PID to the PID file
  char str[10];
  sprintf(str, "%d\n", getpid());
  write(pid_file, str, strlen(str));
  close(pid_file);

// ADDED - Drop the privilages to a non-root user
  if (setgid(GID) != 0 || setuid(UID) != 0) {
    syslog(LOG_ERR, "Failed to drop privilages... exiting");
    exit(EXIT_FAILURE);
  }

// Set up signal handling
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);

}

// FCN: Grabbing current time every second
void get_time() {
  time_t t_now;
  struct tm *timeinfo;
  char buffer[80];

// Get time from system with 'time()' fcn and format using 'strftime'
  while (1) {
    time(&t_now);
    timeinfo = localtime(&t_now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    syslog(LOG_INFO, "Current Local Time: %s", buffer);
    sleep(1);
  }
}

// FCN: Start Daemon
void start() {
  openlog(DAEMON_NAME, LOG_PID | LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON);
  syslog(LOG_INFO, "Starting the JSimon HW3 Daemon");

// Call Daemonize fcn to set up daemon process
  daemonize();

// Call get_time fcn to gather and log time
  get_time();

  syslog(LOG_INFO, "Daemon exiting");
  closelog();
}

// FCN: Stop Daemon
void stop() {
  FILE *pid_file;
  int pid;

  pid_file = fopen(PID_FILE, "w");

// Check if PID file exists
  if (pid_file == NULL) {
    perror("Error opening the PID file");
    exit(EXIT_FAILURE);
  }

// Write PID into file then close
  fscanf(pid_file, "%d", &pid);
  fclose(pid_file);

// Send the SIGTERM signal to kill daemon
  if (kill(pid, SIGTERM) != 0) {
    perror("Error stopping daemon");
    exit(EXIT_FAILURE);
  }

  remove(PID_FILE);
}

// FCN: Restart Daemon
void restart() {
  stop();
  start();
}

// FCN: Main fcn which handles user arguments with fcn calls
int main(int argc, char *argv[]) {
// Show daemon useage if called incorrectly
  if (argc != 2) {
    fprintf(stderr, "Usage is: %s {start|stop|restart}\n", argv[0]);
    exit(EXIT_FAILURE);
  }

// Check method call
  if (strcmp(argv[1], "start") == 0) {
    start();
  } else if (strcmp(argv[1], "stop") == 0) {
      stop();
  } else if (strcmp(argv[1], "restart") == 0) {
      restart();
  } else {
      fprintf(stderr, "Usage is: %s {start|stop|restart}\n", argv[0]);
      exit(EXIT_FAILURE);
  }

  return(OK);
}
