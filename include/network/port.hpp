#pragma once

#include <string>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>

namespace vswitch {

class Port {
public:
    virtual ~Port() = default;

    virtual const std::string& get_name() const = 0;
    virtual int get_fd() const = 0;
    virtual ssize_t read(uint8_t* buffer, size_t size) = 0;
    virtual ssize_t write(const uint8_t* buffer, size_t size) = 0;
};

} // namespace vswitch
