#ifndef IDISPLAY_HPP
#define IDISPLAY_HPP

#include <cstddef>

// ATTENTION. This was already built and one of the first headers/src files.
// This must exist to decouple the established program.

// IDisplay is an abstract class, also called interface. This does not require 
// a .cpp file, and serves to prevent circular dependencies between the Display.h
// and Network.h. Display.h depends on Network.h, but Network.h needs to be passed 
// the OLED reference from Display.h in order to print errors.

namespace UI {

#define OLED_UPDATE_DELAY_ms 1000 // delay to allow OLED updates to be viewed
#define OLED_CHAR_SIZE UI_DRVR::DIM::D5x7
#define OLED_CAPACITY 200 // Qty of 5x7 chars in the OLED display.

class IDisplay {
    public:
    // Makes this an abstract class.
    virtual void displayMsg(char* error) = 0;
    virtual bool getOverrideStat() = 0;
	virtual void setOverrideStat(bool setting) = 0; // allows error display
    virtual size_t getOLEDCapacity() const = 0;
    virtual ~IDisplay() {}
};

}

#endif // IDISPLAY_HPP

// Explanation for future. In c++ polymorphism allows any superclass pointer or 
// reference to refer to objects of any class that derives from it. Since the 
// Display class implements the IDisplay interface, this means that Display provides
// concrete implemntations for all virtual functions declared in IDisplay. This will 
// only work for the virtual functions of course. This allows the superclass, IDisplay,
// to reach down to display and use displayMsg, but it prevents us from including 
// display on the network.h file, instead we can include IDisplay.h, but can only use 
// that printError method.