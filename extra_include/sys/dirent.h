/* <dirent.h> includes <sys/dirent.h>, which is this file.  On a
   system which supports <dirent.h>, this file is overridden by
   dirent.h in the libc/sys/.../sys directory.  On a system which does
   not support <dirent.h>, we will get this file which tries to find
   any other <dirent.h> which may be lurking around.  If there isn't
   one, the user will get an error indicating that there is no
   <dirent.h>.  */

#ifdef __cplusplus
extern "C" {
#endif
#include_next <dirent.h>
#ifdef __cplusplus
}
#endif
