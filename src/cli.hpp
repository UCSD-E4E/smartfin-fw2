#ifndef __CLI_HPP__
#define __CLI_HPP__

#include "task.hpp"
#include "product.hpp"

#define CLI_RGB_LED_COLOR       SF_CLI_RGB_LED_COLOR
#define CLI_RGB_LED_PATTERN     SF_CLI_RGB_LED_PATTERN
#define CLI_RGB_LED_PERIOD      SF_CLI_RGB_LED_PERIOD
#define CLI_RGB_LED_PRIORITY    SF_CLI_RGB_LED_PRIORITY

class CLI : public Task {
    public:
    void init(void);
    STATES_e run(void);
    void exit(void);
};

#endif