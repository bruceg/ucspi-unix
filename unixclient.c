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

extern void setup_env(int, const char*);

#ifndef SUN_LEN
/* Evaluate to actual length of the `sockaddr_un' structure.  */
#define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path)	      \
		      + strlen ((ptr)->sun_path))
#endif

static const char* argv0;
static const char* opt_socket;
static char** command_argv;

void die(const char* msg)
{
  perror(msg);
  exit(1);
}

int do_connect(void)
{
  size_t size;
  struct sockaddr_un* saddr;
  int s = socket(AF_UNIX, SOCK_STREAM, 0);
  if(s == -1)
    die("socket");
  size = sizeof(struct sockaddr_un) + strlen(opt_socket)+1;
  saddr = (struct sockaddr_un*)malloc(size);
  saddr->sun_family = AF_UNIX;
  strcpy(saddr->sun_path, opt_socket);
  if(connect(s, (struct sockaddr*)saddr, SUN_LEN(saddr)) == -1)
    die("connect");
  free(saddr);
  return s;
}

void usage(const char* message)
{
  if(message)
    fprintf(stderr, "%s: %s\n", argv0, message);
  fprintf(stderr, "usage: %s [options] socket program\n"
	  "  -q           Quiet. Do not print any messages.\n"
	  "  -Q (default) Print error messages.\n"
	  "  -v           Verbose.  Print error and status messages.\n"
	  "  -c N         Do not handle more than N simultaneous connections.\n"
	  "               (default 10)\n", argv0);
  exit(1);
}

void parse_options(int argc, char* argv[])
{
  argv0 = argv[0];
  ++argv;
  --argc;
  if(argc < 2)
    usage(0);
  opt_socket = argv[0];
  command_argv = argv + 1;
}

void exec_program(int fd)
{
  setup_env(fd, opt_socket);
  if(dup2(fd, 6) == -1 || dup2(fd, 7) == -1)
    die("dup2");
  close(fd);
  execvp(command_argv[0], command_argv);
  die("execvp");
}

int main(int argc, char* argv[])
{
  int fd;
  parse_options(argc, argv);
  fd = do_connect();
  exec_program(fd);
  return 1;
}
