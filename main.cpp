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

int main() {
    Bolt *bolt = bolt_connect("127.0.0.1", 7687);
    printf("Using protocol version %d\n", bolt->version);

    bolt_init(bolt, "c-driver/1.0");

    bolt_run(bolt, "RETURN 1", 0, NULL);
    bolt_pull_all(bolt);

    bolt_disconnect(bolt);

    return 0;
}
