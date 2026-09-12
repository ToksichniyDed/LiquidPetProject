//
// Created by DED on 12.09.2026.
//

#ifndef LIQUIDPETPROJECT_KAFKACONSUMERCONFIGURATION_H
#define LIQUIDPETPROJECT_KAFKACONSUMERCONFIGURATION_H

#include <string>

namespace shared::messaging {

struct KafkaConsumerConfiguration
{
    std::string brokers;
    std::string groupId;
    std::string topic;
};

}

#endif  // LIQUIDPETPROJECT_KAFKACONSUMERCONFIGURATION_H
