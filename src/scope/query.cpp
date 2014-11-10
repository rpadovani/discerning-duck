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

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the layout for the forecast results
 *
 * The icon size is small, and ask for the card layout
 * itself to be horizontal. I.e. the text will be placed
 * next to the image.
 */
const static string ABSTRACT_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
            "category-layout": "vertical-journal",
            "card-layout": "vertical",
            "card-size": "38"
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
 * Define the larger "current weather" layout.
 *
 * The icons are larger.
 */
const static string CITY_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-size": "medium"
        },
        "components": {
        "title": "title",
        "art" : {
        "field": "art"
        },
        "subtitle": "subtitle"
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
            // If the string is empty, get the current weather for London
            queryResults = client_.queryResults("DuckDuckGo");
        } else {
            // otherwise, get the current weather for the search string
            queryResults = client_.queryResults(query_string);
        }

        // Build up the title
        stringstream ss(stringstream::in | stringstream::out);
        ss << queryResults.abstract.heading;

        // Register a category for the current weather, with the title we just built
        auto abstract_cat = reply->register_category("queryResults", ss.str(), "",
                                                     sc::CategoryRenderer(ABSTRACT_TEMPLATE));

        {
            // Create a single result for the current weather category
            sc::CategorisedResult res(abstract_cat);

            // We must have a URI
            res.set_uri(queryResults.abstract.heading);

            // Build up the description for the current weather
            stringstream ss(stringstream::in | stringstream::out);
            ss << queryResults.abstract.heading;
            res.set_title(ss.str());

            // Set the rest of the attributes, art, description, etc
            res.set_art(queryResults.abstract.imageUrl);
            res["summary"] = queryResults.abstract.textSummary;

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }

        /*Client::Forecast forecast;
        if (query_string.empty()) {
            // If there is no search string, get the forecast for London
            forecast = client_.forecast_daily("London,uk");
        } else {
            // otherwise, get the forecast for the search string
            forecast = client_.forecast_daily(query_string);
        }

        // Register a category for the forecast
        auto forecast_cat = reply->register_category("forecast",
                                                     _("7 day forecast"), "", sc::CategoryRenderer(WEATHER_TEMPLATE));

        // For each of the forecast days
        for (const auto &weather : forecast.weather) {
            // Create a result
            sc::CategorisedResult res(forecast_cat);

            // We must have a URI
            res.set_uri(to_string(weather.id));

            // Build the description for the result
            stringstream ss(stringstream::in | stringstream::out);
            ss << setprecision(3) << weather.temp.max;
            ss << "°C to ";
            ss << setprecision(3) << weather.temp.min;
            ss << "°C";
            res.set_title(ss.str());

            // Set the rest of the attributes
            res.set_art(weather.icon);
            res["subtitle"] = weather.description;
            res["description"] = "A description of the result";

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }
*/
    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

