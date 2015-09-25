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

enum PackStream_Type
{

    /* Basic types */
    PACKSTREAM_NULL = 0,
    PACKSTREAM_BOOLEAN = 1,
    PACKSTREAM_INTEGER = 2,
    PACKSTREAM_FLOAT = 3,
    PACKSTREAM_TEXT = 4,
    PACKSTREAM_LIST = 5,
    PACKSTREAM_MAP = 6,

    /* Structures */
    PACKSTREAM_STRUCTURE = 7,
    PACKSTREAM_IDENTITY = 8,
    PACKSTREAM_NODE = 9,
    PACKSTREAM_RELATIONSHIP = 10,
    PACKSTREAM_PATH = 11,

};

struct PackStream_Value
{
    PackStream_Type type;
    size_t size;
    void *value;
};

struct PackStream_Pair
{
    PackStream_Value name;
    PackStream_Value value;
};

void packstream_read_header(const char **buffer, PackStream_Value *header);

void packstream_read_integer(const char **buffer, int64_t *value);

void packstream_read_float(const char **buffer, double *value);

void packstream_read_text(const char **buffer, char *value, size_t size);

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
