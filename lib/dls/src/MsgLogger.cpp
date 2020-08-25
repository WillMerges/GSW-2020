#include "dls.h"
#include <mqueue.h>

using namespace dls;

MsgLogger::MsgLogger(): class_name(""), func_name(""){}

MsgLogger::MsgLogger(std::string class_name, std::string func_name) {
    this->class_name = class_name;
    this->func_name = func_name;
}

MsgLogger::MsgLogger(std::string func_name): class_name("") {
    this->func_name = func_name;
}

RetType MsgLogger::log_message(std::string msg) { //TODO maybe remove this from the class and make it a normal function so the telemtry data logger can use it
    // TODO add timestamp? (maybe let the writing process do this)
    std::string new_msg = class_name + ", " + func_name + ": " + msg;

    mqd_t mq;
    mq = mq_open(MQUEUE_NAME, O_WRONLY);
    if((mqd_t)-1 == mq) {
        // no sense in logging this because it is the logger!
        return FAILURE;
    }

    if(0 > mq_send(mq, msg.c_str(), msg.size(), 0)) {
        // no sense in logging this because it is the logger!
        return FAILURE;
    }

    if((mqd_t)-1 == mq_close(mq)) {
        std::string err = "Failed to close mqueue!";
        for(int i = 0; i<5; i++) { // try to send error msg 5 times
            if(0 <= mq_send(mq, err.c_str(), err.size(), 0)) {
                return FAILURE;
            }
        }
    }
    return SUCCESS;
}
