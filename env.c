#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

extern const char* utoa(unsigned long);

void setup_env(int fd, const char* socket)
{
#ifdef SO_PEERCRED
  struct ucred peer;
  int optlen = sizeof(peer);
  if(getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &optlen) != -1) {
    setenv("UNIXREMOTEEGID", utoa(peer.gid), 1);
    setenv("UNIXREMOTEEUID", utoa(peer.uid), 1);
    setenv("UNIXREMOTEPID", utoa(peer.pid), 1);
  }
#endif
  setenv("PROTO", "UNIX", 1);
  setenv("UNIXLOCALGID", utoa(getgid()), 1);
  setenv("UNIXLOCALPID", utoa(getpid()), 1);
  setenv("UNIXLOCALPATH", socket, 1);
  setenv("UNIXLOCALUID", utoa(getuid()), 1);
}
