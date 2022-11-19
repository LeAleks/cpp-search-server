#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const int RELEVANCE_ACCURACY = 1e-6;

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

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        //Проверка на неотрицательный id
        if (document_id < 0) {
            throw invalid_argument("AddDocument - Document id < 0"s);
        }

        //Проверка на уникальность id
        if (documents_.count(document_id)) {
            throw invalid_argument("AddDocument - Document id already exists"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document); //Добавить IsValidWord
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        documents_id_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < RELEVANCE_ACCURACY) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
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
        return tuple{ matched_words, documents_.at(document_id).status };

    }

    int GetDocumentId(int index) const {
        return documents_id_.at(index); //Либо выводит id, либо вызывает throw out_of_range
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> documents_id_; //Контейнер, в котором содержится информация о порядке добавления документов

    //Перенесено в приватный метод для оптимизации (выполнения замечаний ревью)
    vector<string> SplitIntoWords(const string& text) const {
        //Проверка на IsValidWord, которая требует, чтобы SplitIntoWords был приватным методом
        if (!IsValidWord(text)) {
            throw invalid_argument("SplitIntoWords - Invalid text"s);
        }

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

    //Перенесено в приватный метод для оптимизации (выполнения замечаний ревью)
    template <typename StringContainer>
    set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) const {
        //Проверка слов в контейнере на IsValidWord, из-за которой осуществлен перенос в приватную область
        for (const string& word : strings) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Server initialisation - Incorrect stop words in the container"s);
            }
        }

        set<string> non_empty_strings;
        for (const string& str : strings) {
            if (!str.empty()) {
                non_empty_strings.insert(str);
            }
        }
        return non_empty_strings;
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' '; //'-' вне этого списка
            });
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
        //Алгоритм перенесен из предыдущего спринта
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
        //Проверка на отсутсвие двух последовательных минусов и минуса без слова
        //Реализовано в ParseQuery, а не в ParseQueryWord, чтобы исключение срабатывало раньше,
        //чем выполнялись ненужные в таком случае действия (создание контейнера query и Split into words)
        int query_size = static_cast<int>(text.size());
        for (int i = 0; i < query_size; ++i) {
            if (text[i] == '-') {
                if ((i + 1) == query_size) {
                    throw invalid_argument("ParseQuery - '-' is in the query end"s);
                }
                else if (text[i + 1] == '-') {
                    throw invalid_argument("ParseQuery - Double '-'"s);
                }
                else if (text[i + 1] == ' ') {
                    throw invalid_argument("ParseQuery - No word after '-'"s);
                }
            }
        }

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

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
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


//Макрос для проверки исключений
#define TRY_TEST(func)\
{\
try {\
    func;\
}\
catch (const out_of_range&) {\
cout << "Error: GetDocumentId - value is out of range"s << endl;\
}\
catch (const invalid_argument& i) {\
    cout << "Error: "s << i.what() << endl;\
}\
}

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    SearchServer search_server("и в на"s);
    // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
    // о неиспользуемом результате его вызова
    (void)search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    cout << search_server.GetDocumentCount() << endl;

    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL)) {
        PrintDocument(document);
    }

    char faulty_char = 28;
    string faulty_string = faulty_char + string("кот"s);
    vector<string> faulty_container = { "cat"s, faulty_string, "dog"s };

    //Проверка инициализации search server
    {
        TRY_TEST(SearchServer search_server1(faulty_string));

        vector<string> faulty_container = { "cat"s, faulty_string, "dog"s };
        TRY_TEST(SearchServer search_server1(faulty_container));
    }
    //Проверка GetId
    {
        TRY_TEST(search_server.GetDocumentId(1));
    }


    //Проверка AddDocument
    {
        TRY_TEST(search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 }));
        TRY_TEST(search_server.AddDocument(-1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 }));
        TRY_TEST(search_server.AddDocument(-1, faulty_string, DocumentStatus::ACTUAL, { 7, 2, 7 }));
    }

    //Проверка FindTopDocuments
    {
        TRY_TEST(for (const Document& document : search_server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL)) {PrintDocument(document);});
        TRY_TEST(for (const Document& document : search_server.FindTopDocuments("--кот"s, DocumentStatus::ACTUAL)) {PrintDocument(document);});
        TRY_TEST(for (const Document& document : search_server.FindTopDocuments("кот-"s, DocumentStatus::BANNED)) {PrintDocument(document);});
        TRY_TEST(for (const Document& document : search_server.FindTopDocuments("кот- сом"s, DocumentStatus::BANNED)) {PrintDocument(document);});
        TRY_TEST(for (const Document& document : search_server.FindTopDocuments(faulty_string, DocumentStatus::BANNED)) {PrintDocument(document);});
    }

    //Проверка MatchedDocuments
    {
        TRY_TEST(search_server.MatchDocument("--пушистый ухоженный кот"s, 1));
        TRY_TEST(search_server.MatchDocument("- пушистый ухоженный кот"s, 1));
        TRY_TEST(search_server.MatchDocument("пушистый ухоженный кот-"s, 1));
        TRY_TEST(search_server.MatchDocument(faulty_string, 1));
    }
}