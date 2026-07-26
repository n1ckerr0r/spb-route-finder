#include "route_report_formatter.hpp"

#include <iomanip>
#include <sstream>

namespace {
std::string valueOrUnknown(const nlohmann::json &object, const char *field) {
    if (object.contains(field) && object[field].is_string()) {
        return object[field].get<std::string>();
    }
    return "unknown";
}
}  // namespace

std::string RouteReportFormatter::toHtml(
    const nlohmann::json &routes, const std::string &travel_date,
    const std::string &from_city_name, const std::string &to_city_name,
    std::uint16_t route_count) {
    std::ostringstream html;
    html << "<html><body style='font-family: Segoe UI, Arial, sans-serif; "
            "background:#f4f7fb; color:#183153;'>";
    html << "<div style='background:linear-gradient(135deg,#0b63f6,#19a974);"
            "border-radius:18px;padding:18px 22px;color:white;margin-bottom:16px;'>";
    html << "<div style='font-size:13px;opacity:0.9;'>Round-trip route search</div>";
    html << "<div style='font-size:28px;font-weight:700;margin-top:4px;'>"
         << escapeHtml(from_city_name) << " to " << escapeHtml(to_city_name) << "</div>";
    html << "<div style='margin-top:8px;font-size:14px;'>Date: "
         << escapeHtml(travel_date) << " | Routes found: " << route_count << "</div>";
    html << "</div>";

    if (route_count == 0) {
        html << "<div style='background:white;border:1px solid #d8e2f0;border-radius:16px;"
                "padding:18px; font-size:15px;'>"
             << "No round-trip routes with at most one transfer were found for this date."
             << "</div></body></html>";
        return html.str();
    }

    for (std::uint16_t index = 0; index < route_count; ++index) {
        const auto route_key = "route " + std::to_string(index);
        if (!routes.contains(route_key)) {
            continue;
        }

        const auto &route = routes.at(route_key);
        html << "<div style='background:white;border:1px solid #d8e2f0;border-radius:16px;"
                "padding:18px;margin-bottom:14px;box-shadow:0 8px 24px rgba(11,99,246,0.08);'>";
        html << "<div style='font-size:20px;font-weight:700;color:#0b63f6;margin-bottom:12px;'>"
             << "Route " << (index + 1) << "</div>";

        if (route.contains("there-route")) {
            html << renderSegment(route.at("there-route"), "Outbound");
        }
        if (route.contains("back-route")) {
            html << renderSegment(route.at("back-route"), "Return");
        }

        html << "</div>";
    }

    html << "</body></html>";
    return html.str();
}

std::string RouteReportFormatter::escapeHtml(const std::string &value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '&':
            escaped += "&amp;";
            break;
        case '<':
            escaped += "&lt;";
            break;
        case '>':
            escaped += "&gt;";
            break;
        case '"':
            escaped += "&quot;";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::string RouteReportFormatter::formatIsoTime(const std::string &value) {
    std::tm tm = {};
    std::istringstream input(value.substr(0, 19));
    input >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (input.fail()) {
        return value;
    }

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M", &tm);
    return buffer;
}

std::string RouteReportFormatter::renderSegment(
    const nlohmann::json &segment, const std::string &title) {
    std::ostringstream html;
    html << "<div style='border-top:1px solid #edf2f9;padding-top:12px;margin-top:12px;'>";
    html << "<div style='font-size:17px;font-weight:700;color:#183153;'>" << title << "</div>";
    html << "<div style='margin-top:8px;font-size:14px;line-height:1.6;'>";
    html << "<b>Departure:</b> "
         << escapeHtml(formatIsoTime(valueOrUnknown(segment, "departure"))) << "<br/>";
    html << "<b>Arrival:</b> "
         << escapeHtml(formatIsoTime(valueOrUnknown(segment, "arrival"))) << "<br/>";

    if (segment.contains("from") && segment["from"].contains("title")) {
        html << "<b>From:</b> "
             << escapeHtml(segment["from"]["title"].get<std::string>()) << "<br/>";
    } else if (segment.contains("departure_from") &&
               segment["departure_from"].contains("title")) {
        html << "<b>From:</b> "
             << escapeHtml(segment["departure_from"]["title"].get<std::string>()) << "<br/>";
    }

    if (segment.contains("to") && segment["to"].contains("title")) {
        html << "<b>To:</b> " << escapeHtml(segment["to"]["title"].get<std::string>())
             << "<br/>";
    } else if (segment.contains("arrival_to") && segment["arrival_to"].contains("title")) {
        html << "<b>To:</b> "
             << escapeHtml(segment["arrival_to"]["title"].get<std::string>()) << "<br/>";
    }

    if (segment.contains("duration")) {
        html << "<b>Duration:</b> " << std::fixed << std::setprecision(1)
             << (segment["duration"].get<double>() / 3600.0) << " h<br/>";
    }

    if (segment.contains("thread") && segment["thread"].contains("transport_type")) {
        html << "<b>Transport:</b> "
             << escapeHtml(segment["thread"]["transport_type"].get<std::string>()) << "<br/>";
    } else if (segment.contains("transport_types") &&
               segment["transport_types"].is_array()) {
        html << "<b>Transport:</b> ";
        bool first = true;
        for (const auto &transport_type : segment["transport_types"]) {
            if (!first) {
                html << ", ";
            }
            first = false;
            html << escapeHtml(transport_type.get<std::string>());
        }
        html << "<br/>";
    }

    if (segment.contains("details") && segment["details"].is_array()) {
        html << renderDetails(segment["details"]);
    }

    html << "</div></div>";
    return html.str();
}

std::string RouteReportFormatter::renderDetails(const nlohmann::json &details) {
    std::ostringstream html;
    html << "<div style='margin-top:10px;padding:10px 12px;background:#f8fbff;"
            "border-radius:12px;border:1px solid #e5edf8;'>";
    html << "<div style='font-weight:700;margin-bottom:8px;'>Trip details</div>";

    for (const auto &detail : details) {
        html << "<div style='margin-bottom:8px;'>";
        if (detail.contains("is_transfer") && detail["is_transfer"].get<bool>()) {
            html << "<span style='display:inline-block;background:#ffe8a3;color:#7a5200;"
                    "border-radius:999px;padding:3px 8px;font-size:12px;font-weight:700;'>"
                 << "Transfer</span> ";
            if (detail.contains("transfer_point") && detail["transfer_point"].contains("title")) {
                html << escapeHtml(detail["transfer_point"]["title"].get<std::string>());
            }
            if (detail.contains("duration")) {
                html << " (" << std::fixed << std::setprecision(1)
                     << detail["duration"].get<double>() / 3600.0 << " h)";
            }
        } else {
            if (detail.contains("thread") && detail["thread"].contains("title")) {
                html << "<b>"
                     << escapeHtml(detail["thread"]["title"].get<std::string>())
                     << "</b><br/>";
            }
            html << escapeHtml(formatIsoTime(valueOrUnknown(detail, "departure")))
                 << " -> "
                 << escapeHtml(formatIsoTime(valueOrUnknown(detail, "arrival")));
        }
        html << "</div>";
    }

    html << "</div>";
    return html.str();
}
