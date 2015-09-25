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

#include "packstream.h"

using namespace std;

void packstream_read_header(const char **buffer, PackStream_Value *header)
{
    size_t byte_size = 1;
    char marker;
    memcpy(&marker, *buffer, 1);
    char marker_high_nibble = (char) ((marker & 0xF0) >> 4);
    char marker_low_nibble = (char) (marker & 0xF);
    if (marker_high_nibble == 0x8) {
        header->type = PACKSTREAM_TEXT;
        header->size = (size_t) marker_low_nibble;
    } else if (marker_high_nibble == 0x9) {
        header->type = PACKSTREAM_LIST;
        header->size = (size_t) marker_low_nibble;
    } else if (marker_high_nibble == 0xA) {
        header->type = PACKSTREAM_MAP;
        header->size = (size_t) marker_low_nibble;
    } else if (marker_high_nibble == 0xB) {
        header->size = (size_t) marker_low_nibble;
        char signature;
        memcpy(&signature, *buffer + 1, 1);
        if (signature == 'N') {
            header->type = PACKSTREAM_NODE;
        } else {
            // unknown structure type
            header->type = PACKSTREAM_STRUCTURE;
        }
    } else if (marker == (char) 0xD0) {
        header->type = PACKSTREAM_TEXT;
        memcpy(&(header->size), *buffer + 1, 1);
        byte_size += 1;
    } else if (marker == (char) 0xD4) {
        header->type = PACKSTREAM_LIST;
        memcpy(&(header->size), *buffer + 1, 1);
        byte_size += 1;
    } else {
        header->type = PACKSTREAM_INTEGER;
        header->value = (void *) &marker;
    }
    *buffer += byte_size;
}

void packstream_read_text(const char **buffer, char *value, size_t size)
{
    memcpy(value, buffer, size);
    *buffer += size;
}

size_t packstream_write_null(char *buffer)
{
    size_t byte_size;
    char data[] = {(char) 0xC0};
    byte_size = sizeof data;
    memcpy(buffer, data, byte_size);
    return byte_size;
}

size_t packstream_write_boolean(char *buffer, bool value)
{
    size_t byte_size;
    char data[] = {value ? (char) 0xC3 : (char) 0xC2};
    byte_size = sizeof data;
    memcpy(buffer, data, byte_size);
    return byte_size;
}

void packstream_write_integer(char **buffer, int64_t value)
{
    size_t byte_size;
    if (-16 <= value && value < 128) {
        char data[] = {(char) value};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else {
        byte_size = 0;
    }
    *buffer += byte_size;
}

void packstream_write_text(char **buffer, size_t size, const char *value)
{
    size_t byte_size;
    if (size < 0x10) {
        char data[] = {(char) (0x80 | size)};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
        memcpy(*buffer + byte_size, value, size);
        byte_size += size;
    } else if (size < 0x100) {
        char data[] = {(char) 0xD0, (char) size};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
        memcpy(*buffer + byte_size, value, size);
        byte_size += size;
    } else {
        byte_size = 0;
    }
    *buffer += byte_size;
}

void packstream_write_list_header(char **buffer, size_t size)
{
    size_t byte_size;
    if (size < 0x10) {
        char data[] = {(char) (0x90 | size)};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else if (size < 0x100) {
        char data[] = {(char) 0xD4, (char) size};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else {
        byte_size = 0;
    }
    *buffer += byte_size;
}

void packstream_write_map_header(char **buffer, size_t size)
{
    size_t byte_size;
    if (size < 0x10) {
        char data[] = {(char) (0xA0 | size)};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else if (size < 0x100) {
        char data[] = {(char) 0xD8, (char) size};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else {
        byte_size = 0;
    }
    *buffer += byte_size;
}

void packstream_write_map(char **buffer, size_t size, const PackStream_Pair *entries)
{
    packstream_write_map_header(buffer, size);
    for (int i = 0; i < size; i++) {
        PackStream_Pair entry = entries[i];
        packstream_write_value(buffer, &(entry.name));
        packstream_write_value(buffer, &(entry.value));
    }
}

void packstream_write_struct_header(char **buffer, size_t size, char signature)
{
    size_t byte_size;
    if (size < 0x10) {
        char data[] = {(char) (0xB0 | size), signature};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else if (size < 0x100) {
        char data[] = {(char) 0xDC, (char) size, signature};
        byte_size = sizeof data;
        memcpy(*buffer, data, byte_size);
    } else {
        byte_size = 0;
    }
    *buffer += byte_size;
}

void packstream_write_value(char **buffer, PackStream_Value *value)
{
    switch (value->type) {
        case PACKSTREAM_INTEGER:
            packstream_write_integer(buffer, (int64_t) (value->value));
            break;
        case PACKSTREAM_TEXT:
            packstream_write_text(buffer, value->size, (char *) (value->value));
            break;
        default:
            cerr << "This shouldn't happen: " << value->type << endl;
    }
}
