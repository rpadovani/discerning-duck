#ifndef API_CONFIG_H_
#define API_CONFIG_H_

#include <memory>
#include <string>

namespace api {

struct Config {
    typedef std::shared_ptr<Config> Ptr;

    /*
     * The root of all API request URLs
     */
    std::string apiroot { "https://api.duckduckgo.com" };

    /*
     * The custom HTTP user agent string for this library
     */
    std::string user_agent { "discerning-duck 0.1; (foo)" };
};

}

#endif /* API_CONFIG_H_ */

