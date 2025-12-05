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
#include <errno.h>

// macOS interpose macro
#define DYLD_INTERPOSE(_replacement, _original) \
    __attribute__((used)) static struct { \
        const void* replacement; \
        const void* original; \
    } _interpose_##_original __attribute__((section("__DATA,__interpose"))) = { \
        (const void*)(unsigned long)&_replacement, \
        (const void*)(unsigned long)&_original \
    };

// Returns 1 if allowed, 0 if denied, -1 on error
static int ask_controller(const char *etype, const char *detail) {
    const char *sock_path = getenv("SAFERUN_SOCK");
    if (!sock_path) {
        fprintf(stderr, "[shim] SAFERUN_SOCK not set, allowing by default\n");
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "[shim] socket() failed: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[shim] connect() to controller failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    char msg[512];
    snprintf(msg, sizeof(msg), "{\"type\":\"%s\",\"detail\":\"%s\",\"pid\":%d}\n",
             etype, detail ? detail : "", (int)getpid());

    ssize_t sent = send(fd, msg, strlen(msg), 0);
    if (sent < 0) {
        fprintf(stderr, "[shim] send() failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    char resp[256];
    memset(resp, 0, sizeof(resp));
    ssize_t n = recv(fd, resp, sizeof(resp) - 1, 0);
    close(fd);

    if (n <= 0) {
        fprintf(stderr, "[shim] recv() failed or empty response\n");
        return -1;
    }

    fprintf(stderr, "[shim] verdict for %s %s: %s", etype, detail ? detail : "", resp);

    if (strstr(resp, "\"allow\"")) {
        return 1;
    } else if (strstr(resp, "\"deny\"")) {
        return 0;
    }

    fprintf(stderr, "[shim] unknown verdict format, allowing\n");
    return 1;
}

__attribute__((constructor))
static void init_hooks(void) {
    fprintf(stderr, "[shim] loaded, pid=%d\n", (int)getpid());
}

// Replacement functions - call original via the real symbol
int my_open(const char *pathname, int flags, ...) {
    fprintf(stderr, "[shim] open() called: path=%s\n", pathname ? pathname : "(null)");

    if (pathname) {
        int verdict = ask_controller("file", pathname);
        if (verdict == 0) {
            fprintf(stderr, "[shim] BLOCKED open(%s)\n", pathname);
            errno = EACCES;
            return -1;
        }
    }

    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
        return open(pathname, flags, mode);
    }
    return open(pathname, flags);
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    fprintf(stderr, "[shim] connect() called\n");

    // Skip AF_UNIX (our own controller connections)
    if (addr && addr->sa_family == AF_UNIX) {
        return connect(sockfd, addr, addrlen);
    }

    if (addr && addr->sa_family == AF_INET) {
        const struct sockaddr_in *in = (const struct sockaddr_in*)addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &in->sin_addr, ip, sizeof(ip));
        char detail[64];
        snprintf(detail, sizeof(detail), "%s:%d", ip, ntohs(in->sin_port));

        int verdict = ask_controller("net", detail);
        if (verdict == 0) {
            fprintf(stderr, "[shim] BLOCKED connect(%s)\n", detail);
            errno = EACCES;
            return -1;
        }
    }

    return connect(sockfd, addr, addrlen);
}

// Register interposers
DYLD_INTERPOSE(my_open, open)
DYLD_INTERPOSE(my_connect, connect)