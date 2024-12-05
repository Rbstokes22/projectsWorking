#include "NVS2/NVS.hpp"
#include "nvs.h"
#include "assert.h"

namespace NVS {

// Requires pointer to nvs object. This will safely open the nvs
// and safely destruct it once it goes out of scope or it the
// opening encounters an error. You can check for success by 
// checking the nvs configuration bool "NVSopen"
NVS_SAFE_OPEN::NVS_SAFE_OPEN(NVSctrl *nvs) : nvs(nvs), TAG("NVS Open") {
 
    // Initializes if not already initialized. If the nvs is 
    // unable to initialize, does not attept to open.
    if (NVSctrl::init() != nvs_ret_t::NVS_INIT_OK) {
        printf("%s: Unable to initialize\n", this->TAG); 
        return;
    }

    NVS_Config* conf = this->nvs->getConfig(); // nvs configuration.
    

    if (conf->NVSopen) return; // If open return, if not, open

    esp_err_t open = nvs_open(conf->nameSpace, NVS_READWRITE, &conf->handle);

    switch (open) {
        case ESP_OK:
        printf("%s: Namespace %s is open\n", this->TAG, conf->nameSpace);
        conf->NVSopen = true;
        return;

        default: // Print statements that require no action.
        NVSctrl::defaultPrint(this->TAG, open);
    }

    this->~NVS_SAFE_OPEN(); // Will destruct upon unsuccessful open attempt.
}

// Destructor will close the NVS once called or it goes out of scope.
NVS_SAFE_OPEN::~NVS_SAFE_OPEN() {
    NVS_Config* conf = this->nvs->getConfig();

    if (!conf->NVSopen) return; // Will not close unless open.

    nvs_close(conf->handle); // closes the NVS connection
    conf->NVSopen = false; // flags NVSopen as false to show closed.
    printf("%s: Namespace %s closed\n", this->TAG, conf->nameSpace);
}

}