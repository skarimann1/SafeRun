// examples/spawn_test.cpp
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main() {
    printf("Attempting to run /bin/sh (should be BLOCKED)...\n");
    char *const args[] = {(char*)"/bin/sh", (char*)"-c", (char*)"echo pwned", NULL};
    int result = execve("/bin/sh", args, NULL);
    printf("execve /bin/sh returned %d: %s\n", result, strerror(errno));
    
    printf("\nAttempting to run /bin/ls (should be ALLOWED)...\n");
    char *const args2[] = {(char*)"/bin/ls", (char*)"-la", NULL};
    execve("/bin/ls", args2, NULL);
    perror("execve /bin/ls failed");
    
    return 0;
}