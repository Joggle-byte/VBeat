#pragma once

#include "song.hpp"
#include "playlist.hpp"


class ResourceValidator {
public:

    ResourceValidator() {}
    ~ResourceValidator() {}

    void validate(Song* res);

private:

    bool is_device_available(int deviceIndex);
};