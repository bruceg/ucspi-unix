#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sysdeps.h>

extern void setup_env(int, const char*);

#ifndef SUN_LEN
/* Evaluate to actual length of the `sockaddr_un' structure.  */
#define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path)	      \
		      + strlen ((ptr)->sun_path))
#endif

static const char* argv0;
static char** command_argv;

static unsigned forked = 0;

static unsigned opt_quiet = 0;
static unsigned opt_verbose = 0;
static unsigned opt_delete = 1;
static unsigned opt_connections = 10;
static const char* opt_socket;
static uid_t opt_uid = -1;
static gid_t opt_gid = -1;
static uid_t opt_socket_uid = -1;
static gid_t opt_socket_gid = -1;
static mode_t opt_perms = -1;
static mode_t opt_umask = 0;
static int opt_backlog = 128;
static const char* opt_banner = 0;

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
	  "  -u UID       Change user id to UID after creating socket.\n"
	  "  -g GID       Change group id to GID after creating socket.\n"
	  "  -U           Same as '-u $UID -g $GID'.\n"
	  "  -o UID       Make the socket owned by UID.\n"
	  "  -r GID       Make the socket group owned by GID.\n"
	  "  -O           Same as '-o $SOCKET_UID -? $SOCKET_GID'.\n"
	  "  -p PERM      Set the permissions on the created socket (in octal).\n"
	  "               (defaults to 0666 minus umask)\n"
	  "  -m MASK      Set umask to MASK (in octal) before creating socket.\n"
	  "               (defaults to 0, previous value is restored afterwards)\n"
	  "  -c N         Do not handle more than N simultaneous connections.\n"
	  "               (default 10)\n"
	  "  -b N         Allow a backlog of N connections.\n"
	  "  -B BANNER    Write BANNER to the client immediately after connecting.\n",
	  argv0);
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
  if(opt_delete)
    unlink(opt_socket);
  exit(1);
}

static int parseu(const char* str, unsigned* out, int base)
{
  char* ptr;
  if (!str) return 0;
  *out = strtoul(str, &ptr, base);
  return (*ptr == 0);
}

static void use_uid(const char* str)
{
  if (!str) usage("UID not found in environment.");
  if (!parseu(str, &opt_uid, 10)) usage("Invalid UID number");
}

static void use_gid(const char* str)
{
  if (!str) usage("GID not found in environment.");
  if (!parseu(str, &opt_gid, 10)) usage("Invalid GID number");
}

static void use_socket_uid(const char* str)
{
  if (!str) usage("Socket UID not found in environment.");
  if (!parseu(str, &opt_socket_uid, 10)) usage("Invalid socket UID number");
}

static void use_socket_gid(const char* str)
{
  if (!str) usage("Socket GID not found in environment.");
  if (!parseu(str, &opt_socket_gid, 10)) usage("Invalid socket GID number");
}

void parse_options(int argc, char* argv[])
{
  int opt;
  argv0 = argv[0];
  while((opt = getopt(argc, argv, "qQvc:u:g:Ub:B:m:o:r:Op:")) != EOF) {
    switch(opt) {
    case 'q': opt_quiet = 1; opt_verbose = 0; break;
    case 'Q': opt_quiet = 0; break;
    case 'v': opt_quiet = 0; opt_verbose = 1; break;
    case 'd': opt_delete = 0; break;
    case 'D': opt_delete = 1; break;
    case 'c':
      if (!parseu(optarg, &opt_connections, 10))
	usage("Invalid connection limit number.");
      break;
    case 'u': use_uid(optarg); break;
    case 'g': use_gid(optarg); break;
    case 'U':
      use_uid(getenv("UID"));
      use_gid(getenv("GID"));
      break;
    case 'o': use_socket_uid(optarg); break;
    case 'r': use_socket_gid(optarg); break;
    case 'O':
      use_socket_uid(getenv("SOCKET_UID"));
      use_socket_gid(getenv("SOCKET_GID"));
      break;
    case 'p':
      if (!parseu(optarg, &opt_perms, 8)) usage("Invalid permissions value.");
      break;
    case 'm':
      if (!parseu(optarg, &opt_umask, 8)) usage("Invalid mask value.");
      break;
    case 'b':
      if (!parseu(optarg, &opt_backlog, 10)) usage("Invalid backlog count.");
      break;
    case 'B': opt_banner = optarg; break;
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
  struct sockaddr_un* saddr;
  int s;
  mode_t old_umask = umask(opt_umask);
  saddr = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un) +
				      strlen(opt_socket) + 1);
  saddr->sun_family = AF_UNIX;
  strcpy(saddr->sun_path, opt_socket);
  unlink(opt_socket);
  s = socket(AF_UNIX, SOCK_STREAM, 0);
  if(s < 0)
    die("socket");
  if(bind(s, (struct sockaddr*)saddr, SUN_LEN(saddr)) != 0)
    die("bind");
  if(listen(s, opt_backlog) != 0)
    die("listen");
  if(opt_gid != (gid_t)-1 && setgid(opt_gid) == -1)
    die("setgid");
  if(opt_uid != (uid_t)-1 && setuid(opt_uid) == -1)
    die("setuid");
  umask(old_umask);
  return s;
}

void start_child(int fd)
{
  if(opt_banner)
    write(fd, opt_banner, strlen(opt_banner));
  setup_env(fd, opt_socket);
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

void handle_children()
{
  pid_t pid;
  int status;
#if HASWAITPID
  while((pid = waitpid(0, &status, WNOHANG | WUNTRACED)) > 0) {
    --forked;
    log_child_exit(pid, status);
  }
#else
  pid = wait(&status);
  --forked;
  log_child_exit(pid, status);
#endif
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

void handle_intr()
{
  if(opt_delete)
    unlink(opt_socket);
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
  log_status();
  for(;;) {
    while(forked >= opt_connections)
      handle_child();
    handle_connection(s);
  }
}
