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
#include <arpa/inet.h>

#include "bolt.h"

using namespace std;

enum PrintFormat {
    NONE = 0,
    JSON = 1,
};

void print_json_string(char *buffer, int32_t size)
{
    cout << '"';
    for (int i = 0; i < size; i++) {
        unsigned char ch = (unsigned char) buffer[i];
        if (ch >= ' ' and ch <= '~') {
            cout << buffer[i];
        }
        else {
            switch (ch) {
                case '"':
                    cout << "\\\"";
                    break;
                case '\\':
                    cout << "\\\\";
                    break;
                case '\b':
                    cout << "\\b";
                    break;
                case '\f':
                    cout << "\\f";
                    break;
                case '\n':
                    cout << "\\n";
                    break;
                case '\r':
                    cout << "\\r";
                    break;
                case '\t':
                    cout << "\\t";
                    break;
                default:
                    // TODO: unicode character escaping as \uXXXX
                    cout << "\\x" << (ch < 0x10 ? "0" : "") << uppercase << hex << (int) ch;
            }
        }
    }
    cout << '"';
}

void print_next_value(Bolt *bolt, PrintFormat format)
{
    switch (packstream_next_type(bolt->reader))
    {
        case PACKSTREAM_NULL: {
            packstream_read_null(&bolt->reader);
            switch (format) {
                case JSON:
                    cout << "null";
                default:
                    ;
            }
            break;
        }
        case PACKSTREAM_BOOLEAN: {
            bool value;
            packstream_read_boolean(&bolt->reader, &value);
            switch (format) {
                case JSON:
                    cout << (value ? "true" : "false");
                default:
                    ;
            }
            break;
        }
        case PACKSTREAM_INTEGER: {
            int64_t value;
            packstream_read_integer(&bolt->reader, &value);
            switch (format) {
                case JSON:
                    cout << value;
                default:
                    ;
            }
            break;
        }
        case PACKSTREAM_FLOAT: {
            double value;
            packstream_read_float(&bolt->reader, &value);
            switch (format) {
                case JSON:
                    cout << value;
                default:
                    ;
            }
            break;
        }
        case PACKSTREAM_TEXT: {
            int32_t size;
            char *value;
            packstream_read_text(&bolt->reader, &size, &value);
            switch (format) {
                case JSON:
                    print_json_string(value, size);
                default:
                    ;
            }
            break;
        }
        case PACKSTREAM_LIST: {
            int32_t size;
            packstream_read_list_header(&bolt->reader, &size);
            switch (format) {
                case JSON:
                    cout << '[';
                    for (int i = 0; i < size; i++) {
                        if (i > 0) {
                            cout << ", ";
                        }
                        print_next_value(bolt, format);
                    }
                    cout << ']';
                default:
                    ;
            }
            break;
        }
        case PACKSTREAM_MAP: {
            int32_t size;
            packstream_read_map_header(&bolt->reader, &size);
            switch (format) {
                case JSON:
                    cout << '{';
                    for (int i = 0; i < size; i++) {
                        if (i > 0) {
                            cout << ", ";
                        }
                        print_next_value(bolt, format);
                        cout << ": ";
                        print_next_value(bolt, format);
                    }
                    cout << '}';
                default:
                    ;
            }
            break;
        }
        default: {
            cout << '?';
        }
    }
}

void print_next_separated_list(Bolt *bolt, char separator, PrintFormat format)
{
    int32_t size;
    packstream_read_list_header(&bolt->reader, &size);
    for (long i = 0; i < size; i++) {
        if (i > 0) cout << separator;
        print_next_value(bolt, format);
    }
    cout << endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        puts("usage: ...");
        exit(0);
    }

    unsigned int times = 5;
    PrintFormat format = JSON;

    Bolt *bolt = bolt_connect("127.0.0.1", 7687);
    //printf("Using protocol version %d\n", bolt->version);

    bolt_init(bolt, "seabolt/1.0");
    bolt_read_message(bolt);

    for (int x = 0; x < times; x++) {
        for(int arg = 1; arg < argc; arg++) {
            bolt_run(bolt, argv[arg], 0, NULL);
            bolt_pull_all(bolt);

            // Header
            bolt_read_message(bolt);
            PackStream_Type type = packstream_next_type(bolt->reader);
            if (type == PACKSTREAM_MAP) {
                int32_t size;
                packstream_read_map_header(&bolt->reader, &size);
                for (long i = 0; i < size; i++) {
                    int32_t key_size;
                    char *key;
                    packstream_read_text(&bolt->reader, &key_size, &key);
                    if (key_size == 6 and strcmp(key, "fields") == 0) {
                        print_next_separated_list(bolt, '\t', format);
                    }
                    else {
                        print_next_value(bolt, NONE);
                    }
                }
            } else {
                cerr << "Map expected" << endl;
            }

            do {
                bolt_read_message(bolt);
                if (bolt->message_signature == RECORD_MESSAGE) {
                    print_next_separated_list(bolt, '\t', format);
                } else {
                    cout << endl;
                }
            } while (bolt->message_signature == RECORD_MESSAGE);
        }
    }

    bolt_disconnect(bolt);

    return 0;
}
