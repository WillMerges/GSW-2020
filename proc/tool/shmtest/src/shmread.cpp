#include "lib/shm/shm.h"
#include <stdint.h>
#include <iostream>
#include <stdio.h>

using namespace std;
using namespace shm;

int main() {
    if(FAILURE == set_shmem_size(4)) {
        printf("Failed to set shared memory size\n");
        return -1;
    }

    if(FAILURE == attach_to_shm()) {
        printf("Failed to attach to shared memory\n");
        return -1;
    }

    char b[4];

    if(FAILURE == read_from_shm(b, 4)) {
        printf("Failed to read from shared memory\n");
        return -1;
    }

    cout << b << '\n';

    if(FAILURE == detach_from_shm()) {
        printf("Failed to detach from shared memory\n");
        return -1;
    }
}
