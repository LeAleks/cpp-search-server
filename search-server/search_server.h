#pragma once
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <execution>
#include <functional>
#include <string_view>

#include <type_traits>


#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_COMPARE_ACCURACY = 1e-6;
const size_t MAX_BUCKETS_DURING_SEARCH = 128;

class SearchServer {
public:
    // Конструктор из строки стоп-слов string
    SearchServer(const std::string& stop_words_text)
        : SearchServer(std::string_view(stop_words_text)){}

    // Конструктор из указателя строки стоп-слов string-view
    SearchServer(const std::string_view stop_words_text);

    // Конструктор из массива стоп-слов
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    // Добавление документа на сервер
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);


    // Поиск и вывод первых MAX_RESULT_DOCUMENT_COUNT наиболее релевантных документов 

    // Однопоточная версия FindTopDocuments
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    // Многопоточная версия FindTopDocuments с последовательным параметром
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query) const;


    // Многопоточная реализация FindTopDocuments с параллельным параметром   
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query) const;


    // Вывод количества документов в базе
    int GetDocumentCount() const;

    //Нужно убрать и заменить на begin & end
    //int GetDocumentId(int index) const;
    std::set<int>::iterator begin();
    std::set<int>::iterator end();

    // Последовательная (однопоточная) версия MatchDocument
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

    // Последовательная (задано параметром) версия MatchDocument
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        int document_id) const;

    // Параллельная (многопоточная) версия MatchDocument
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        int document_id) const;

    // Вывод слов с частотой для документа
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    // Многопоточная версия с однопоточным параметом
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);

    // Многопоточная версия с многопоточным параметом
    void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);


private:
    // --- structs ---
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    // Запрос для работы в параллельном режиме
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // --- variables ---

    // Список стоп-слов. Добавлен параметр less<> для работы со string_view
    const std::set<std::string, std::less<>> stop_words_;
    
    // Словарь слов: слово, (номер документа, частота слова в документе)
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    // Словарь документов: номер документа, (слово, частота слова в документе)
    std::map<int, std::map<std::string, double>> documents_to_word_freqs_;
    
    //Словарь документов: номер документа св-ва
    std::map<int, DocumentData> documents_;
    
    // Сортированный список документов. Указывается при добавлении документа
    // (в прошлом был vector и указывал порядок добавления)
    std::set<int> document_ids_;


    // --- methods ---

    bool IsStopWord(std::string_view word) const;
    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;

    // Последовательный парсинг
    Query ParseQuery(std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    // Однопоточная версия FindAllDocuments
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(Query& query, DocumentPredicate document_predicate) const;

    // Многопоточная версия FindAllDocuments с последовательным параметром
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const std::execution::sequenced_policy& policy,
        Query& query,
        DocumentPredicate document_predicate) const;

    // Многопоточная версия FindAllDocuments
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const std::execution::parallel_policy& policy,
        Query& query,
        DocumentPredicate document_predicate) const;
};


// ----- Тела функций из объявления класса -----

//public:
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument(std::string("Some of stop words are invalid"));
    }
}


// Однопоточная версия FindTopDocuments
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    
    // Выводит структуру Query (2xvector<string_view>)
    auto query = ParseQuery(raw_query);

    // Сортируем полуенный результат - для Query с list
    std::sort(query.plus_words.begin(), query.plus_words.end());
    auto new_end = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(new_end, query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    new_end = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(new_end, query.minus_words.end());

    // Находим все подходящеие документы
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_COMPARE_ACCURACY) {
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


// Многопоточная версия FindTopDocuments с последовательным параметром
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::sequenced_policy& policy,
    std::string_view raw_query,
    DocumentPredicate document_predicate) const{
    return FindTopDocuments(raw_query, document_predicate);
}


