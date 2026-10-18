#include "storage/LocalRepository.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vigilio;

#define CHECK(condition)                                                                                \
    do {                                                                                                \
        if (!(condition))                                                                               \
            throw std::runtime_error("CHECK failed: " #condition);                                      \
    } while (false)

namespace {
struct TempDbGuard {
    std::filesystem::path path;
    explicit TempDbGuard(std::filesystem::path p) : path(std::move(p)) {
        cleanup();
    }
    ~TempDbGuard() {
        cleanup();
    }
    void cleanup() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        std::filesystem::remove(path.string() + "-wal", ec);
        std::filesystem::remove(path.string() + "-shm", ec);
    }
    TempDbGuard(const TempDbGuard&) = delete;
    TempDbGuard& operator=(const TempDbGuard&) = delete;
};
} // namespace

int main() {
    try {
        const auto file = std::filesystem::temp_directory_path() / "vigilio-test-storage.sqlite";
        TempDbGuard dbGuard(file);

        // --- 1. Fresh database creation and valid repository ---
        {
            LocalRepository db(file);
            CHECK(db.valid());

            // --- 2. Idempotent insertion and updates of bookmarks ---
            CHECK(db.addBookmark("https://example.com", "Example"));
            CHECK(db.addBookmark("https://example.com", "Updated Example"));
            CHECK(db.bookmarks().size() == 1);
            CHECK(db.bookmarks()[0].url == "https://example.com");
            CHECK(db.bookmarks()[0].title == "Updated Example");

            // Additional bookmarks and case-insensitive NOCASE sorting check
            CHECK(db.addBookmark("https://zebra.test", "zebra site"));
            CHECK(db.addBookmark("https://alpha.test", "Alpha Site"));
            auto bms = db.bookmarks();
            CHECK(bms.size() == 3);
            CHECK(bms[0].title == "Alpha Site");
            CHECK(bms[1].title == "Updated Example");
            CHECK(bms[2].title == "zebra site");

            // --- 3. SQL injection resistance (parameter binding validation) ---
            CHECK(db.addBookmark("https://test.com/?q=%27%3BDROP%20TABLE",
                                 "O'Reilly; DROP TABLE bookmarks; --"));
            CHECK(db.addBookmark("https://injection.test", "') UNION SELECT 1,2; --"));
            CHECK(db.recordVisit("https://safe.test", "Robert'); DROP TABLE visits; --"));

            // --- 4. Invalid URLs rejected ---
            CHECK(!db.addBookmark("javascript:alert(1)", "bad"));
            CHECK(!db.addBookmark("file:///etc/passwd", "bad"));
            CHECK(!db.addBookmark("", "empty"));
            CHECK(!db.addBookmark("https://user:pass@example.com", "credentials"));
            CHECK(!db.recordVisit("javascript:alert(1)", "bad"));
            CHECK(!db.recordVisit("", "bad"));

            // --- 5. Title length bounds (<= 4096 allowed, > 4096 rejected) ---
            const std::string validTitle(4096, 'a');
            CHECK(db.addBookmark("https://len-valid.test", validTitle));
            const std::string oversizedTitle(4097, 'b');
            CHECK(!db.addBookmark("https://len-invalid.test", oversizedTitle));
            CHECK(!db.recordVisit("https://len-invalid.test", oversizedTitle));

            // --- 6. History visit tracking ---
            CHECK(db.recordVisit("https://example.com", "Visited"));
            // Total visits recorded: 2 (Robert' DROP TABLE visits and Visited)
            CHECK(db.historyCount() == 2);
        }

        // --- 7. Reopening database: persistence and schema integrity ---
        {
            LocalRepository reopened(file);
            CHECK(reopened.valid());
            // Bookmarks: "https://example.com", "https://zebra.test", "https://alpha.test",
            //            "https://test.com/?q=%27%3BDROP%20TABLE", "https://injection.test",
            //            "https://len-valid.test" -> total 6
            CHECK(reopened.bookmarks().size() == 6);
            CHECK(reopened.historyCount() == 2);
        }

        // --- 8. Non-creatable / invalid database path recovery ---
        {
            const auto invalidPath = std::filesystem::path("/non_existent_dir_987654321/db.sqlite");
            LocalRepository badDb(invalidPath);
            CHECK(!badDb.valid());
            CHECK(!badDb.addBookmark("https://example.com", "Test"));
            CHECK(!badDb.recordVisit("https://example.com", "Test"));
            CHECK(badDb.historyCount() == -1);
            CHECK(badDb.bookmarks().empty());
        }

        std::cout << "SQLite integration tests: PASS\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
