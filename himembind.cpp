//   Copyright 2019 <Huawei Technologies Co., Ltd>
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#ifdef _MSC_VER
#     include <process.h>
#     define execvp _execvp
#else

#     include <unistd.h>

#endif  // _MSC_VER

#include <hwloc.h>
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <string.h>
#include <ctype.h>

int is_valid_call_name(const char *call_name, const char *valid_name);

int is_valid_character(char character);

int check_single_argument(const char *arg);

int is_valid_arguments(char **args, int argc);

int check_call_name(const char *call_name);

int main(int argc, char **argv) {
    int ret = 0;

    const char *myname = argv[0];
    ++argv;
    --argc;

    const char *callname = argv[0];

    hwloc_topology_t topology;
    unsigned long flags = 0;

    hwloc_topology_init(&topology);
    hwloc_topology_set_flags(topology, flags);
    hwloc_topology_load(topology);

    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_PROCESS);

    char *ss = nullptr;
    hwloc_bitmap_asprintf(&ss, cpuset);
    fprintf(stderr, "cpuset: %s\n", ss);
    free(ss);
    ss = nullptr;

    char *ss1 = nullptr;
    hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
    hwloc_cpuset_to_nodeset(topology, cpuset, nodeset);
    hwloc_bitmap_asprintf(&ss1, nodeset);
    fprintf(stderr, "nodeset: %s\n", ss1);
    free(ss1);
    ss1 = nullptr;

    ret = hwloc_set_membind_nodeset(topology, nodeset, HWLOC_MEMBIND_BIND,
                                    HWLOC_MEMBIND_STRICT);
    if (ret) {
        fprintf(stderr, "%s: Failed to set membind policy\n", myname);
        goto exit_failure;
    }

    if (argc == 0) {
        fprintf(stderr, "Usage: %s <cmd>\n", myname);
        goto exit_failure;
    }

    // Check for call name
    if (!check_call_name(callname)) {
        fprintf(stderr, "Invalid call name\n");
        goto exit_failure;
    }

    // Check for arguments
    if (!is_valid_arguments(argv, argc)) {
        fprintf(stderr, "Invalid arguments\n");
        goto exit_failure;
    }


    ret = execvp(callname, argv);

    if (ret) {
        fprintf(stderr, "%s: Failed to launch executable \"%s\"\n", callname,
                argv[0]);
        perror("execvp");
        goto exit_failure;
    }

    exit_failure:
    hwloc_bitmap_free(cpuset);
    hwloc_bitmap_free(nodeset);
    hwloc_topology_destroy(topology);
    return EXIT_FAILURE;
}

/**
 * Check valid call name
 * @param call_name
 * @param valid_name
 * @return check result
 */
int is_valid_call_name(const char *call_name, const char *valid_name) {
    int call_name_len = strlen(call_name);
    int valid_name_len = strlen(valid_name);

    /**
     * The two stings should be equal length.
     */
    if (call_name_len != valid_name_len)
        return 0;

    /**
     * check string content
     */
    int i = 0;
    while (i < call_name_len) {
        if (call_name[i] != valid_name[i])
            return 0;
        i++;
    }

    return 1;
}

/**
 * check valid character
 * @param character
 * @return
 */
int is_valid_character(char character) {
    if (isalpha(character) || isnumber(character))
        return 1;
    /**
     * check other allowed character '-','.','_'
     */
    if (character == '-' || character == '.' || character == '_')
        return 1;

    return 0;
}

/**
 * check whether single argument is valid
 * @param arg
 * @return result
 */
int check_single_argument(const char *arg) {
    int len = strlen(arg);
    int i = 0;
    while (i < len) {
        if (!is_valid_character(arg[i]))
            return 0;
        i++;
    }
    return 1;
}

/**
 * Check arguments are valid
 * @param args arguments
 * @param argc arguments count
 * @return result
 */
int is_valid_arguments(char **args, int argc) {
    int i = 0;
    while (i < argc) {
        if (!check_single_argument(args[i]))
            return 0;
        i++;
    }
    return 1;
}

/**
 * Check call name is valid
 * @param call_name
 * @return result
 */
int check_call_name(const char *call_name) {
    if (is_valid_call_name(call_name, "mpirun"))
        return 1;
    return 0;
}
