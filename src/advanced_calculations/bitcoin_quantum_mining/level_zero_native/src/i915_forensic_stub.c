/* C346: Stub forensic pour compilation rapide */
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int i915_forensic_ioctl_impl(int fd, unsigned long request, void *arg, 
                              const char *name, const char *file, int line) {
    int ret = ioctl(fd, request, arg);
    if (ret < 0) {
        fprintf(stderr, "[FORENSIC] %s:%d %s failed: errno=%d (%s)\n",
                file, line, name, errno, strerror(errno));
    }
    return ret;
}

// Made with Bob