// Многопоточная реализация FindTopDocuments с параллельным параметром
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const std::execution::parallel_policy& policy,
    std::string_view raw_query,
    DocumentPredicate document_predicate) const {

    // Выводит структуру Query (2xvector<string_view>)
    auto query = ParseQuery(raw_query);

    // Сортируем полуенный результат - для Query с list
    std::sort(query.plus_words.begin(), query.plus_words.end());
    auto new_end = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(new_end, query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    new_end = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(new_end, query.minus_words.end());

    // Находим все подходящеие документы
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_COMPARE_ACCURACY) {
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



//private:

// Однопоточная версия FindAllDocuments
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(Query& query, DocumentPredicate document_predicate) const {
    // Контейнер для хранения найденных документов
    std::map<int, double> document_to_relevance;

    // Находим документы, содержащие плюс слова
    for (auto& word_view : query.plus_words) {
        // Преобразуем указатель в строку для возможности работы встроенных методов
        std::string word_string{ word_view };

        // Если слова нет, переходим к следующему слову
        if (word_to_document_freqs_.count(word_string) == 0) {
            continue;
        }

        // Считаем инверсированную частоту слова
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_string);

        // Проходим по связанному со словом словарю для доступа к документам, связанным с этим словом
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word_string)) {
            // Берем информацию о документе
            const auto& document_data = documents_.at(document_id);

            // Проверяем документ на доплнительные условаия. Если ок, то увеличиваем релевантность документа
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }


    // Находим докуметы, сожержащие минус слова
    for (auto& word_view : query.minus_words) {
        // Преобразуем указатель в строку для возможности работы встроенных методов
        std::string word_string{ word_view };

        // Если слова нет, переходим к следующему слову
        if (word_to_document_freqs_.count(word_string) == 0) {
            continue;
        }

        // Проверяем какие документы содержат минус слова и удаляем их из выдачи
        for (const auto [document_id, _] : word_to_document_freqs_.at(word_string)) {
            document_to_relevance.erase(document_id);
        }
    }

    // Контейнер для возврата найденных документов
    std::vector<Document> matched_documents;

    // Переносим документы из поиска в вывод, присваивая нужные параметры
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

// Многопоточная версия FindAllDocuments с последовательным параметром
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const std::execution::sequenced_policy& policy,
    Query& query,
    DocumentPredicate document_predicate) const {

    return FindAllDocuments(query, document_predicate);
}

// Многопоточная версия FindAllDocuments с параллельным параметром
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const std::execution::parallel_policy& policy,
    Query& query,
    DocumentPredicate document_predicate) const {

    // Контейнер для хранения найденных документов, 
    //std::map<int, double> document_to_relevance;
    ConcurrentMap<int, double> document_to_relevance(MAX_BUCKETS_DURING_SEARCH);

    // Проверяем есть ли слово в базе и, если есть, обрабатываем
    auto is_plus_presented = [&](auto& word_view) {
        // Преобразуем указатель в строку для возможности работы встроенных методов
        std::string word_string{ word_view };

        // Если слова нет, пропускаем его
        if (word_to_document_freqs_.count(word_string) != 0) {
            // Считаем инверсированную частоту слова
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_string);

            // Проходим по связанному со словом словарю для доступа к документам, связанным с этим словом
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word_string)) {
                // Берем информацию о документе
                const auto& document_data = documents_.at(document_id);

                // Проверяем документ на дополнительные условия. Если ок,
                // то увеличиваем релевантность документа
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    };

    // Находим документы, содержащие плюс слова
    for_each(policy,
        query.plus_words.begin(), query.plus_words.end(),
        is_plus_presented);

    
    // Проверяем есть ли слово в базе и, если есть, обрабатываем
    auto is_minus_presented = [&](auto& word_view) {
        // Преобразуем указатель в строку для возможности работы встроенных методов
        std::string word_string{ word_view };

        // Если слова нет, переходим к следующему слову
        if (word_to_document_freqs_.count(word_string) != 0) {
            // Проверяем какие документы содержат минус слова и удаляем их из выдачи
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word_string)) {
               // document_to_relevance.erase(document_id);
                document_to_relevance[document_id].ref_to_value = 0;
            }
        }
    };

    for_each(policy,
        query.minus_words.begin(), query.minus_words.end(),
        is_minus_presented);

    // Контейнер для возврата найденных документов
    std::vector<Document> matched_documents;

    // Переносим документы из поиска в вывод, присваивая нужные параметры
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        if (relevance > 0) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
    }


    return matched_documents;
}