#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//typedef int (*open_t)(const char*,int,...);
typedef int (*openat_t)(int,const char*,int,...);
typedef int (*connect_t)(int,const struct sockaddr*,socklen_t);

//static open_t real_open = NULL;
static openat_t real_openat = NULL;
static connect_t real_connect = NULL;

static void send_event(const char *etype, const char *detail) {
    const char *sock_path = getenv("SAFERUN_SOCK");
    if (!sock_path) return;

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) return;

    struct sockaddr_un addr;
    memset(&addr,0,sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path)-1);

    char msg[512];
    snprintf(msg, sizeof(msg), "{\"type\":\"%s\",\"detail\":\"%s\",\"pid\":%d}", etype, detail?detail:"", (int)getpid());

    sendto(fd, msg, strlen(msg), 0, (struct sockaddr*)&addr, sizeof(addr));
    close(fd);
}

__attribute__((constructor))
static void init_hooks(void) {
    //real_open = (open_t)dlsym(RTLD_NEXT, "open");
    real_openat = (openat_t)dlsym(RTLD_NEXT, "openat");
    real_connect = (connect_t)dlsym(RTLD_NEXT, "connect");
    // you can dlsym other functions here
}

/*
int open(const char *pathname, int flags, ...) {
    if (pathname) send_event("file", pathname);

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
        return real_open(pathname, flags, mode);
    }
    return real_open(pathname, flags);
}
*/


int openat(int dirfd, const char *pathname, int flags, ...) {
    if (pathname) send_event("file", pathname);

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
        return real_openat(dirfd, pathname, flags, mode);
    }
    return real_openat(dirfd, pathname, flags);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (addr && addr->sa_family == AF_INET) {
        const struct sockaddr_in *in = (const struct sockaddr_in*)addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &in->sin_addr, ip, sizeof(ip));
        char detail[64];
        snprintf(detail, sizeof(detail), "%s:%d", ip, ntohs(in->sin_port));
        send_event("net", detail);
    } else {
        send_event("net", "unknown");
    }
    return real_connect(sockfd, addr, addrlen);
}
