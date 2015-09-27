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

void print_next_value(Bolt *pBolt);

using namespace std;

void print_next_value(Bolt *bolt)
{
    switch(packstream_next_type(bolt->reader))
    {
        case PACKSTREAM_NULL: {
            packstream_read_null(&bolt->reader);
            cout << "null";
            break;
        }
        case PACKSTREAM_BOOLEAN: {
            bool value;
            packstream_read_boolean(&bolt->reader, &value);
            cout << (value ? "true" : "false");
            break;
        }
        case PACKSTREAM_INTEGER: {
            int64_t value;
            packstream_read_integer(&bolt->reader, &value);
            cout << value;
            break;
        }
        case PACKSTREAM_FLOAT: {
            double value;
            packstream_read_float(&bolt->reader, &value);
            cout << value;
            break;
        }
        case PACKSTREAM_TEXT: {
            size_t size;
            char *value;
            packstream_read_text(&bolt->reader, &size, &value);
            // TODO: character escaping
            cout << '"' << string(value, size) << '"';
            break;
        }
        default: {
            cout << '?';
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        puts("usage: ...");
        exit(0);
    }

    char *statement = argv[1];
    unsigned int times = 5;

    Bolt *bolt = bolt_connect("127.0.0.1", 7687);
    //printf("Using protocol version %d\n", bolt->version);

    bolt_init(bolt, "c-driver/1.0");
    bolt_read_message(bolt);

    for (int x = 0; x < times; x++) {
        bolt_run(bolt, statement, 0, NULL);
        bolt_pull_all(bolt);

        bolt_read_message(bolt);
        //printf("(HEADER)\n");
        do {
            bolt_read_message(bolt);
            if (bolt->message_signature == RECORD_MESSAGE) {
                PackStream_Type type = packstream_next_type(bolt->reader);
                if (type == PACKSTREAM_LIST) {
                    int32_t size;
                    packstream_read_list_header(&bolt->reader, &size);
                    for (long i = 0; i < size; i++) {
                        if (i > 0) cout << '\t';
                        print_next_value(bolt);
                    }
                    cout << endl;
                } else {
                    cerr << "List expected" << endl;
                }
            } else {
                //printf("(FOOTER)\n");
            }
        } while (bolt->message_signature == RECORD_MESSAGE);

        cout << endl;
    }

    bolt_disconnect(bolt);

    return 0;
}
