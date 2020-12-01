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
#include <memory>
#include <algorithm>
#include <cctype>
#include <string_view>
#include <iostream>

bool is_valid_call_name(const char *call_name);

bool is_valid_arguments(char **args, int argc);

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
    std::cerr << "cpuset: " << ss << std::endl;
    free(ss);
    ss = nullptr;

    char *ss1 = nullptr;
    hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
    hwloc_cpuset_to_nodeset(topology, cpuset, nodeset);
    hwloc_bitmap_asprintf(&ss1, nodeset);
    std::cerr << "nodeset: " << ss1 << std::endl;

    free(ss1);
    ss1 = nullptr;

    ret = hwloc_set_membind_nodeset(topology, nodeset, HWLOC_MEMBIND_BIND,
                                    HWLOC_MEMBIND_STRICT);
    if (ret) {
        std::cerr << myname << ": Failed to set membind policy" << std::endl;
        goto exit_failure;
    }

    if (argc == 0) {
        std::cerr << "Usage: " << myname << " <cmd>" << std::endl;
        goto exit_failure;
    }

    // Check for call name
    if (!is_valid_call_name(callname)) {
        std::cerr << "Invalid call name" << std::endl;
        goto exit_failure;
    }

    // Check for arguments
    if (!is_valid_arguments(argv, argc)) {
        std::cerr << "Invalid arguments" << std::endl;
        goto exit_failure;
    }


    ret = execvp(callname, argv);

    if (ret) {
        std::cerr << callname << ": Failed to launch executable \"" << argv[0] << "\"" << std::endl;
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
 * Check arguments in cmd
 * @param args arguments vector
 * @param argc arguments count
 * @return result
 */
bool is_valid_arguments(char **args, int argc) {
    return std::all_of(
            args, args + argc,
            [](std::string_view arg) {
                return std::all_of(std::begin(arg), std::end(arg),
                                   [](unsigned char c) {
                                       return std::isalnum(c)
                                              || c == '-'
                                              || c == '.'
                                              || c == '_'
                                              || c == '/'
                                              || c == '\\'
                                              || c == ' ';
                                   });
            });
}

/**
 * Check call name in cmd
 * @param call_name
 * @return result
 */
bool is_valid_call_name(const char *call_name) {
    if (strcmp(call_name, "mpirun") == 0)
        return true;
    if (strcmp(call_name, "python") == 0)
        return true;    
    if (strcmp(call_name, "python3") == 0)
        return true;
    return false;
}
