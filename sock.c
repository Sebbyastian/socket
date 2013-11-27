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

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sock.h"

sock *sockinfo_add(sockinfo **list, sock *s) {
    size_t old_size = *list ? (*list)->size : 0, new_size = old_size + 1;
    assert(new_size > old_size);

    if ((old_size & new_size) == 0) {
        if (SIZE_MAX / 2 / sizeof s <= new_size) { return NULL; }

        size_t new_capacity = new_size * 2 * sizeof *s;
        assert(new_capacity > new_size);

        void *temp = realloc(*list, sizeof *list + new_capacity);

        if (temp == NULL) { return NULL; }
        *list = temp;
    }

    (*list)->size = new_size;
    (*list)->sock[old_size] = *s;
    return (*list)->sock + old_size;
}

void sockinfo_bind(sockinfo **list, addrinfo *addr, handler *handler) {
    while (addr != NULL) {
        sockfd fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (invalid_sock(fd)) {
            addr = addr->ai_next;
            continue;
        }

        if (!listen(fd, addr) || !set_nonblock(fd) || !sockinfo_add(list, &(sock){ .fd = fd,
                                                                                   .handler = handler })) {
            closesocket(fd);
        }

        addr = addr->ai_next;
    }
}

int sock_accept(sock *s, sockinfo **list, handler *handler) {
    sockfd fd = accept(s->fd);

    if (invalid_sock(fd)) {
        return 0;
    }

    if (!set_nonblock(fd) || !sockinfo_add(list, &(sock){ .fd = fd,
                                                          .handler = handler })) {
        closesocket(fd);
    }

    return 0;
}

int sock_cleanup(sock *s, sockinfo **list) {
    assert(0);
    return 0;
}

int sock_recv(sock *s, sockinfo **list, size_t size) {
    if (s->recvdata_size >= size) {
        return 1;
    }

    if (s->data_capacity < size) {
        size_t sock_index = s - (*list)->sock;
        unsigned char *list_end = (void *) sockinfo_add(list, &(sock) { 0 });
        if (list_end == NULL) {
            return 0;
        }

        s = (*list)->sock + sock_index;

	unsigned char *data_end = s->data + s->data_capacity;
        memmove(data_end + sizeof *s, data_end, list_end - data_end);
        s->data_capacity += sizeof *s;
        return 0;
    }

    int n = recv(s->fd, s->data + s->recvdata_size, size - s->recvdata_size, 0);
    if (n < 0) {
        return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : n;
    }

    s->recvdata_size += n;
    return s->recvdata_size >= size;
}

int sock_send(sock *s, void *data, size_t size) {
    if (s->senddata_size >= size) {
        s->senddata_size = 0;
        return 1;
    }

    int n = send(s->fd, (unsigned char *) data + s->senddata_size, size - s->senddata_size <= INT_MAX ? size - s->senddata_size : INT_MAX, 0);
    if (n < 0) {
        return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : n;
    }

    s->senddata_size += n;
    if (s->senddata_size < size) {
        return 0;
    }

    s->senddata_size = 0;
    return 1;    
}
