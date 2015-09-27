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

PackStream_Type packstream_next_type(const char *buffer)
{
    PackStream_Type type;
    unsigned char marker = (unsigned char) buffer[0];
    if (marker < 128 or marker >= 240 or marker == 0xC8 or marker == 0xC9 or marker == 0xCA or marker == 0xCB) {
        type = PACKSTREAM_INTEGER;
    }
    else if (marker == 0xC0) {
        type = PACKSTREAM_NULL;
    }
    else if (marker == 0xC1) {
        type = PACKSTREAM_FLOAT;
    }
    else if (marker == 0xC2 or marker == 0xC3) {
        type = PACKSTREAM_BOOLEAN;
    }
    else if (marker == 0xCC or marker == 0xCD or marker == 0xCE) {
        type = PACKSTREAM_BYTES;
    }
    else if (marker == 0xD0 or marker == 0xD1 or marker == 0xD2) {
        type = PACKSTREAM_TEXT;
    }
    else if (marker == 0xD4 or marker == 0xD5 or marker == 0xD6 or marker == 0xD7) {
        type = PACKSTREAM_LIST;
    }
    else if (marker == 0xD8 or marker == 0xD9 or marker == 0xDA or marker == 0xDB) {
        type = PACKSTREAM_MAP;
    }
    else if (marker == 0xDC or marker == 0xDD) {
        type = PACKSTREAM_STRUCTURE;
    }
    else if (marker == 0xDF) {
        type = PACKSTREAM_END_OF_STREAM;
    }
    else {
        unsigned char marker_high_nibble = (unsigned char) ((marker & 0xF0) >> 4);
        if (marker_high_nibble == 0x8) {
            type = PACKSTREAM_TEXT;
        }
        else if (marker_high_nibble == 0x9) {
            type = PACKSTREAM_LIST;
        }
        else if (marker_high_nibble == 0xA) {
            type = PACKSTREAM_MAP;
        }
        else if (marker_high_nibble == 0xB) {
            type = PACKSTREAM_STRUCTURE;
        }
        else {
            type = PACKSTREAM_RESERVED;
        }
    }
    return type;
}

bool packstream_read_null(char **buffer)
{
    unsigned char marker = (unsigned char) (*buffer)[0];
    if (marker == 0xC0) {
        *buffer += 1;
    }
    else {
        return false;
    }
    return true;
}

bool packstream_read_boolean(char **buffer, bool *value)
{
    unsigned char marker = (unsigned char) (*buffer)[0];
    if (marker == 0xC3) {
        *value = true;
        *buffer += 1;
    }
    else if (marker == 0xC2) {
        *value = false;
        *buffer += 1;
    }
    else {
        return false;
    }
    return true;
}

bool packstream_read_integer(char **buffer, int64_t *value)
{
    unsigned char marker = (unsigned char) (*buffer)[0];
    if (marker < 128 or marker >= 240) {
        *value = marker;
        *buffer += 1;
    }
    else if (marker == 0xC8 or marker == 0xC9 or marker == 0xCA or marker == 0xCB) {
        // TODO
        value = 0;
    }
    else {
        return false;
    }
    return true;
}

bool packstream_read_float(char **buffer, double *value)
{
    // TODO
    *value = 0.0;
    *buffer += 9;
    return true;
}

bool packstream_read_list_header(char **buffer, int32_t *size)
{
    unsigned char marker = (unsigned char) (*buffer)[0];
    if (marker == 0xD4) {
        *size = (uint8_t) (*buffer)[1];
        *buffer += 2;
    }
    else if (marker == 0xD5) {
        *size = ((uint8_t) (*buffer)[1] << 8) | ((uint8_t) (*buffer)[2]);
        *buffer += 3;
    }
    else if (marker == 0xD6) {
        *size = ((uint8_t) (*buffer)[1] << 24) | ((uint8_t) (*buffer)[2] << 16) |
                ((uint8_t) (*buffer)[3] << 8) | ((uint8_t) (*buffer)[4]);
        *buffer += 5;
    }
    else if (marker == 0xD7) {
        *size = -1;
        *buffer += 1;
    }
    else {
        unsigned char marker_high_nibble = (unsigned char) (marker & 0xF0);
        if (marker_high_nibble == 0x90) {
            *size = marker & 0x0F;
            *buffer += 1;
        }
        else {
            return false;
        }
    }
    return true;
}

bool packstream_read_structure_header(char **buffer, int32_t *size, char *signature)
{
    unsigned char marker = (unsigned char) (*buffer)[0];
    if (marker == 0xDC) {
        *size = (uint8_t) (*buffer)[1];
        *signature = (*buffer)[2];
        *buffer += 3;
    }
    else if (marker == 0xDD) {
        *size = ((uint8_t) (*buffer)[1] << 8) | ((uint8_t) (*buffer)[2]);
        *signature = (*buffer)[3];
        *buffer += 4;
    }
    else {
        unsigned char marker_high_nibble = (unsigned char) (marker & 0xF0);
        if (marker_high_nibble == 0xB0) {
            *size = marker & 0x0F;
            *signature = (*buffer)[1];
            *buffer += 2;
        }
        else {
            return false;
        }
    }
    return true;
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
