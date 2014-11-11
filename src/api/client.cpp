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
    QJsonDocument root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get( {"/"}, {{"q", query}, {"format", "json"}, {"t", "discerningduck"}},
                root);
    // e.g. http://api.duckduckgo.com/?q=QUERY&format=json&t=disceningduck

    QueryResults queryResults;

    // Read out the city we found
    QVariantMap variant = root.toVariant().toMap();
    if (variant["Abstract"].toString().toStdString() != "") {
        queryResults.abstract.summary = variant["Abstract"].toString().toStdString();
        queryResults.abstract.textSummary = variant["AbstractText"].toString().toStdString();
        queryResults.abstract.source = variant["AbstractSource"].toString().toStdString();
        queryResults.abstract.url = variant["AbstractUrl"].toString().toStdString();
        queryResults.abstract.imageUrl = variant["Image"].toString().toStdString();
        queryResults.abstract.heading = variant["Heading"].toString().toStdString();
    }

    if (variant["Answer"].toString().toStdString() != "") {
        queryResults.answer.istantAnswer = variant["Answer"].toString().toStdString();
        queryResults.answer.type = variant["AnswerType"].toString().toStdString();
    }

    if (variant["Definition"].toString().toStdString() != "") {
        queryResults.definition.definition = variant["Definition"].toString().toStdString();
        queryResults.definition.source = variant["DefinitionSource"].toString().toStdString();
        queryResults.definition.url = variant["DefinitionUrl"].toString().toStdString();
    }

        QVariantMap infobox = variant["Infobox"].toMap();
        QVariantList content = infobox["content"].toList();
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

