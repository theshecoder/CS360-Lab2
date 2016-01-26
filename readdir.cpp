#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
main()
{
  int len;
  DIR *dirp;
  struct dirent *dp;

  dirp = opendir(".");
  while ((dp = readdir(dirp)) != NULL)
    printf("name %s\n", dp->d_name);
  (void)closedir(dirp);
}
