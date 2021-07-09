/********************************************************************
*  Name: shm.c
*
*  Purpose: Utility functions for creating/modifying shared memory.
*
*  Author: Will Merges
*
*  RIT Launch Initiative
*********************************************************************/
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "lib/shm/shm.h"
#include "lib/dls/dls.h"
#include "common/types.h"

// NOTE: shared memory can be manually altered with 'ipcs' and 'ipcrm' programs

using namespace dls;
using namespace shm;

// constructor
Shm::Shm(const char* file, const int id, size_t size):size(size), key_file(file), key_id(id) {
    data = NULL;
    shmid = -1;
}

// creates shared memory but does not attach to it
RetType Shm::create() {
    MsgLogger logger("SHM", "create");

    // create key
    key_t key = ftok(key_file, key_id);
    if(key == (key_t) -1) {
        logger.log_message("ftok failure, no key generated");
        return FAILURE;
    }

    // get id
    shmid = shmget(key, size, 0666|IPC_CREAT|IPC_EXCL);
    if(shmid == -1) {
        logger.log_message("shmget failure");
        return FAILURE;
    }

    return SUCCESS;
}

RetType Shm::attach() {
    MsgLogger logger("SHM", "attach");

    // create key
    key_t key = ftok(key_file, key_id);
    if(key == (key_t) -1) {
        logger.log_message("ftok failure, no key generated");
        return FAILURE;
    }

    // get id
    shmid = shmget(key, size, 0666);
    if(shmid == -1) {
        logger.log_message("shmget failure");
        return FAILURE;
    }

    // attach to shared block
    data = (uint8_t*) shmat(shmid, (void*)0, 0);
    if(data == (void*) -1) {
        data = NULL;
        logger.log_message("shmat failure, cannot attach to shmem");
        return FAILURE;
    }

    return SUCCESS;
}

RetType Shm::detach() {
    MsgLogger logger("SHM", "detach_from_shm");

    if(data) {
        if(shmdt(data) == 0) {
            data = NULL;
            return SUCCESS;
        }
    }

    logger.log_message("shmdt failure");
    return FAILURE;
}

RetType Shm::destroy() {
    MsgLogger logger("SHM", "destroy");

    if(shmid == -1) {
        logger.log_message("No shmem to destroy");
        return FAILURE;
    }

    if(shmctl(shmid, IPC_RMID, NULL) == -1) {
        logger.log_message("shmctl failure, unable to destroy shared memory");
        return FAILURE;
    }

    shmid = -1;
    data = NULL;

    return SUCCESS;
}
