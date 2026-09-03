#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


int getentropy(void *buf, size_t buflen) {
  if(!buf) { fprintf(stderr,"buf was NULL!\n"); fflush(stderr); }
  // Fallback: build a low-quality pseudo-entropy loop if needed, 
  // or just zero it out if SDL3 handles empty entropy gracefully.
  unsigned char *p = (unsigned char *)buf;
  for (size_t i = 0; i < buflen; i++) {
p[i] = 0; 
  }
  return 0;
}


int symlink(const char *target, const char *linkpath) {
  fprintf(stderr,"symlink()...\n");  fflush(stderr);
  if(!target) { fprintf(stderr,"target was NULL!\n"); fflush(stderr); }
  if(!linkpath) { fprintf(stderr,"linkpath was NULL!\n"); fflush(stderr); }
  // Classic Mac OS does not natively support symbolic links
  return -1; 
}


int truncate(const char *path, off_t length) {
  fprintf(stderr,"truncate()...\n");  fflush(stderr);
  if(!path) { fprintf(stderr,"path was NULL!\n"); fflush(stderr); }
  // Return error status; replace with custom Mac Toolbox FS calls if required
  return -1; 
}


// Changes permissions relative to a directory file descriptor
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
  fprintf(stderr,"fchmodat()...\n");  fflush(stderr);
  if(!pathname) { fprintf(stderr,"pathname was NULL!\n"); fflush(stderr); }
  // Classic Mac OS does not have directory descriptors or POSIX permissions
  errno = ENOSYS; // Function not implemented
  return -1;
}


// Changes permissions of an open file descriptor
int fchmod(int fd, mode_t mode) {
  fprintf(stderr,"fchmod()...\n");  fflush(stderr);
  errno = ENOSYS;
  return -1;
}


// Gets configuration values for a specific file path (like max filename length)
long pathconf(const char *path, int name) {
  fprintf(stderr,"pathconf()...\n");  fflush(stderr);
  if(!path) { fprintf(stderr,"path was NULL!\n"); fflush(stderr); }
  // Standard behavior when a specific configuration option is not supported
  errno = EINVAL; 
  return -1;
}


// Reads the value of a symbolic link
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  fprintf(stderr,"readlink()...\n");  fflush(stderr);
  if(!pathname) { fprintf(stderr,"pathname was NULL!\n"); fflush(stderr); }
  if(!buf) { fprintf(stderr,"buf was NULL!\n"); fflush(stderr); }
  // Explicitly tell libstdc++ that the path is not a symbolic link
  errno = EINVAL; 
  return -1;
}
