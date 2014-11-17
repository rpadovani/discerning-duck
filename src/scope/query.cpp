#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

#include <iomanip>
#include <sstream>
#include <QDebug>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the layout for the abstract responses
 *
 * The card is large, with a little photo, to have the maximum number of
 * information available
 */
const static string ABSTRACT_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "grid",
                "card-layout": "horizontal",
                "card-size": "large"
            },
            "components": {
                "title": "title",
                "subtitle": "subtitle",
                "art" : {
                    "field": "art"
                },
                "summary": "summary",
                "type": "type"
            }
        }
    )";

/**
 * Infobox template, for a lot of informations :-)
 *
 * Cards are little, with only essential informations: the name of the data,
 * and the data itself
 */
const static string INFOBOX_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "vertical-journal",
                "card-layout": "horizontal",
                "card-size": "small"
            },
            "components": {
                "title": "title",
                "summary": "summary",
                "type": "type"
            }
        }
    )";

/**
 * Answer template, for simple questions with a simple answer
 */
const static string ANSWER_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "vertical-journal",
                "card-layout": "horizontal",
                "card-size": "large"
            },
            "components": {
                "title": "title",
                "summary": "summary",
                "type": "type"
            }
        }
    )";

/**
 * This template is for list of things related to a topic
 * The response type has to be C
 */
const static string CATEGORIES_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "grid",
                "overlay": true,
                "card-size": "medium"
            },
            "components": {
                "title": "title",
                "summary": "summary",
                "art": {
                    "field": "art"
                },
                "type": "type"
            }
        }
    )";

/**
 * This template is for disambiguation, it's similar to type C but it's D
 */
const static string DISAMBIGUATION_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "carousel",
                "overlay": true
            },
            "components": {
                "title": "title",
                "summary": "summary",
                "art": {
                    "field": "art"
                },
                "type": "type"
            }
        }
    )";

/*
 * 404 page - Nothing return from the query
 */
const static string EMPTY_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "grid",
                "card-size": "large"
            },
            "components": {
                "title": "title",
                "summary": "summary",
                "type": "type"
            }
        }
    )";

/*
 * Footer
 */
const static string FOOTER_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "grid",
                "card-size": "large"
            },
            "components": {
                "title": "title",
                "summary": "summary",
                "type": "type"
            }
        }
    )";

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}

