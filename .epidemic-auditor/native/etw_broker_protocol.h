#pragma once
#include <string>
#include <cstdint>

namespace etw_broker {
inline constexpr wchar_t service_name[] = L"EpidemicAuditorEtwBroker";
inline constexpr wchar_t pipe_name[] = L"\\\\.\\pipe\\EpidemicAuditorEtwBroker.v1";
inline constexpr wchar_t instance_name[] = L"EpidemicAuditorEtwBroker";
inline constexpr wchar_t store_path[] = L"E:\\EpidemicAuditorEtwBroker";
enum class Operation {
    invalid,
    start,
    stop,
    status,
    cancel
};
struct Request {
    Operation operation = Operation::invalid;
    std::string capture_id;
};
struct Peer {
    std::uint32_t pid = 0;
    std::uint64_t creation = 0;
    bool owns(const Peer &other) const {
        return pid != 0 && creation != 0 && pid == other.pid && creation == other.creation;
    }
};
inline bool valid_id(const std::string &value) {
    if (value.size() != 36)
        return false;
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index == 8 || index == 13 || index == 18 || index == 23) {
            if (value[index] != '-')
                return false;
        } else if (!((value[index] >= '0' && value[index] <= '9') || (value[index] >= 'a' && value[index] <= 'f')))
            return false;
    }
    return true;
}
inline Request parse(const std::string &value) {
    if (value == "START GeneralProfile")
        return {Operation::start, {}};
    if (value == "STATUS")
        return {Operation::status, {}};
    if (value.size() == 41 && value.substr(0, 5) == "STOP " && valid_id(value.substr(5)))
        return {Operation::stop, value.substr(5)};
    if (value.size() == 43 && value.substr(0, 7) == "CANCEL " && valid_id(value.substr(7)))
        return {Operation::cancel, value.substr(7)};
    return {};
}
int install(const wchar_t *sid, const wchar_t *expected_sha, const wchar_t *previous_sha = nullptr);
}
