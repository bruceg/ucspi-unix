#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef SUN_LEN
/* Evaluate to actual length of the `sockaddr_un' structure.  */
#define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path)	      \
		      + strlen ((ptr)->sun_path))
#endif

static const char* argv0;
static char** command_argv;

static unsigned forked = 0;

static unsigned opt_quiet = 0;
static unsigned opt_verbose = 1;
static unsigned opt_delete = 1;
static unsigned opt_connections = 10;
static const char* opt_socket;

void usage(const char* message)
{
  if(message)
    fprintf(stderr, "%s: %s\n", argv0, message);
  fprintf(stderr, "usage: %s [options] socket program\n"
	  "  -q           Quiet. Do not print any messages.\n"
	  "  -Q (default) Print error messages.\n"
	  "  -v           Verbose.  Print error and status messages.\n"
	  "  -d           Do not delete the socket file on exit.\n"
	  "  -D (default) Delete the socket file on exit.\n"
	  "  -c N         Do not handle more than N simultaneous connections.\n"
	  "               (default 10)\n", argv0);
  exit(1);
}

void log_status(void)
{
  if(opt_verbose)
    printf("unixserver: status: %d/%d\n", forked, opt_connections);
}

void log_child_exit(pid_t pid, int status)
{
  if(opt_verbose)
    printf("unixserver: end %d status %d\n", pid, status);
  log_status();
}

void log_child_start(pid_t pid)
{
  if(opt_verbose)
    printf("unixserver: pid %d\n", pid);
}

void die(const char* msg)
{
  perror(msg);
  unlink(opt_socket);
  exit(1);
}

void parse_options(int argc, char* argv[])
{
  int opt;
  char* ptr;
  argv0 = argv[0];
  while((opt = getopt(argc, argv, "qQvc:")) != EOF) {
    switch(opt) {
    case 'q': opt_quiet = 1; opt_verbose = 0; break;
    case 'Q': opt_quiet = 0; break;
    case 'v': opt_quiet = 0; opt_verbose = 1; break;
    case 'd': opt_delete = 0; break;
    case 'D': opt_delete = 1; break;
    case 'c':
      opt_connections = strtoul(optarg, &ptr, 10);
      if(*ptr != 0)
	usage("Invalid GID number");
      break;
    default:
      usage(0);
    }
  }
  argc -= optind;
  argv += optind;
  if(argc < 2)
    usage(0);
  opt_socket = argv[0];
  command_argv = argv + 1;
}

int make_socket()
{
  struct sockaddr_un saddr;
  int s;
  saddr.sun_family = AF_UNIX;
  strcpy(saddr.sun_path, opt_socket);
  unlink(opt_socket);
  s = socket(AF_UNIX, SOCK_STREAM, 0);
  if(s < 0)
    die("socket");
  if(bind(s, (struct sockaddr*)&saddr, SUN_LEN(&saddr)) != 0)
    die("bind");
  if(listen(s, 128) != 0)
    die("listen");
  return s;
}

const char* utoa(unsigned long i)
{
#define LONGLEN (sizeof(unsigned long)*4)
  static char buf[LONGLEN];
  char* ptr = buf + LONGLEN;
  if(!i)
    return "0";
  *--ptr = 0;
  while(i) {
    *--ptr = (i % 10) + '0';
    i /= 10;
  }
  return ptr;
}

#ifdef SO_PEERCRED
void setup_env(const struct ucred* peer)
#else
void setup_env()
#endif
{
  setenv("PROTO", "UNIX", 1);
  setenv("UNIXLOCALGID", utoa(getgid()), 1);
  setenv("UNIXLOCALPID", utoa(getpid()), 1);
  setenv("UNIXLOCALPATH", opt_socket, 1);
  setenv("UNIXLOCALUID", utoa(getuid()), 1);
#ifdef SO_PEERCRED
  setenv("UNIXREMOTEEGID", utoa(peer->gid), 1);
  setenv("UNIXREMOTEEUID", utoa(peer->uid), 1);
  setenv("UNIXREMOTEPID", utoa(peer->pid), 1);
#endif
}

void start_child(int fd)
{
#ifdef SO_PEERCRED
  struct ucred peer;
  int optlen;
  if(getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &optlen) == -1) {
    perror("getsockopt");
    exit(-1);
  }
  setup_env(&peer);
#else
  setup_env();
#endif
  close(0);
  close(1);
  if(dup2(fd, 0) == -1 || dup2(fd, 1) == -1) {
    perror("dup2");
    exit(-1);
  }
  close(fd);
  execvp(command_argv[0], command_argv);
  perror("execvp");
  exit(-1);
}

void handle_connection(int s)
{
  int fd;
  pid_t pid;
  do {
    fd = accept(s, NULL, NULL);
    // All the listed error return values are not possible except for
    // buggy code, so just try again if accept fails.
  } while(fd < 0);
  ++forked;
  log_status();
  pid = fork();
  switch(pid) {
  case -1: // Could not fork
    close(fd);
    perror("fork");
    --forked;
    log_status();
    break;
  case 0:
    start_child(fd);
    break;
  default:
    close(fd);
    log_child_start(pid);
  }
}

void handle_children(int sig)
{
  pid_t pid;
  int status;
  while((pid = waitpid(0, &status, WNOHANG | WUNTRACED)) != -1) {
    --forked;
    log_child_exit(pid, status);
  }
  signal(SIGCHLD, handle_children);
}

void handle_child(void)
{
  int status;
  pid_t pid = wait(&status);
  if(pid == -1)
    die("wait");
  --forked;
  log_child_exit(pid, status);
}

void exitfn(void)
{
  if(opt_delete)
    unlink(opt_socket);
}

void handle_intr(int sig)
{
  exit(0);
}

int main(int argc, char* argv[])
{
  int s;
  parse_options(argc, argv);
  signal(SIGCHLD, handle_children);
  signal(SIGINT, handle_intr);
  signal(SIGTERM, handle_intr);
  signal(SIGQUIT, handle_intr);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);
  s = make_socket();
  atexit(exitfn);
  log_status();
  for(;;) {
    if(forked >= opt_connections)
      handle_child();
    else
      handle_connection(s);
  }
}
