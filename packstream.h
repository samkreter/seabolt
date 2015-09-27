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
#include <cstdbool>

#ifndef NEO4J_C_DRIVER_PACKSTREAM_H
#define NEO4J_C_DRIVER_PACKSTREAM_H

enum PackStream_Type {
    PACKSTREAM_RESERVED = -1,
    PACKSTREAM_NULL = 0,
    PACKSTREAM_BOOLEAN = 1,
    PACKSTREAM_INTEGER = 2,
    PACKSTREAM_FLOAT = 3,
    PACKSTREAM_BYTES = 4,
    PACKSTREAM_TEXT = 5,
    PACKSTREAM_LIST = 6,
    PACKSTREAM_MAP = 7,
    PACKSTREAM_STRUCTURE = 8,
    PACKSTREAM_END_OF_STREAM = 9,
};

struct PackStream_Value {
    PackStream_Type type;
    size_t size;
    void *value;
};

struct PackStream_Pair {
    PackStream_Value name;
    PackStream_Value value;
};

static const char NEO4J_IDENTITY = 'I';
static const char NEO4J_NODE = 'N';
static const char NEO4J_RELATIONSHIP = 'R';
static const char NEO4J_UNBOUND_RELATIONSHIP = 'r';
static const char NEO4J_PATH = 'I';

PackStream_Type packstream_next_type(const char *buffer);

bool packstream_read_null(char **buffer);

bool packstream_read_boolean(char **buffer, bool *value);

bool packstream_read_integer(char **buffer, int64_t *value);

bool packstream_read_float(char **buffer, double *value);

bool packstream_read_text(char **buffer, size_t *size, char **value);

bool packstream_read_list_header(char **buffer, int32_t *size);

bool packstream_read_structure_header(char **buffer, int32_t *size, char *signature);


size_t packstream_write_null(char *buffer);

size_t packstream_write_boolean(char *buffer, bool value);  // maybe write_true and write_false

void packstream_write_integer(char **buffer, int64_t value);

void packstream_write_float(char **buffer, double value);

void packstream_write_text(char **buffer, size_t size, const char *value);

void packstream_write_list_header(char **buffer, size_t size);

void packstream_write_map_header(char **buffer, size_t size);

void packstream_write_map(char **buffer, size_t size, const PackStream_Pair *entries);

void packstream_write_struct_header(char **buffer, size_t size, char signature);

void packstream_write_value(char **buffer, PackStream_Value *value);


#endif // NEO4J_C_DRIVER_PACKSTREAM_H
