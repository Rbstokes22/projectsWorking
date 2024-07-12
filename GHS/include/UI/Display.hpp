#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "config.hpp"
#include "UI/IDisplay.hpp"
#include "Drivers/SSD1306_Library.hpp"

// TEMP DELETE
namespace Comms {
struct STAdetails {
char SSID[32] = "Bulbasaur";
char IPADDR[15] = "192.168.86.1";
char signalStrength[8] = "-32";
};

};


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
	void printWAP(
		const char SSID[20], // Hard coded index values, will not change.
		const char ipaddr[16], 
		const char status[4], 
		const char WAPtype[20], 
		const char heap[10],
		const uint8_t clientsConnected);
	void printSTA(
		Comms::STAdetails &details, 
		const char status[4], 
		const char heap[10]);
	void printUpdates(char* update);
	void updateProgress(char* progress);

	// this is meant for errors or runtime messaging
	void displayMsg(char* msg) override; // inherits from IDisplay
	bool getOverrideStat() override;
	
	// allows error display to take priority over net display.
	void setOverrideStat(bool setting) override; 
	size_t getOLEDCapacity() const;
};

}

#endif // DISPLAY_HPP