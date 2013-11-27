/* Copyright (c) 2013, Sebastian Ramadan
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of the {organization} nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INCLUDE_SOCK_H
#define INCLUDE_SOCK_H

#ifdef WIN32
#    include <winsock2.h>
#    include <Ws2tcpip.h>
#    define EAGAIN           WSAEWOULDBLOCK
#    define EWOULDBLOCK      WSAEWOULDBLOCK
#    define EISCONN          WSAEISCONN
#    define EINPROGRESS      WSAEINPROGRESS
#    define EINVAL           WSAEINVAL
#    define EALREADY         WSAEALREADY
#    define accept(fd)       (accept(fd, NULL, NULL))
#    define invalid_sock(fd) (fd == INVALID_SOCKET)
#    define listen(fd, addr) (bind(fd, addr->ai_addr, addr->ai_addrlen) == 0 && addr->ai_socktype == SOCK_DGRAM || listen(fd, 0) == 0)
#    define set_nonblock(fd) (ioctlsocket(fd, FIONBIO, (u_long[]){1}) == 0)
typedef SOCKET sockfd;
#else
#    include <netdb.h>
#    include <fcntl.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    define closesocket(fd)  close(fd)
#    define accept(fd)       (accept(fd, NULL, (socklen_t[]) { 0 }))
#    define invalid_sock(fd) (fd < 0)
#    define listen(fd, addr) (bind(fd, addr->ai_addr, addr->ai_addrlen) == 0 && addr->ai_socktype == SOCK_DGRAM || listen(fd, 0) == 0)
#    define set_nonblock(fd) (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) != -1)
typedef int sockfd;
#endif

struct sock;
struct sockinfo;

typedef struct addrinfo addrinfo;

typedef struct sock {
    sockfd fd;

    struct sock *relay;
    int (* handler)(struct sock *, struct sockinfo **);

    size_t data_capacity;
    size_t recvdata_size;
    size_t senddata_size;
    unsigned char data[];
} sock;

typedef struct sockinfo {
    size_t size;
    sock sock[];
} sockinfo;

typedef int handler(sock *, sockinfo **);

sock *sockinfo_add(sockinfo **, sock *);
void sockinfo_bind(sockinfo **, addrinfo *, handler *);

int sock_accept(sock *, sockinfo **, handler *);
int sock_cleanup(sock *, sockinfo **);
int sock_recv(sock *, sockinfo **, size_t);
int sock_send(sock *, void *, size_t);
#endif
