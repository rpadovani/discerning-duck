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
                "art" : {
                    "field": "art"
                },
                "summary": "summary"
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
                "summary": "summary"
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
                "summary": "description"
            }
        }
    )";

const static string CATEGORIES_TEMPLATE =
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
                "art": {
                    "field": "art"
                }
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
            // Build up the title
            stringstream ss(stringstream::in | stringstream::out);
            ss << queryResults.abstract.heading;

            // Register a category for the abstract
            auto abstract_cat = reply->register_category("abstract", ss.str(), "",
                    sc::CategoryRenderer(ABSTRACT_TEMPLATE));

            {
                // Create a single result for the current abstract
                sc::CategorisedResult res(abstract_cat);

                // Set results
                res.set_uri(queryResults.abstract.url);
                res.set_title(queryResults.abstract.heading);
                res.set_art(queryResults.abstract.imageUrl);
                res["summary"] = queryResults.abstract.textSummary;

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
                    _("Other informations"), "", sc::CategoryRenderer(INFOBOX_TEMPLATE));

            {
                // For each of the informations in the infobox, create a card
                for (const auto &content : queryResults.infobox) {
                    // Create a result
                    sc::CategorisedResult res(infobox_cat);

                    // Set informations
                    res.set_uri(to_string(content.wiki_order));
                    res.set_title(content.label);
                    res["summary"] = content.value;

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
            auto answer_cat = reply->register_category("answer", _("Answer"), "",
                    sc::CategoryRenderer(ANSWER_TEMPLATE));

            {
                // Create a single result for the answer
                sc::CategorisedResult res(answer_cat);

                // We must have a URI
                res.set_uri(queryResults.answer.type);
                res.set_title(queryResults.answer.type);
                res["description"] = queryResults.answer.instantAnswer;

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
         */
        //if (!queryResults.empty()) {
            // Register a category for the infobox
            auto category_cat = reply->register_category("category",
                    _("Lists"), "", sc::CategoryRenderer(CATEGORIES_TEMPLATE));

            {
                // For each of the informations in the infobox, create a card
                for (const auto &content : queryResults.relatedTopics) {
                    // Create a result
                    sc::CategorisedResult res(category_cat);

                    // Set informations
                    res.set_uri(content.url);
                    res.set_title("SETTITLE");
                    res["summary"] = content.text;
                    res.set_art(content.icon.url);

                    // Push the result
                    if (!reply->push(res)) {
                        // If we fail to push, it means the query has been cancelled.
                        // So don't continue;
                        return;
                    }
                }
            }
        //}

    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}
