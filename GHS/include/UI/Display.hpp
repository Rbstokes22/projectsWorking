#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <cstdint>
#include "UI/IDisplay.hpp"
#include "Drivers/SSD1306_Library.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"

// All user interface data and functions
namespace UI {

class Display : public IDisplay {
    private:
    UI_DRVR::OLEDbasic display;
	bool displayOverride; // will allow system errors to display

	// Starting addresses for each error.
	uint8_t msgAddresses[static_cast<int>(UIvals::msgIndicyTotal)];
	uint8_t lastIndex;
	char msgBuffer[static_cast<int>(UIvals::OLEDCapacity)];
	void removeMessage();
	void appendMessage(char* msg);

    public:
    bool displayStatus;
    Display(); // constructor
    void init(uint8_t address); // initialize the display display logo
	void printWAP(Comms::WAPdetails &details);
	void printSTA(Comms::STAdetails &details);
	void printUpdates(const char* update);
	void updateProgress(const char* progress);

	// this is meant for errors or runtime messaging
	void displayMsg(char* msg) override; // inherits from IDisplay
	bool getOverrideStat() override;
	
	// allows error display to take priority over net display.
	void setOverrideStat(bool setting) override; 
	size_t getOLEDCapacity() const;
};

}

#endif // DISPLAY_HPP