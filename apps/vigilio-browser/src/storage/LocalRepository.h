#pragma once
#include <filesystem>
#include <sqlite3.h>
#include <string>
#include <vector>
namespace vigilio {
struct Bookmark {
    std::string url;
    std::string title;
};
class LocalRepository {
  public:
    explicit LocalRepository(const std::filesystem::path& path);
    ~LocalRepository();
    LocalRepository(const LocalRepository&) = delete;
    LocalRepository& operator=(const LocalRepository&) = delete;
    bool valid() const {
        return db_ != nullptr && ready_;
    }
    bool addBookmark(const std::string& url, const std::string& title);
    std::vector<Bookmark> bookmarks() const;
    bool recordVisit(const std::string& url, const std::string& title);
    int historyCount() const;
    void close();

  private:
    sqlite3* db_{nullptr};
    bool ready_{false};
};
} // namespace vigilio
