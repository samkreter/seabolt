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

#ifndef NEO4J_C_DRIVER_BOLT_H
#define NEO4J_C_DRIVER_BOLT_H

#include "packstream.h"

static const ssize_t MAX_CHUNK_SIZE = 65535;
static const ssize_t MAX_MESSAGE_SIZE = 65535;
static const char *END_OF_MESSAGE[] = {0x00, 0x00};

static const char INIT_MESSAGE = 0x01;
static const char RUN_MESSAGE = 0x10;
static const char PULL_ALL_MESSAGE = 0x3F;

static const char SUCCESS_MESSAGE = 0x70;
static const char RECORD_MESSAGE = 0x71;
static const char IGNORED_MESSAGE = 0x7E;
static const char FAILURE_MESSAGE = 0x7F;

struct Bolt
{
    int socket;
    uint32_t version;

    // incoming
    char *read_buffer;
    char *message;
    int message_size;
    int message_field_count;
    char message_signature;
    char *reader;

    // outgoing
    char *write_buffer;
    char *start_of_chunk;
    char *writer;

};

ssize_t bolt_send(Bolt *bolt);

uint32_t bolt_recv_uint32(Bolt *bolt);

size_t bolt_read_chunk_header(Bolt *bolt);

void bolt_read_chunk_data(Bolt *bolt, size_t chunk_size);

bool bolt_recv(Bolt *bolt);

Bolt *bolt_connect(const char *host, const in_port_t port);

void bolt_disconnect(Bolt *bolt);

void bolt_init(Bolt *bolt, const char *user_agent);

void bolt_run(Bolt *bolt, const char *statement, size_t parameter_count, PackStream_Pair *parameters);

void bolt_pull_all(Bolt *bolt);


#endif // NEO4J_C_DRIVER_BOLT_H
