#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <cstdint>
#include <cstddef>
#include "Drivers/SSD1306_Library.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"

// All user interface data and functions
namespace UI {

class Display : public IDisplay {
    private:
    UI_DRVR::OLEDbasic display;
	bool displayOverride; // will allow system errors to display

    public:
    bool displayStatus;
    Display(); // constructor
    void init(uint8_t address); // initialize the display display logo
	void printWAP(Comms::WAPdetails &details);
	void printSTA(Comms::STAdetails &details);
	void printUpdates(const char* update);
	void updateProgress(const char* progress);
	void invalidFirmware();
	void displayMsg(char* msg); 
	bool getOverrideStat();
	void setOverrideStat(bool setting); 
	size_t getOLEDCapacity() const;
};

}

#endif // DISPLAY_HPP