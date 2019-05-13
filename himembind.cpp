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
#  include <process.h>
#  define execvp _execvp
#else
#  include <unistd.h>
#endif // _MSC_VER
#include <stdlib.h>
#include <stdio.h>
#include <hwloc.h>
#include <memory>

int main(int argc, char** argv) {

    int ret = 0;

    const char* myname = argv[0];
    ++argv;
    --argc;


    const char* callname = argv[0];

    hwloc_topology_t topology;
    unsigned long flags = 0;

    hwloc_topology_init(&topology);
    hwloc_topology_set_flags(topology, flags);
    hwloc_topology_load(topology);

    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_PROCESS);

    char* ss = nullptr;
    hwloc_bitmap_asprintf(&ss, cpuset);
    fprintf(stderr, "cpuset: %s\n", ss);
    free(ss);
    ss = nullptr;

    char* ss1 = nullptr;
    hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
    hwloc_cpuset_to_nodeset(topology, cpuset, nodeset);
    hwloc_bitmap_asprintf(&ss1, nodeset);
    fprintf(stderr, "nodeset: %s\n", ss1);
    free(ss1);
    ss1 = nullptr;

    ret = hwloc_set_membind_nodeset(topology, nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_STRICT);
    if(ret) {
        fprintf(stderr, "%s: Failed to set membind policy\n", myname);
        goto exit_failure;
    }

    if(argc == 0) {
        fprintf(stderr, "Usage: %s <cmd>\n", myname);
        goto exit_failure;
    }

    ret = execvp(callname, argv);
    if (ret) {
        fprintf(stderr, "%s: Failed to launch executable \"%s\"\n",
                callname, argv[0]);
        perror("execvp");
        goto exit_failure;
    }

exit_failure:
    hwloc_bitmap_free(cpuset);
    hwloc_bitmap_free(nodeset);
    hwloc_topology_destroy(topology);
    return EXIT_FAILURE;
}

