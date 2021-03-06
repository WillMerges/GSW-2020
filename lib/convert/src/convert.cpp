#include "lib/convert/convert.h"
#include "lib/dls/dls.h"
#include <stdint.h>
#include <string.h>

using namespace vcm;
using namespace dls;

char result[MAX_CONVERSION_SIZE];

// TODO maybe have a fancy function with va_args, idk


RetType convert::convert_float(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, float* dst) {
    MsgLogger logger("CONVERT", "convert_float");

    if(measurement->type != FLOAT_TYPE) {
        logger.log_message("Measurement must be a float type!");
        return FAILURE;
    }

    if(measurement->size != sizeof(float)) {
        logger.log_message("measurement is not the size of a float!");
        return FAILURE;
    }

    uint8_t val[sizeof(float)];
    size_t addr = (size_t)measurement->addr;
    const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(uint32_t) - i - 1] = buff[addr + i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = buff[addr + i];
        }
    }

    *dst = *((float*)val);
    return SUCCESS;
}

RetType convert::convert_uint(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, uint32_t* dst) {
    MsgLogger logger("CONVERT", "convert_uint");

    if(measurement->type != INT_TYPE || measurement->sign != UNSIGNED_TYPE) {
        logger.log_message("Measurement must be an unsigned integer!");
        return FAILURE;
    }

    if(measurement->size > sizeof(int32_t)) {
        logger.log_message("measurement too large to fit into integer");
        return FAILURE;
    }

    uint8_t val[sizeof(uint32_t)];
    size_t addr = (size_t)measurement->addr;
    const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(uint32_t) - i - 1] = buff[addr + i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = buff[addr + i];
        }
    }

    *dst = *((uint32_t*)val);
    return SUCCESS;
}

RetType convert::convert_int(vcm::VCM* vcm, vcm::measurement_info_t* measurement, const void* data, int32_t* dst) {
    MsgLogger logger("CONVERT", "convert_int");

    if(measurement->type != INT_TYPE || measurement->sign != UNSIGNED_TYPE) {
        logger.log_message("Measurement must be an unsigned integer!");
        return FAILURE;
    }

    if(measurement->size > sizeof(int32_t)) {
        logger.log_message("measurement too large to fit into integer");
        return FAILURE;
    }

    uint8_t val[sizeof(int32_t)];
    size_t addr = (size_t)measurement->addr;
    const uint8_t* buff = (const uint8_t*)data;

    if(vcm->recv_endianness != vcm->sys_endianness) {
        for(size_t i = 0; i < measurement->size; i++) {
            val[sizeof(int32_t) - i - 1] = buff[addr + i];
        }
    } else {
        for(size_t i = 0; i < measurement->size; i++) {
            val[i] = buff[addr + i];
        }
    }

    *dst = *((int32_t*)val);
    return SUCCESS;
}


// NOTE: may be a problem if unsigned and signed ints aren't the same length
//       but that would be weird to have them not the same length
// TODO this function could use some cleaning, uses malloc when it doesnt need to, some unnecessary code from it being refactored so many times
RetType convert::convert_str(VCM* vcm, measurement_info_t* measurement, const void* data, std::string* dst) {
    MsgLogger logger("CONVERT", "convert_str");

    switch(measurement->type) {
        // all integer types, will try to put it a standard integer type of the same size or larger than the measurement size
        // distinguishes between signed and unsigned
        case INT_TYPE: {
            if(measurement->size > (sizeof(long int))) {
                logger.log_message("Measurement size too great to convert to integer");
                return FAILURE; // TODO add logging
            }

            // assume unsigned and signed are the same size (potential problem)
            unsigned char val[sizeof(long int)]; // always assume it's the biggest (don't care about a few bytes)
            memset(val, 0, sizeof(long int));

            size_t addr = (size_t)measurement->addr;
            const unsigned char* buff = (const unsigned char*)data;

            if(vcm->recv_endianness != vcm->sys_endianness) {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[sizeof(long int) - i - 1] = buff[addr + i];
                }
            } else {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[i] = buff[addr + i];
                }
            }

            if(measurement->sign == SIGNED_TYPE) { // signed
                snprintf(result, MAX_CONVERSION_SIZE, "%li", *((int64_t*)val));
            } else { // unsigned
                snprintf(result, MAX_CONVERSION_SIZE, "%lu", *((uint64_t*)val));
            }

            }
            break;

        // encompasses 4-byte floats and 8-byte doubles
        case FLOAT_TYPE: {
            size_t convert_size = 0;
            if(measurement->size == sizeof(float)) {
                convert_size = sizeof(float);
            } else if(measurement->size == sizeof(double)) {
                convert_size = sizeof(double);
            }

            if(convert_size == 0) {
                logger.log_message("Unable to convert floating type to float or double");
                return FAILURE;
            }

            unsigned char* val = (unsigned char*)malloc(convert_size);
            memset(val, 0, convert_size);

            size_t addr = (size_t)measurement->addr;
            const unsigned char* buff = (const unsigned char*)data;

            if(vcm->recv_endianness != vcm->sys_endianness) {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[convert_size - i - 1] = buff[addr + i];
                }
            } else {
                for(size_t i = 0; i < measurement->size; i++) {
                    val[i] = buff[addr + i];
                }
            }

            // sign doesn't exist for floating point
            snprintf(result, MAX_CONVERSION_SIZE, "%f", *((float*)val));

            free(val);
            }
            break;

        case STRING_TYPE: {
            // -1 is for room for a null terminator
            if(measurement->size > MAX_CONVERSION_SIZE - 1) {
                return FAILURE; // TODO add logging
            }
            size_t addr = (size_t)measurement->addr;
            const unsigned char* buff = (const unsigned char*)data;
            memcpy(result, buff + addr, measurement->size);
            result[measurement->size] = '\0'; // null terminate just in case

            }
            break;

        default:
            // TODO maybe display as hex?
            return FAILURE;
    }

    *dst = result;
    return SUCCESS;
}
