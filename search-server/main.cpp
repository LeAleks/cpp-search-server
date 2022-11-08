#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>

//#include "search_server.h"

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

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
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

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
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }


    template <typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter_by) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter_by);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double comparator_accuracy = 1e-6;
                if (abs(lhs.relevance - rhs.relevance) < comparator_accuracy) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });


        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    //Функция для фильтрации по статусу
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_request = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status_request](int document_id, DocumentStatus status, int rating) { return status == status_request; });
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
        return { matched_words, documents_.at(document_id).status };
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
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

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
        return { text, is_minus, IsStopWord(text) };
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
                }
                else {
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

    template <typename Filter>
    vector<Document> FindAllDocuments(const Query& query, Filter filter_by) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& found_document = documents_.at(document_id);
                if (filter_by(document_id, found_document.status, found_document.rating)) {
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
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};




//реализация макросов ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
//Перегруз << для вывода DocumentStatus
ostream& operator<<(ostream& out, const DocumentStatus& status) {
    out << "Status: "s;
    switch (status) {
    case DocumentStatus::ACTUAL:
        out << "ACTUAL"s;
        break;
    case DocumentStatus::IRRELEVANT:
        out << "IRRELEVANT"s;
        break;
    case DocumentStatus::BANNED:
        out << "BANNED";
        break;
    case DocumentStatus::REMOVED:
        out << "REMOVED";
        break;
    default:
        out << "STATUS UNKONOWN"s;
        break;
    }
    return out;
}

//Перегруз << для вывода vector
template<typename t>
ostream& operator<<(ostream& out, const vector<t>& container) {
    out << "["s;
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            cout << element;
            is_first = false;
            continue;
        }
        out << ", " << element;
    }
    cout << "]"s;
    return out;
}

//Перегруз << для вывода set
template<typename t>
ostream& operator<<(ostream& out, const set<t>& container) {
    out << "{"s;
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            cout << element;
            is_first = false;
            continue;
        }
        out << ", " << element;
    }
    cout << "}"s;
    return out;
}

//Перегруз << для вывода map
ostream& operator<<(ostream& out, const map<string, int>& container) {
    out << "{"s;
    bool is_first = true;
    for (const auto& [name, age] : container) {
        if (is_first) {
            cout << name << ": " << age;
            is_first = false;
            continue;
        }
        out << ", " << name << ": " << age;
    }
    cout << "}"s;
    return out;
}

//Фнукция проверки двух значений на равенство, и вывод диагностической информации
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

//Макрос проверки двух занчений без вывода подсказки
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

//Макрос проверки двух занчений с выводом подсказки
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//Функция проверки что значение верно, и вывод диагностической информации
void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

//Макрос проверки значения без вывода подсказки
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

//Макрос проверки значения с выводом подсказки
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T& t, const string& func_name) {
    cerr << func_name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)


// -------- Начало модульных тестов поисковой системы ----------

//Тест проверяет, что новый документ добавлен и находится по ключевым словам из него (кроме стоп и минус слов)
void TestAddingDocument() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    //Проверяем, что документ добавляется
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //        assert(server.GetDocumentCount() == 1);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "Should be added one document"s);
    }

    //Проверяем, что все характеристи документа перенесены верно
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        //        assert(doc0.id == doc_id);
        ASSERT_EQUAL(doc0.id, doc_id);
        //        assert(doc0.rating == 2);
        ASSERT_EQUAL(doc0.rating, 2);
        ASSERT_EQUAL(doc0.relevance, 0);
        //        assert(doc0.relevance == 0);
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

//Тест проверяет, что поисковая система корректно удаляет докуместы, сожержащие минус-слова из поиска
void TestExludeDocumentsWithMinusWords() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список минус-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //        assert(server.FindTopDocuments("-in"s).empty());
        ASSERT_HINT(server.FindTopDocuments("-in"s).empty(),
            "Document with minus word should be removed from search responce"s);
    }

    //Задаем второй документ, чтобы проверить, что удаляется только он, если только
    //в нем есть минус слово
    const int doc_id2 = 24;
    const string content2 = "cat in the night city"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //Проверяем, что находятся оба документа
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat night"s);
        //        assert(found_docs.size() == 2);
        ASSERT_EQUAL(found_docs.size(), 2u);
    }


    //Проверяем, что один документ удаляется
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat -night"s);
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Only one document should be found"s);
        const Document& doc0 = found_docs[0];
        //        assert(doc0.id == doc_id);
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect documend was removed"s);
    }

}

