#pragma once

#include "target.h"
#include "strings.h"

// #include <string>
// #include <sstream>

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAXLINE 1024 

bool sendUDP(const std::string& _hostname, const std::string& _port, const std::string& _msg) {

    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;

    struct addrinfo *res = NULL;
    int err = getaddrinfo(_hostname.c_str(), _port.c_str(), &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Failed to get address info, error: %d\n", errno);
        return false;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to open UDP socket, error number: %d\n", errno);
        freeaddrinfo(res);
        return false;
    }

    if ( sendto( sockfd, _msg.c_str(), _msg.size(), 0, res->ai_addr, res->ai_addrlen) < 0) {
        printf("%s",strerror(errno));
        return false;
    }

    close(sockfd);
    freeaddrinfo(res);
    return true;
}

template <typename T>
inline void broadcast_UDP(const Target& _target, const std::string& _prop, const T& _value) {
    std::string msg = toString(_value);
    // std::cout << msg << std::endl;
    sendUDP(_target.address, _target.port, msg);
}

