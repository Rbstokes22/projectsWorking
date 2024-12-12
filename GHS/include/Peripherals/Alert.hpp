#ifndef ALERT_HPP
#define ALERT_HPP

namespace Peripheral {

enum class ALTCOND : uint8_t {LESS_THAN, GTR_THAN, NONE}; // Alert condition.

class Alert {
    private:
    Alert();
    Alert(const Alert&) = delete; // prevent copying
    Alert &operator=(const Alert&) = delete; // prevent assignment

    public:
    static Alert* get();
    bool sendMessage(
        const char* APIkey,
        const char* phone,
        const char* msg
    );
};


}

#endif // ALERT_HPP