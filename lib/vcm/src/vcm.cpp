#include "lib/vcm/vcm.h"
#include "lib/dls/dls.h"
#include "common/types.h"
#include <string>
#include <fstream>
#include <sstream>
#include <exception>
#include <unordered_map>
#include "endian.h"

using namespace dls;
using namespace vcm;

VCM::VCM() {
    MsgLogger logger("VCM", "Constructor");

    // default values
    addr = port = -1;
    protocol = PROTOCOL_NOT_SET;
    packet_size = 0;
    device = "";
    recv_endianness = GSW_LITTLE_ENDIAN; // default is little endian

    if(__BYTE_ORDER == __BIG_ENDIAN) {
        sys_endianness = GSW_BIG_ENDIAN;
    } else if(__BYTE_ORDER == __LITTLE_ENDIAN) {
        sys_endianness = GSW_LITTLE_ENDIAN;
    } else {
        logger.log_message("Could not determine endianness of system, assuming little endian");
        sys_endianness = GSW_LITTLE_ENDIAN;
    }

    // figure out default config file
    char* env = getenv("GSW_HOME");
    if(env == NULL) {
        logger.log_message("GSW_HOME environment variable not set!");
        throw new std::runtime_error("Environment error in VCM");
    }
    config_file = env;
    config_file += "/";
    config_file += DEFAULT_CONFIG_FILE;

    // init
    if(init() != SUCCESS) {
        throw new std::runtime_error("Error in VCM init");
    }
}

VCM::VCM(std::string config_file) {
    this->config_file = config_file;

    // default values
    addr = port = -1;
    protocol = PROTOCOL_NOT_SET;
    packet_size = 0;
    // compressed_size = 0;
    device = "";

    // init
    if(init() != SUCCESS) {
        throw new std::runtime_error("Error in VCM init");
    }
}

VCM::~VCM() {
    // free all pointers in addr_map
    for(auto i : addr_map) {
        delete i.second;
    }

    if(f) {
        if(f->is_open()) {
            f->close();
        }

        delete f;
    }
}

measurement_info_t* VCM::get_info(std::string measurement) {
    if(addr_map.count(measurement)) {
        return addr_map.at(measurement);
    } else {
        return NULL;
    }
}

RetType VCM::init() {
    MsgLogger logger("VCM", "init");

    f = new std::ifstream(config_file.c_str());
    if(!f) {
        logger.log_message("Failed to open config file: "+config_file);
        return FAILURE;
    }

    // read the config file
    for(std::string line; std::getline(*f,line); ) {
        if(line == "" || !line.rfind("#",0)) { // blank or comment '#'
            continue;
        }

        // get 1st + 2nd tokens
        // this is really ugly ¯\_(ツ)_/¯
        std::istringstream ss(line);
        std::string fst;
        ss >> fst;
        std::string snd;
        ss >> snd;
        std::string third;
        ss >> third;

        // port or addr or protocol line
        if(snd == "=") {
            if(fst == "addr") {
                try {
                    addr = std::stoi(third, NULL, 10);
                } catch(std::invalid_argument& ia) {
                    logger.log_message("Invalid addr in line: " + line);
                    return FAILURE;
                }
            } else if(fst == "port") {
                try {
                    port = std::stoi(third, NULL, 10);
                } catch(std::invalid_argument& ia) {
                    logger.log_message("Invalid port in line: " + line);
                    return FAILURE;
                }
            } else if(fst == "protocol") {
                if(third == "udp") {
                    protocol = UDP;
                } else {
                    logger.log_message("Unrecogonized protocol on line: " + line);
                    return FAILURE;
                }
            } else if(fst == "name") {
                device = third;
            } else if(fst == "endianness") {
                if(third == "little") {
                    recv_endianness = GSW_LITTLE_ENDIAN;
                } else if(third == "big") {
                    recv_endianness = GSW_BIG_ENDIAN;
                } else {
                    logger.log_message("Unrecogonized endianness on line: " + line);
                    return FAILURE;
                }
            }
        } else {
            std::string fourth;
            ss >> fourth;
            std::string fifth;
            ss >> fifth;
            std::string sixth;
            ss >> sixth;

            if(fst == "" || snd == "" || third == "" || fourth == "") { // these are required
                logger.log_message("Missing information: " + line);
                return FAILURE;
            }

            measurement_info_t* entry = new measurement_info_t;
            entry->addr = (void*)packet_size;
            try {
                entry->size = (size_t)(std::stoi(snd, NULL, 10));
                entry->l_padding = (size_t)(std::stoi(third, NULL, 10));
                entry->r_padding = (size_t)(std::stoi(fourth, NULL, 10));
            } catch(std::invalid_argument& ia) {
                logger.log_message("Invalid measurement size: " + line);
                return FAILURE;
            }
            packet_size += entry->size;
            // compressed_size += (entry->size*sizeof(unsigned char)) - (entry->l_padding + entry->r_padding);


            // check for type (optional, default is undefined)
            if(fifth == "int") {
                entry->type = INT_TYPE;
            } else if(fifth == "float") {
                entry->type = FLOAT_TYPE;
            } else if(fifth == "string") {
                entry->type = STRING_TYPE;
            } else if(fifth == "") {
                entry->type = UNDEFINED_TYPE;
            } else {
                logger.log_message("Invalid type specified: " + fifth);
                return FAILURE;
            }

            if(sixth == "unsigned") {
                entry->sign = UNSIGNED_TYPE;
            } else if(sixth == "signed") {
                entry->sign = SIGNED_TYPE;
            } else if(sixth == "") {
                entry->sign = SIGNED_TYPE; // default is signed
            } else {
                logger.log_message("Invalid sign specified: " + sixth);
                return FAILURE;
            }

            addr_map[fst] = entry;
            measurements.push_back(fst);
        }
    }

    f->close();

    // check for unset mandatory configuration items
    if(protocol == PROTOCOL_NOT_SET) {
        logger.log_message("Config file missing protocol: " + config_file);
        return FAILURE;
    } else if(protocol == UDP && (addr == -1 || port == -1)) {
        logger.log_message("Config file missing port or addr for UDP protocol: " + config_file);
        return FAILURE;
    }

    return SUCCESS;
}
