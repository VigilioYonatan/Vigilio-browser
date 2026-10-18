#include "storage/LocalRepository.h"
#include "core/UrlInput.h"
namespace vigilio {
namespace {
bool exec(sqlite3* db, const char* sql) {
    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}
bool validWebUrl(const std::string& url) {
    auto r = resolveInput(url);
    return r.kind == NavigationTarget::Kind::Url && r.value == url;
}
} // namespace
LocalRepository::LocalRepository(const std::filesystem::path& path) {
    if (sqlite3_open_v2(path.string().c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK) {
        sqlite3_close(db_);
        db_ = nullptr;
        return;
    }
    sqlite3_busy_timeout(db_, 2000);
    ready_ =
        exec(db_, "PRAGMA journal_mode=WAL") &&
        exec(db_, "CREATE TABLE IF NOT EXISTS bookmarks (url TEXT PRIMARY KEY, title TEXT NOT NULL)") &&
        exec(db_, "CREATE TABLE IF NOT EXISTS visits (id INTEGER PRIMARY KEY, url TEXT NOT NULL, title "
                  "TEXT NOT NULL, visited_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)");
}
void LocalRepository::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        ready_ = false;
    }
}
LocalRepository::~LocalRepository() {
    close();
}
bool LocalRepository::addBookmark(const std::string& url, const std::string& title) {
    if (!valid() || !validWebUrl(url) || title.size() > 4096)
        return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_,
                           "INSERT INTO bookmarks (url,title) VALUES(?,?) ON CONFLICT(url) DO UPDATE "
                           "SET title=excluded.title",
                           -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
std::vector<Bookmark> LocalRepository::bookmarks() const {
    std::vector<Bookmark> output;
    if (!valid())
        return output;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_,
                           "SELECT url,title FROM bookmarks ORDER BY title COLLATE NOCASE LIMIT 2000",
                           -1, &stmt, nullptr) != SQLITE_OK)
        return output;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* url = sqlite3_column_text(stmt, 0);
        const auto* title = sqlite3_column_text(stmt, 1);
        if (url && title)
            output.push_back({reinterpret_cast<const char*>(url), reinterpret_cast<const char*>(title)});
    }
    sqlite3_finalize(stmt);
    return output;
}
bool LocalRepository::recordVisit(const std::string& url, const std::string& title) {
    if (!valid() || !validWebUrl(url) || title.size() > 4096)
        return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "INSERT INTO visits(url,title) VALUES(?,?)", -1, &stmt, nullptr) !=
        SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
int LocalRepository::historyCount() const {
    if (!valid())
        return -1;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM visits", -1, &stmt, nullptr) != SQLITE_OK)
        return -1;
    int n = sqlite3_step(stmt) == SQLITE_ROW ? sqlite3_column_int(stmt, 0) : -1;
    sqlite3_finalize(stmt);
    return n;
}
} // namespace vigilio
