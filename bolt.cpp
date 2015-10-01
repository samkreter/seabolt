/*
 * Copyright 2015, Nigel Small
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iomanip>

#include "packstream.h"
#include "bolt.h"

using namespace std;

void dump(const char *buffer, ssize_t size)
{
    for (int byte_index = 0; byte_index < size; byte_index++) {
        if (byte_index > 0) {
            cerr << ':';
        }
        int byte_value = (int) buffer[byte_index] & 0xFF;
        if (byte_value < 0x10) {
            cerr << '0' << uppercase << hex << byte_value;
        } else {
            cerr << uppercase << hex << byte_value;
        }
    }
    cerr << endl;
}

void bolt_reset_writer(Bolt *bolt)
{
    bolt->writer = bolt->write_buffer;
}

void bolt_start_chunk(Bolt *bolt)
{
    bolt->start_of_chunk = bolt->writer;
    bolt->writer += 2;
}

void bolt_end_chunk(Bolt *bolt)
{
    size_t send_size = bolt->writer - bolt->start_of_chunk;
    size_t chunk_size = send_size - 2;
    char chunk_header[] = {(char) (chunk_size >> 8), (char) (chunk_size & 0xFF)};
    memcpy(bolt->start_of_chunk, chunk_header, 2);
}

void bolt_end_message(Bolt *bolt)
{
    memcpy(bolt->writer, END_OF_MESSAGE, 2);
    bolt->writer += 2;
}

ssize_t bolt_send_data(Bolt *bolt, const char *buffer, size_t size)
{
    ssize_t sent = send(bolt->socket, buffer, size, 0);
    //cerr << "C: "; dump(buffer, size);
    return sent;
}

// Send all queued messages
ssize_t bolt_send(Bolt *bolt)
{
    ssize_t size = bolt_send_data(bolt, bolt->write_buffer, bolt->writer - bolt->write_buffer);
    bolt_reset_writer(bolt);
    return size;
}

ssize_t bolt_recv_data(Bolt *bolt, void *buffer, size_t size)
{
    ssize_t received = recv(bolt->socket, buffer, size, 0);
    //cerr << "S: "; dump((char *) buffer, size);
    return received;
}

uint32_t bolt_recv_uint32(Bolt *bolt)
{
    unsigned char buffer[4];

    ssize_t received = bolt_recv_data(bolt, buffer, sizeof(buffer));
    if (received < 0) {
        puts("recv failed");
    }

    uint32_t value = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    return value;
}

size_t bolt_read_chunk_header(Bolt *bolt)
{
    char buffer[2];

    ssize_t received = bolt_recv_data(bolt, buffer, sizeof(buffer));
    if (received < 0) {
        puts("recv failed");
    }

    return (uint16_t) (buffer[0] << 8 | buffer[1]);
}

void bolt_read_chunk_data(Bolt *bolt, size_t chunk_size)
{
    bolt_recv_data(bolt, bolt->read_buffer, chunk_size);
}

// Receive the next message
bool bolt_recv(Bolt *bolt)
{
    bolt->message_size = 0;
    size_t chunk_size;
    do {
        chunk_size = bolt_read_chunk_header(bolt);
        if (chunk_size > 0) {
            bolt_read_chunk_data(bolt, chunk_size);
            // TODO check for buffer overflow
            strncpy(bolt->message + bolt->message_size, bolt->read_buffer, chunk_size);
            bolt->message_size += chunk_size;
        }
    } while (chunk_size > 0);
    bolt->reader = bolt->message;
    return packstream_read_structure_header(&bolt->reader, &bolt->message_field_count, &bolt->message_signature);
}

Bolt *bolt_connect(const char *host, const in_port_t port)
{
    Bolt *bolt = new Bolt;
    bolt->write_buffer = new char[MAX_CHUNK_SIZE + 4];
    bolt->read_buffer = new char[MAX_CHUNK_SIZE];
    bolt->message = new char[MAX_MESSAGE_SIZE];

    // Create socket
    bolt->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (bolt->socket == -1) {
        perror("Could not create socket");
        bolt->version = 0;
        return NULL;
    }

    struct sockaddr_in server;

    // TODO: host name resolution
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Connect to remote server
    if (connect(bolt->socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect failed. Error");
        bolt->version = 0;
        return NULL;
    }

    // Perform handshake
    bolt_send_data(bolt, "\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
    bolt->version = bolt_recv_uint32(bolt);

    bolt_reset_writer(bolt);

    return bolt;
}

void bolt_disconnect(Bolt *bolt)
{
    shutdown(bolt->socket, SHUT_RDWR);
}

void bolt_init(Bolt *bolt, const char *user_agent)
{
    bolt_start_chunk(bolt);
    packstream_write_struct_header(&bolt->writer, 1, INIT_MESSAGE);
    packstream_write_text(&bolt->writer, strlen(user_agent), user_agent);
    bolt_end_chunk(bolt);
    bolt_end_message(bolt);
}

void bolt_run(Bolt *bolt, const char *statement, size_t parameter_count, PackStream_Pair *parameters)
{
    bolt_start_chunk(bolt);
    packstream_write_struct_header(&bolt->writer, 2, RUN_MESSAGE);
    packstream_write_text(&bolt->writer, strlen(statement), statement);
    packstream_write_map(&bolt->writer, parameter_count, parameters);
    bolt_end_chunk(bolt);
    bolt_end_message(bolt);
}

void bolt_pull_all(Bolt *bolt)
{
    bolt_start_chunk(bolt);
    packstream_write_struct_header(&bolt->writer, 0, PULL_ALL_MESSAGE);
    bolt_end_chunk(bolt);
    bolt_end_message(bolt);
}