//Тест проверяет, что поиск подходящих документов работает корректно
void TestMatchingDocuments() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов или минус слов,
    // находит нужный документ и выводит все совпадающие слова из запроса и верный статус
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark city", doc_id);
        vector<string> right_order = { "cat"s, "city"s, "in"s };
        //        assert(matched_words[0] == "cat"s);
        //        ASSERT_EQUAL(matched_words[0], "cat");
        //        assert(matched_words[1] == "city"s);
        //        ASSERT_EQUAL(matched_words[1], "city");
        //        assert(matched_words[2] == "in"s);
        //        ASSERT_EQUAL(matched_words[2], "in");
        ASSERT_EQUAL_HINT(matched_words, right_order, "Incorrect order of found words"s);
        //        assert(status == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }

    //Проверяем, что наличие минус слова возвращает пустой список слов
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark -city", doc_id);
        //        assert(matched_words.size() == 0);
        ASSERT_EQUAL_HINT(matched_words.size(), 0u, "Document with minus words should be excluded from the search responce"s);
        //        assert(status == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
}

//Тест проверяет, что relevance рассчитывется корректно, и документы сортируются в порядке неубывания relevance
void TestRelevanceSorting() {
    //Задаем тела документов
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 10;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 100;
    const string content2 = "dogs afraid of the black man"s;
    const vector<int> ratings2 = { 1, 2, 3 };


    //Проверяем расчет relevance
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black"s);
        double relevance_accuracy = 0.001;
        double real_accuracy = abs((found_docs[0].relevance - 0.183102) / 0.183102);
        //        assert(real_accuracy <= relevance_accuracy);
        ASSERT_HINT((real_accuracy <= relevance_accuracy), "Incorrect calculated relevance");
    }

    // Заносим все три документа в базу. Проверяем, что документы сортируются не по возрастанию relevance
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat black"s);
        //        assert((found_docs[0].relevance >= found_docs[1].relevance) && (found_docs[1].relevance >= found_docs[2].relevance));
        ASSERT_HINT(((found_docs[0].relevance >= found_docs[1].relevance) && (found_docs[1].relevance >= found_docs[2].relevance)),
            "Incorrect relevance sorting. Should be no-descending"s);
    }
}

//Тест проверяет, что работает фильтрация найденных документов по предикату, заданным пользователем
void TestPredicatSorting() {
    //Задаем тела документов
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 2;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 3;
    const string content2 = "dogs afraid of the black cat"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //Проверяем, что выполняется условие нечетности
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 != 0; });
        //        assert(found_docs.size() == 2);
        ASSERT_EQUAL_HINT(found_docs.size(), 2u, "Incorrect number of found documents"s);
        //        const Document& doc0 = found_docs[0];
        //        const Document& doc1 = found_docs[1];
        vector<int> correct_order = { doc2_id, doc0_id };
        vector<int> real_order = { found_docs[0].id , found_docs[1].id };
        //        assert(doc0.id == doc2_id);
        //        assert(doc1.id == doc0_id);
        ASSERT_EQUAL_HINT(real_order, correct_order, "Incorrect document order sorted by relevance or id");
    }

    //Проверяем, что выполняется условие четности
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect number of found documents"s);
        //        const Document& doc0 = found_docs[0];
        //        assert(doc0.id == doc1_id);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc1_id, "Incorrect document id");
    }
}

//Тест проверяет, что поисковая система корректно фильтрует найденные документы по статусу в запросе
void TestMatchingStatus() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова
    // находит документ с нужным статусом
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::IRRELEVANT);
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL(found_docs.size(), 1u);
        //       const Document& doc0 = found_docs[0];
       //        assert(doc0.id == doc_id);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в документ с неверным статусом,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        //        assert(server.FindTopDocuments("in"s).empty());
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Document with wrong status should be excluded"s);
    }

}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestPredicatSorting);
    RUN_TEST(TestMatchingStatus);
}

// --------- Окончание модульных тестов поисковой системы -----------



int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}