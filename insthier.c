#include <installer.h>
#include "conf_bin.c"
#include "conf_man.c"

void insthier(void)
{
  int bin = opendir(conf_bin);
  int man = opendir(conf_man);
  int man1;

  c(bin, "unixcat",    -1, -1, 0755);
  c(bin, "unixclient", -1, -1, 0755);
  c(bin, "unixserver", -1, -1, 0755);

  man1 = d(man, "man1", -1, -1, 0755);
  c(man1, "unixclient.1", -1, -1, 0644);
  c(man1, "unixserver.1", -1, -1, 0644);
}
