#include "core/pinned_versions.h"

#include <QFile>
#include <QString>
#include <sstream>
#include <string>

#define TOML_EXCEPTIONS 0
#include <tomlplusplus/toml.hpp>

namespace rm::installer::core {

namespace {

QString tomlString(const toml::table& tbl, std::string_view key) {
    auto node = tbl[key];
    if (auto s = node.as_string()) {
        return QString::fromStdString(s->get());
    }
    return {};
}

bool requireString(const toml::table& tbl, std::string_view key, QString& out,
                   QString* err) {
    auto node = tbl[key];
    auto s = node.as_string();
    if (!s) {
        if (err) {
            *err = QStringLiteral("missing or non-string key: %1")
                       .arg(QString::fromUtf8(key.data(),
                                              static_cast<int>(key.size())));
        }
        return false;
    }
    out = QString::fromStdString(s->get());
    return true;
}

const toml::table* requireSection(const toml::table& root, std::string_view key,
                                  QString* err) {
    auto node = root[key];
    auto* tbl = node.as_table();
    if (!tbl) {
        if (err) {
            *err = QStringLiteral("missing section: [%1]")
                       .arg(QString::fromUtf8(key.data(),
                                              static_cast<int>(key.size())));
        }
        return nullptr;
    }
    return tbl;
}

}

std::optional<PinnedConfig> PinnedConfig::loadFromString(const QByteArray& toml_bytes,
                                                         QString* err) {
    toml::parse_result result = toml::parse(std::string_view(
        toml_bytes.constData(), static_cast<size_t>(toml_bytes.size())));
    if (!result) {
        if (err) {
            std::ostringstream oss;
            oss << result.error();
            *err = QStringLiteral("TOML parse error: %1")
                       .arg(QString::fromStdString(oss.str()));
        }
        return std::nullopt;
    }
    const toml::table& root = result.table();

    PinnedConfig cfg;

    const toml::table* xovi = requireSection(root, "xovi", err);
    if (!xovi) return std::nullopt;
    if (!requireString(*xovi, "tag", cfg.xovi.tag, err)) return std::nullopt;
    if (!requireString(*xovi, "url", cfg.xovi.url, err)) return std::nullopt;
    if (!requireString(*xovi, "sha256", cfg.xovi.sha256, err)) return std::nullopt;
    if (!requireString(*xovi, "minimum_compatible_tag", cfg.xovi.minimumCompatibleTag,
                       err)) {
        return std::nullopt;
    }

    const toml::table* appload = requireSection(root, "appload", err);
    if (!appload) return std::nullopt;
    if (!requireString(*appload, "tag", cfg.appload.tag, err)) return std::nullopt;
    if (!requireString(*appload, "url", cfg.appload.url, err)) return std::nullopt;
    if (!requireString(*appload, "sha256", cfg.appload.sha256, err)) return std::nullopt;

    const toml::table* rm = requireSection(root, "readmarkable", err);
    if (!rm) return std::nullopt;
    if (!requireString(*rm, "url_template", cfg.readmarkable.urlTemplate, err)) {
        return std::nullopt;
    }

    cfg.readmarkable.sha256UrlTemplate = tomlString(*rm, "sha256_url_template");
    if (cfg.readmarkable.sha256UrlTemplate.isEmpty()) {
        cfg.readmarkable.sha256UrlTemplate = QStringLiteral("{url}.sha256");
    }
    if (!requireString(*rm, "min_version", cfg.readmarkable.minVersion, err)) {
        return std::nullopt;
    }

    return cfg;
}

std::optional<PinnedConfig> PinnedConfig::loadFromResource(const QString& resourcePath,
                                                           QString* err) {
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) {
            *err = QStringLiteral("cannot open resource: %1").arg(resourcePath);
        }
        return std::nullopt;
    }
    return loadFromString(f.readAll(), err);
}

std::optional<PinnedConfig> PinnedConfig::loadFromFile(const QString& path,
                                                       QString* err) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) {
            *err = QStringLiteral("cannot open file: %1").arg(path);
        }
        return std::nullopt;
    }
    return loadFromString(f.readAll(), err);
}

int PinnedConfig::compareXoviTag(const QString& a, const QString& b) {

    return QString::compare(a, b);
}

QString PinnedConfig::resolveReadMarkableUrl(const QString& tag) const {
    QString out = readmarkable.urlTemplate;
    out.replace(QStringLiteral("{tag}"), tag);
    return out;
}

QString PinnedConfig::resolveReadMarkableSha256Url(const QString& tag) const {
    QString resolved = resolveReadMarkableUrl(tag);
    QString out = readmarkable.sha256UrlTemplate;
    out.replace(QStringLiteral("{url}"), resolved);
    out.replace(QStringLiteral("{tag}"), tag);
    return out;
}

}
