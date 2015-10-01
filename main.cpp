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

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string.h>
#include <arpa/inet.h>

#include "bolt.h"

using namespace std;
using namespace chrono;

typedef time_point<high_resolution_clock> Time;

struct TimeSet
{
    Time init;
    Time req_prepared;
    Time req_sent;
    Time run_complete;
    Time pull_complete;
};

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
        if (i > 0 and format != NONE) cout << separator;
        print_next_value(bolt, format);
    }
    if (format != NONE) cout << endl;
}

int print_help(int argc, char *argv[])
{
    puts("usage: ...");
    return 0;
}

int run(const char *statement, size_t parameter_count, PackStream_Pair *parameters, PrintFormat format)
{
    Bolt *bolt = bolt_connect("127.0.0.1", 7687);
    //printf("Using protocol version %d\n", bolt->version);

    bolt_init(bolt, "seabolt/1.0");
    bolt_send(bolt);
    bolt_recv(bolt);

    bolt_run(bolt, statement, parameter_count, parameters);
    bolt_pull_all(bolt);
    bolt_send(bolt);

    // Header
    bolt_recv(bolt);
    PackStream_Type type = packstream_next_type(bolt->reader);
    if (type == PACKSTREAM_MAP) {
        int32_t size;
        packstream_read_map_header(&bolt->reader, &size);
        for (long i = 0; i < size; i++) {
            int32_t key_size;
            char *key;
            packstream_read_text(&bolt->reader, &key_size, &key);
            if (strcmp(key, "fields") == 0) {
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
        bolt_recv(bolt);
        if (bolt->message_signature == RECORD_MESSAGE) {
            print_next_separated_list(bolt, '\t', format);
        } else {
            cout << endl;
        }
    } while (bolt->message_signature == RECORD_MESSAGE);

    bolt_disconnect(bolt);

    return 0;
}

TimeSet bench_one(Bolt * bolt, const char *statement, size_t parameter_count, PackStream_Pair *parameters)
{
    TimeSet times;
    times.init = high_resolution_clock::now();

    bolt_run(bolt, statement, parameter_count, parameters);
    bolt_pull_all(bolt);
    times.req_prepared = high_resolution_clock::now();

    bolt_send(bolt);
    times.req_sent = high_resolution_clock::now();

    // Header
    bolt_recv(bolt);
    times.run_complete = high_resolution_clock::now();

    PackStream_Type type = packstream_next_type(bolt->reader);
    if (type == PACKSTREAM_MAP) {
        int32_t size;
        packstream_read_map_header(&bolt->reader, &size);
        for (long i = 0; i < size; i++) {
            int32_t key_size;
            char *key;
            packstream_read_text(&bolt->reader, &key_size, &key);
            if (strcmp(key, "fields") == 0) {
                print_next_separated_list(bolt, '\t', NONE);
            }
            else {
                print_next_value(bolt, NONE);
            }
        }
    } else {
        cerr << "Map expected" << endl;
    }

    do {
        bolt_recv(bolt);
        if (bolt->message_signature == RECORD_MESSAGE) {
            print_next_separated_list(bolt, '\t', NONE);
        }
    } while (bolt->message_signature == RECORD_MESSAGE);
    times.pull_complete = high_resolution_clock::now();

    return times;
}

int bench(const char *statement, size_t parameter_count, PackStream_Pair *parameters, unsigned int times)
{

    system_clock clock = high_resolution_clock();
    TimeSet * checkpoints = new TimeSet[times];

    Bolt *bolt = bolt_connect("127.0.0.1", 7687);
    //printf("Using protocol version %d\n", bolt->version);

    bolt_init(bolt, "seabolt/1.0");
    bolt_send(bolt);
    bolt_recv(bolt);

    Time t0 = high_resolution_clock::now();
    for (unsigned int x = 0; x < times; x++) {
        checkpoints[x] = bench_one(bolt, statement, parameter_count, parameters);
    }
    Time t1 = high_resolution_clock::now();

    double tx_per_sec = times / duration_cast<duration<double>>(t1 - t0).count();
    cout << tx_per_sec << " tx/sec" << endl;

    duration<double> overall_durations[times];
    duration<double> network_durations[times];
    duration<double> wait_durations[times];
    for (unsigned int x = 0; x < times; x++)
    {
        overall_durations[x] = duration_cast<duration<double>>(checkpoints[x].pull_complete - checkpoints[x].init);
        network_durations[x] = duration_cast<duration<double>>(checkpoints[x].pull_complete - checkpoints[x].req_prepared);
        wait_durations[x] = duration_cast<duration<double>>(checkpoints[x].run_complete - checkpoints[x].req_sent);
    }
    sort(overall_durations, overall_durations + times);
    sort(network_durations, network_durations + times);
    sort(wait_durations, wait_durations + times);

    double percentiles[] = {0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0,
                            80.0, 90.0, 95.0, 98.0, 99.0, 99.5, 99.9, 100.0};
    for (int i = 0; i < 16; i++) {
        double percentile = percentiles[i];
        unsigned int index = (unsigned int) floor(percentile * (times - 1) / 100.0);
        double overall = overall_durations[index].count();
        double network = network_durations[index].count();
        double wait = wait_durations[index].count();
        printf(" %9.1f%% | %9.1fµs | %9.1fµs | %9.1fµs |\n",
               percentile, 1000000.0 * overall, 1000000.0 * network, 1000000.0 * wait);
    }

    bolt_disconnect(bolt);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        exit(print_help(argc, argv));
    }

    char * command = argv[1];
    if (strcmp(command, "run") == 0) {
        exit(run(argv[2], 0, NULL, JSON));
    }
    else if (strcmp(command, "bench") == 0) {
        exit(bench(argv[2], 0, NULL, 100000));
    }
    else {
        cout << "Unknown command '" << command << '\'' << endl;
        exit(1);
    }

}