void Query::run(sc::SearchReplyProxy const& reply) {
    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        Client::QueryResults queryResults;
        if (query_string.empty()) {
            // If the string is empty, get informations about DuckDuckGo
            // TODO: create a better default page
            queryResults = client_.queryResults("DuckDuckGo");
        } else {
            // otherwise, process the query
            queryResults = client_.queryResults(query_string);
        }

        /**
         *  Abstract
         */
        if (!queryResults.abstract.textSummary.empty()) {
            // Register a category for the abstract
            auto abstract_cat = reply->register_category("abstract",
                    queryResults.abstract.heading, "", sc::CategoryRenderer(ABSTRACT_TEMPLATE));

            {
                // Create a single result for the current abstract
                sc::CategorisedResult res(abstract_cat);

                // Set results
                res.set_uri(queryResults.abstract.url);
                res.set_title(queryResults.abstract.heading);
                res.set_art(queryResults.abstract.imageUrl);
                res["summary"] = queryResults.abstract.textSummary;
                if (queryResults.abstract.source != "") {
                    res["subtitle"] = "Source: " + queryResults.abstract.source;
                }

                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
        }

        /**
         * Infobox
         */
        if (!queryResults.infobox.empty()) {
            // Register a category for the infobox
            auto infobox_cat = reply->register_category("infobox",
                    "", "", sc::CategoryRenderer(INFOBOX_TEMPLATE));

            {
                // For each of the informations in the infobox, create a card
                for (const auto &content : queryResults.infobox) {
                    // Create a result
                    sc::CategorisedResult res(infobox_cat);

                    // Set informations
                    res.set_uri(queryResults.abstract.url);
                    res.set_title(content.label);
                    res["summary"] = content.value;

                    // These are only for the preview
                    res.set_art(queryResults.abstract.imageUrl);
                    res["subtitle"] = "Source: " + queryResults.abstract.source;

                    // Push the result
                    if (!reply->push(res)) {
                        // If we fail to push, it means the query has been cancelled.
                        // So don't continue;
                        return;
                    }
                }
            }
        }

        /**
         * Answer
         */
        if (!queryResults.answer.type.empty()) {
            // Register a category for answer
            auto answer_cat = reply->register_category("answer", "", "",
                    sc::CategoryRenderer(ANSWER_TEMPLATE));

            {
                // Create a single result for the answer
                sc::CategorisedResult res(answer_cat);

                // We must have a URI, anyway we will not use it
                res.set_uri(queryResults.answer.type);
                res.set_title(queryResults.answer.type);
                res["summary"] = queryResults.answer.instantAnswer;
                if (queryResults.abstract.source != "") {
                    res["subtitle"] = "Source: " + queryResults.abstract.source;
                }

                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
        }

        /**
         * Category
         * We don't want this if we already have an infobox
         */
        if (queryResults.infobox.empty() && queryResults.type == "C") {
            // Register a category for the category
            auto category_cat = reply->register_category("category",
                    queryResults.abstract.heading, "",
                    sc::CategoryRenderer(CATEGORIES_TEMPLATE));

            {
                // For each element of the category
                for (const auto &content : queryResults.relatedTopics) {
                    // Create a result
                    sc::CategorisedResult res(category_cat);

                    // Take the title of the result
                    std::size_t startPos = content.result.find("\">") + 2;
                    std::size_t endPos = content.result.find("</a>");
                    res.set_title(content.result.substr(startPos, endPos - startPos));
                    // 7 chars: "</a> - "
                    res["summary"] = content.result.substr(endPos+7);

                    // Remove https://www.duckduckgo.com/
                    res.set_uri(content.url.substr(23));
                    res.set_art(content.icon.url);

                    // Only for the preview
                    res["subtitle"] = "Source: " + queryResults.abstract.source;
                    res["type"] = queryResults.type;

                    // Push the result
                    if (!reply->push(res)) {
                        // If we fail to push, it means the query has been cancelled.
                        // So don't continue;
                        return;
                    }
                }
            }
        }

        /**
         * Disambiguation
         * We don't want this if we already have an infobox
         */
        if (queryResults.infobox.empty() && queryResults.type == "D") {
            // Register a category for the category
            auto disambiguation_cat = reply->register_category("disambiguation",
                    "Meanings", "",
                    sc::CategoryRenderer(DISAMBIGUATION_TEMPLATE));

            {
                // For each element of the category
                for (const auto &content : queryResults.relatedTopics) {
                    // Create a result
                    sc::CategorisedResult res(disambiguation_cat);

                    // Take the title of the result
                    std::size_t startPos = content.result.find("\">") + 2;
                    std::size_t endPos = content.result.find("</a>");
                    res.set_title(content.result.substr(startPos, endPos - startPos));
                    // 4 chars: "</a>"
                    res["summary"] = content.result.substr(endPos+4);

                    // Remove https://www.duckduckgo.com/
                    res.set_uri(content.url.substr(23));
                    res.set_art(content.icon.url);

                    // Only for the preview
                    res["subtitle"] = "Source: " + queryResults.abstract.source;
                    res["type"] = queryResults.type;

                    // Push the result
                    if (!reply->push(res)) {
                        // If we fail to push, it means the query has been cancelled.
                        // So don't continue;
                        return;
                    }
                }
            }
        }

        /**
         * 404: nothing found!
         */
        if (queryResults.isEmpty()) {
            auto empty_cat = reply->register_category("empty",
                    _("Nothing found"), "", sc::CategoryRenderer(EMPTY_TEMPLATE));

            {
                // Create a result
                sc::CategorisedResult res(empty_cat);

                // Set informations
                res.set_uri("1");
                res.set_title("Nothing here");
                res["summary"] = "Unfortunately, I'm not a search engine, but only a Discerning Duck - I cannot provide you results, but only answers. Please try another search :-)";

                // Push the result
                if (!reply->push(res)) {
                    // If we fail to push, it means the query has been cancelled.
                    // So don't continue;
                    return;
                }
            }
        }

        /**
         * Footer: all credits to DuckDuckGo!
         */
        // Register a category for the footer
        auto footer_cat = reply->register_category("footer",
                _(""), "", sc::CategoryRenderer(EMPTY_TEMPLATE));

        {
            // Create a result
            sc::CategorisedResult res(footer_cat);

            // Set informations
            std::string uri = "https://www.duckduckgo.com/?q=" + query_string;
            res.set_uri(uri);
            res.set_title("ALPHA VERSION!");
            res["summary"] = "Results from DuckDuckGo";

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }

    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}
