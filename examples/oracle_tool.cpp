// examples/oracle_tool.cpp
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    std::cout << "attempting to open /etc/hosts\n";
    int fd = open("/etc/hosts", O_RDONLY);
    if (fd >= 0) { close(fd); std::cout << "opened /etc/hosts\n"; }
    else { std::cout << "open failed\n"; }

    std::cout << "attempting network connect to 1.2.3.4:80\n";
    int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(80);
    inet_pton(AF_INET, "1.2.3.4", &sa.sin_addr);
    connect(s, (sockaddr*)&sa, sizeof(sa));
    close(s);
    return 0;
}
