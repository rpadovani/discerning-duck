#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

#include <QJsonDocument>

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:
    /**
     * Abstract
     */
    struct Abstract {
        std::string summary;
        std::string textSummary;    // No HTML here
        std::string source;
        std::string url;
        std::string imageUrl;
        std::string heading;
    };

    /**
     * Answer
     */
    struct Answer {
        std::string instantAnswer;
        std::string type;
    };

    /**
     * Definition
     */
    struct Definition {
        std::string definition;
        std::string source;
        std::string url;
    };

    /**
     * Content it's used by Infobox to load a single information
     */
    struct Content {
        std::string data_type; // Never found something different from string here, but anyway...
        std::string value;
        std::string label;
        unsigned int wiki_order;
    };

    /**
     * Icon associated with an url
     * Used by Result
     */
    struct Icon {
        std::string url;
        unsigned int width;
        unsigned int height;
    };

    /**
     * Single Result from RelatedTopics or Results
     */
    struct Result {
        std::string result;
        std::string url;
        Icon icon;
        std::string text;
    };

    /**
     * Infobox: array of informations about an abstract
     */
    typedef std::deque<Content> Infobox;

    /**
     * RelatedTopics: array of internal links to related topics associated with
     * Abstract
     */
    typedef std::deque<Result> RelatedTopics;

    /**
     * Results: array of external links associated with Abstract
     */
    typedef std::deque<Result> Results;

    /*
     * Query results
     */
    public: class QueryResults {
        public:
            Abstract abstract;
            Answer answer;
            Definition definition;
            Infobox infobox;
            RelatedTopics relatedTopics;
            Results results;
            std::string type;

            bool isEmpty() {
                return abstract.heading.empty() &&
                    answer.type.empty() &&
                    definition.definition.empty() &&
                    infobox.empty() &&
                    relatedTopics.empty() &&
                    results.empty();
            }
    };

    public: class HomePage {
        public:
            Answer fortune;
            Answer sunrise;

            bool isEmpty() {
                return fortune.type.empty() &&
                    sunrise.type.empty();
            }
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;
    /*
     * Get the result of a query or of the homepage
     */
    virtual QueryResults queryResults(const std::string &query);
    virtual HomePage homepageResults(const std::string &query);

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QJsonDocument &root);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

}

#endif // API_CLIENT_H_

