#include <installer.h>
#include "conf_bin.c"

void insthier(void)
{
  int bin = opendir(conf_bin);
  c(bin, "unixcat",    -1, -1, 0755);
  c(bin, "unixclient", -1, -1, 0755);
  c(bin, "unixserver", -1, -1, 0755);
}
