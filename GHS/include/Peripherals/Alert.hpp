#ifndef ALERT_HPP
#define ALERT_HPP

namespace Peripheral {

class Alert {
    private:
    Alert();
    Alert(const Alert&) = delete; // prevent copying
    Alert &operator=(const Alert&) = delete; // prevent assignment

    public:
    Alert* get();
    bool sendMessage(
        const char* APIkey,
        const char* phone,
        const char* msg
    );
};


}

#endif // ALERT_HPP