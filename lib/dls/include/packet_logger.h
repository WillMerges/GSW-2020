/**
*   Queues messages to the telemetry packet queue. Queues are POSIX mqueues.
*   The message writer process will read the queue and write the packets to
*   the correcty log files atomically.
**/

#ifndef PACKET_LOGGER_H
#define PACKET_LOGGER_H

#include "types.h"
#include "logger.h"
#include <string>

namespace dls {

    static const char* TELEMETRY_MQUEUE_NAME = "/telemetry_queue"; // TODO maybe move this to the message writer process header

    class PacketLogger : public Logger {
    public:
        PacketLogger(std::string device_name);
        RetType log_packet(unsigned char* buffer, size_t size);
    private:
        std::string device_name;
    };

}

#endif
