#include <scope/preview.h>

#include <unity/scopes/ColumnLayout.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/VariantBuilder.h>

#include <iostream>

namespace sc = unity::scopes;

using namespace std;
using namespace scope;

Preview::Preview(const sc::Result &result, const sc::ActionMetadata &metadata) :
    sc::PreviewQueryBase(result, metadata) {
}

void Preview::cancelled() {
}

void Preview::run(sc::PreviewReplyProxy const& reply) {
    sc::Result result = PreviewQueryBase::result();

    // Support three different column layouts
    sc::ColumnLayout layout1col(1), layout2col(2), layout3col(3);

    // Single column layout
    layout1col.add_column( { "image", "header", "summary", "actionsId" });

    // Two column layout
    layout2col.add_column( { "image" });
    layout2col.add_column( { "header", "summary", "actionsId" });

    // Three cokumn layout
    layout3col.add_column( { "image" });
    layout3col.add_column( { "header", "summary" });
    layout3col.add_column( { "actionsId" });

    // Register the layouts we just created
    reply->register_layout( { layout1col, layout2col, layout3col });

    // Define the header section
    sc::PreviewWidget header("header", "header");
    // It has title and a subtitle properties
    header.add_attribute_mapping("title", "title");
    header.add_attribute_mapping("subtitle", "subtitle");

    // Define the image section
    sc::PreviewWidget image("image", "image");
    // It has a single source property, mapped to the result's art property
    image.add_attribute_mapping("source", "art");

    // Define the summary section
    sc::PreviewWidget description("summary", "text");
    // It has a text property, mapped to the result's description property
    description.add_attribute_mapping("text", "summary");

    // Define the action section
    sc::PreviewWidget actions("actionsId", "actions");
    sc::VariantBuilder builder;

    std::string scope_uri = "scope://com.ubuntu.developer.rpadovani.discerningduck_discerningduck?q=";
    std::string uri = result["uri"].get_string();
    std::string label = "See more";

    // Focus on a character in a list
    // Disabled due bug
    // https://bugs.launchpad.net/ubuntu/+source/unity-scopes-shell/+bug/1335761
//    if (result["type"] == sc::Variant("C") || result["type"] == sc::Variant("D")) {
//        uri = scope_uri + uri;
//        label = "Tell me more";
//    }

    builder.add_tuple({
        {"id", sc::Variant("open")},
        {"label", sc::Variant(label)},
        {"uri", sc::Variant(uri)}
    });

    actions.add_attribute_value("actions", builder.end());

    // Push each of the sections, if are Answer there is nothing more
    // The fortune in the homepage doesn't have click
    if (result["type"] == sc::Variant("E") || uri == "fortune.ddg.home") {
        reply->push({ header, description });
    } else {
        reply->push ( { image, header, description, actions });
    }
    return;
}

