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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        puts("usage: ...");
        exit(0);
    }

    char *statement = argv[1];
    unsigned int times = 1;

    Bolt *bolt = bolt_connect("127.0.0.1", 7687);
    //printf("Using protocol version %d\n", bolt->version);

    bolt_init(bolt, "c-driver/1.0");
    bolt_read_message(bolt);

    for (int i = 0; i < times; i++) {
        bolt_run(bolt, statement, 0, NULL);
        bolt_pull_all(bolt);

        bolt_read_message(bolt);
        printf("(HEADER)\n");
        do {
            bolt_read_message(bolt);
            if (bolt->message_signature == RECORD_MESSAGE) {
                printf("RECORD\n");
            } else {
                printf("(FOOTER)\n");
            }
        } while (bolt->message_signature == RECORD_MESSAGE);

    }

    bolt_disconnect(bolt);

    return 0;
}
