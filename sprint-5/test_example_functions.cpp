#include "test_example_functions.h"
#include "search_server.h"
#include "remove_duplicates.h"

using namespace std;

#define ASSERT_EQUAL(a, b) AssertEqualImpl(a, b, #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl(a, b, #a, #b, __FILE__, __FUNCTION__, __LINE__, hint)

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)

#define RUN_TEST(func) RunTestImpl(func, #func)

void TestRemoveDuplicates() {
    SearchServer searchServer("a the on is"s);
    searchServer.AddDocument(0, "this is a test"s, DocumentStatus::ACTUAL, {1});
    searchServer.AddDocument(1, "this green crocodile"s, DocumentStatus::ACTUAL, {1});
    searchServer.AddDocument(2, "this test"s, DocumentStatus::ACTUAL, {1});
    {
        ASSERT_EQUAL(3u, searchServer.GetDocumentCount());
        auto [words, status] = searchServer.MatchDocument("test"s, 2);
        ASSERT(!words.empty());
    }

    RemoveDuplicates(searchServer);
    {
        ASSERT_EQUAL(2u, searchServer.GetDocumentCount());
        auto [words, status] = searchServer.MatchDocument("test"s, 2);
        ASSERT(words.empty());
    }

}
