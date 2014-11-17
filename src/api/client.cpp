#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <QVariantMap>
#include <QDebug>

namespace http = core::net::http;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}


void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, QJsonDocument &root) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }
        // Parse the JSON from the response
        root = QJsonDocument::fromJson(response.body.c_str());
    } catch (net::Error &) {
    }
}

Client::QueryResults Client::queryResults(const string& query) {
    QJsonDocument queryResultsWithQ, queryResultsWithoutQ;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    //
    // Until Scopes doesn't support html, ask to DuckDuckGo only plain text
    // responses
    get( {}, {{"q", query}, {"format", "json"}, {"no_html", "1"},
            {"t", "discerningduck"}}, queryResultsWithQ);
    // e.g. http://api.duckduckgo.com/?q=QUERY&format=json&no_html=1&t=discerningduck
    //
    // The answer of these two queries sometimes are different, we need to
    // take best of both
    get( {query}, {{"format", "json"}, {"no_html", "1"},
            {"t", "discerningduck"}}, queryResultsWithoutQ);
    // e.g. http://api.duckduckgo.com/QUERY&format=json&no_html=1&t=discerningduck
    //
    // See https://api.duckduckgo.com/?q=ferrara&format=json&pretty=1 (no
    // infobox) and https://api.duckduckgo.com/ferrara&format=json&pretty=1
    //
    // On the other hand, see
    // https://api.duckduckgo.com/3*2&format=json&pretty=1 (no answer) and
    // https://api.duckduckgo.com/?q=3*2&format=json&pretty=1

    QueryResults queryResults;

    // Read out the abstract we found and take best results
    QVariantMap variantWithQ = queryResultsWithQ.toVariant().toMap();
    QVariantMap variantWithoutQ = queryResultsWithoutQ.toVariant().toMap();

    queryResults.abstract.summary = variantWithQ["Abstract"].toString().toStdString() != "" ?
        variantWithQ["Abstract"].toString().toStdString() :
        variantWithoutQ["Abstract"].toString().toStdString();
    queryResults.abstract.textSummary = variantWithQ["AbstractText"].toString().toStdString() != "" ?
        variantWithQ["AbstractText"].toString().toStdString() :
        variantWithoutQ["AbstractText"].toString().toStdString();
    queryResults.abstract.source = variantWithQ["AbstractSource"].toString().toStdString() != "" ?
        variantWithQ["AbstractSource"].toString().toStdString() :
        variantWithoutQ["AbstractSource"].toString().toStdString();
    queryResults.abstract.url = variantWithQ["AbstractUrl"].toString().toStdString() != "" ?
        variantWithQ["AbstractURL"].toString().toStdString() :
        variantWithoutQ["AbstractURL"].toString().toStdString();
    queryResults.abstract.imageUrl = variantWithQ["Image"].toString().toStdString() != "" ?
        variantWithQ["Image"].toString().toStdString() :
        variantWithoutQ["Image"].toString().toStdString();
    queryResults.abstract.heading = variantWithQ["Heading"].toString().toStdString() != "" ?
        variantWithQ["Heading"].toString().toStdString() :
        variantWithoutQ["Heading"].toString().toStdString();

    queryResults.answer.instantAnswer = variantWithQ["Answer"].toString().toStdString() != "" ?
        variantWithQ["Answer"].toString().toStdString() :
        variantWithoutQ["Answer"].toString().toStdString();
    queryResults.answer.type = variantWithQ["AnswerType"].toString().toStdString() != "" ?
        variantWithQ["AnswerType"].toString().toStdString() :
        variantWithoutQ["AnswerType"].toString().toStdString();

    queryResults.definition.definition = variantWithQ["Definition"].toString().toStdString() != "" ?
        variantWithQ["Definition"].toString().toStdString() :
        variantWithoutQ["Definition"].toString().toStdString();
    queryResults.definition.source = variantWithQ["DefinitionSource"].toString().toStdString() != "" ?
        variantWithQ["DefinitionSource"].toString().toStdString() :
        variantWithoutQ["DefinitionSouce"].toString().toStdString();
    queryResults.definition.url = variantWithQ["DefinitionURL"].toString().toStdString() != "" ?
        variantWithQ["DefinitionURL"].toString().toStdString() :
        variantWithoutQ["DefinitionURL"].toString().toStdString();

    queryResults.type = variantWithQ["Type"].toString().toStdString() != "" ?
        variantWithQ["Type"].toString().toStdString() :
        variantWithoutQ["Type"].toString().toStdString();

    QVariantMap infobox = variantWithQ["Infobox"].toMap();
    QVariantList content = infobox["content"].toList();

    int hasInfoboxData = 0;
    for (const QVariant &c : content) {
        hasInfoboxData = 1;
        QVariantMap item = c.toMap();
        queryResults.infobox.emplace_back(
            Content {
                item["data_type"].toString().toStdString(),
                item["value"].toString().toStdString(),
                item["label"].toString().toStdString(),
                item["wiki_order"].toUInt()
            }
        );
    }

    if (hasInfoboxData == 0) {
        infobox = variantWithoutQ["Infobox"].toMap();
        content = infobox["content"].toList();

        for (const QVariant &c : content) {
            QVariantMap item = c.toMap();
            queryResults.infobox.emplace_back(
                Content {
                    item["data_type"].toString().toStdString(),
                    item["value"].toString().toStdString(),
                    item["label"].toString().toStdString(),
                    item["wiki_order"].toUInt()
                }
            );
        }
    }

    int hasRelatedData = 0;
    QVariantList related = variantWithQ["RelatedTopics"].toList();
    for (const QVariant &r : related) {
        hasRelatedData = 1;
        QVariantMap result = r.toMap();
        QVariantMap icon = result["Icon"].toMap();
        queryResults.relatedTopics.emplace_back(
            Result {
                result["Result"].toString().toStdString(),
                result["FirstURL"].toString().toStdString(),
                Icon {
                    icon["URL"].toString().toStdString(),
                    icon["Height"].toUInt(),
                    icon["Width"].toUInt()
                },
                result["Text"].toString().toStdString()
            }
        );
    }

    if (hasRelatedData == 0) {
        related = variantWithoutQ["RelatedTopics"].toList();
        for (const QVariant &r : related) {
            QVariantMap result = r.toMap();
            QVariantMap icon = result["Icon"].toMap();
            queryResults.relatedTopics.emplace_back(
                    Result {
                        result["Result"].toString().toStdString(),
                        result["FirstURL"].toString().toStdString(),
                    Icon {
                        icon["URL"].toString().toStdString(),
                        icon["Height"].toUInt(),
                        icon["Width"].toUInt()
                    },
                    result["Text"].toString().toStdString()
                }
            );
        }
    }

    return queryResults;
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

