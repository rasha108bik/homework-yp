#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
 
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus doc_status = DocumentStatus::ACTUAL) const {
        auto predicate = [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; };
        return FindTopDocuments(raw_query, predicate);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                auto document_data = documents_.at(document_id);
                if (predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }
};

template <typename Key, typename Value>
auto& Print(ostream& out, const map<Key, Value>& map) {
    bool first_elem = true;
    for (const auto& [k, v] : map) {
        if (first_elem) {
            out << k << ": " << v;
            first_elem = false;
            continue;
        }
        out << ", " << k << ": " << v;
    }

    return out;
}

template <typename Element>
auto& Print(ostream& out, const Element& elem) {
    bool first_elem = true;
    for (const auto& el : elem) {
        if (first_elem) {
            out << el;
            first_elem = false;
            continue;
        }
        out << ", " << el;
    }

    return out;
}

template <typename Vector>
ostream& operator<<(ostream& out, const vector<Vector>& vec) {
    out << "["s;
    return Print(out, vec) << "]"s;
}

template <typename Set>
ostream& operator<<(ostream& out, const set<Set>& set) {
    out << "{"s;
    return Print(out, set) << "}"s;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& map) {
    out << "{"s;
    return Print(out, map) << "}"s;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

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

template <typename Func, typename Str>
void RunTestImpl(const Func& func, const Str& func_str) {
    func();
    cerr << func_str << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет, что поисковая система исключает документы с минус словами
void TestExcludeMinusDocumentsFromAddedDocumentContent() {
    const int doc_id = 52;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server;
    server.AddDocument(doc_id, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("модный"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];

    ASSERT_HINT(server.FindTopDocuments("-модный"s).empty(), "Minus words must be excluded from result vector Document");
}

// Тест проверяет, что все слова из поискового запроса возвращаются, если нет минус слов
void TestMatchedDocumentContent() {
    {
        SearchServer server;
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});

        auto [words, _] = server.MatchDocument("пушистый ухоженный белый кот"s, 0);
        ASSERT(words.size() == 2u);

        vector<string> expected_response = {"белый", "кот"};
        ASSERT_EQUAL(words, expected_response);
    }

    {
        SearchServer server;
        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});

        auto [words, _] = server.MatchDocument("пушистый ухоженный -кот"s, 1);
        ASSERT(words.empty());
    }
}

// Тест проверяет, сортировку найденных документов по релевантности
void TestSortRelevance() {
    SearchServer server;

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

    double last_relevance = 0.0;
    for (const Document& document : server.FindTopDocuments("пушистый ухоженный кот"s)) {
        ASSERT(document.relevance > last_relevance);
    }
}

// Тест проверяет, что рейтинг добавленного документа равен среднему арифметическому оценок документа
void TestCalculateRating() {
    SearchServer server;
    server.AddDocument(0, "белая собака и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3, 4});

    auto document = server.FindTopDocuments("пушистый собака кот"s);
    ASSERT(document[0].rating == (8 + (-3) + 4) / 3);
}

// Тест проверяет, фильтрацию результатов поиска с использованием предиката, задаваемого пользователем
void TestFilterPredicate() {
    // Тест без предиката, задаваемого пользователем
    {
        SearchServer server;

        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

        ASSERT_EQUAL(server.GetDocumentCount(), 3);
    }

    // Тест c предикатом, задаваемый пользователем
    {
        SearchServer server;

        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

        for (const Document& document : server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) {
            return document_id % 2 == 0;
        })) {
            ASSERT(document.id % 2 == 0);
        }

        ASSERT_EQUAL(server.GetDocumentCount() - 1, 2);
    }
}

// тест проверяет, поиск документов, имеющих заданный статус
void TestFindDocumentsByStatus() {
    {
        SearchServer server;

        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

        auto document = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
        ASSERT(document.size() == 1u);
        const Document& doc0 = document[0];
        ASSERT(doc0.id == 0);
    }

    {
        SearchServer server;

        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

        auto document = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::REMOVED);
        ASSERT(document.size() == 1u);
        const Document& doc0 = document[0];
        ASSERT(doc0.id == 1);
    }

    {
        SearchServer server;

        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

        auto document = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
        ASSERT(document.size() == 1u);
        const Document& doc0 = document[0];
        ASSERT(doc0.id == 2);
    }

    {
        SearchServer server;

        server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

        auto document = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT(document.size() == 1u);
        const Document& doc0 = document[0];
        ASSERT(doc0.id == 3);
    }

    {
        SearchServer server;

        server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
        server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
        server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

        auto document = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
        ASSERT(document.empty());
    }
}

// тест проверяет, корректное вычисление релевантности найденных документов
void TestCorrectCalculateRelevance() {
    SearchServer server;

    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});

    const string& query = "пушистый ухоженный кот"s;
    auto document = server.FindTopDocuments(query);
    const Document& doc0 = document[0];

    const vector<string> words = SplitIntoWords(query);
    const double inv_word_count = 1.0 / words.size();
    const double inverse_document_freq = log(document.size() * 1.0 / 1.0);
    const double tf_df = inv_word_count * inverse_document_freq;
    ASSERT_HINT((doc0.relevance - tf_df) < 1e-6, "Check comparison error"s);
    ASSERT(doc0.relevance == tf_df);
}

// Тест проверяет, что при добавлении документов счетчик количества документов меняется
void TestCountDocuments() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    ASSERT_EQUAL(server.GetDocumentCount(), 1);

    server.AddDocument(2, "cat in the town"s, DocumentStatus::ACTUAL, {1, 9, 5});
    server.AddDocument(3, "cat in the country"s, DocumentStatus::ACTUAL, {1, 5, 3});
    ASSERT_EQUAL(server.GetDocumentCount(), 3);
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusDocumentsFromAddedDocumentContent);
    RUN_TEST(TestMatchedDocumentContent);
    RUN_TEST(TestSortRelevance);
    RUN_TEST(TestCalculateRating);
    RUN_TEST(TestFilterPredicate);
    RUN_TEST(TestFindDocumentsByStatus);
    RUN_TEST(TestCorrectCalculateRelevance);
    RUN_TEST(TestCountDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

    return 0;
}
