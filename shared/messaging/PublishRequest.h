//
// Created by DED on 12.09.2026.
//

#ifndef LIQUIDPETPROJECT_PUBLISHREQUEST_H
#define LIQUIDPETPROJECT_PUBLISHREQUEST_H

#include <string>

#include "MessageMetadata.h"

namespace shared::messaging {

struct PublishRequest
{
    std::string topic;
    std::string key;
    std::string payload;
    MessageMetadata metadata;

    bool operator==(const PublishRequest&) const = default;
};

}


#endif  // LIQUIDPETPROJECT_PUBLISHREQUEST_H
