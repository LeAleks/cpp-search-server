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
    // ����������� �� ������ ����-���� string
    SearchServer(const std::string& stop_words_text)
        : SearchServer(std::string_view(stop_words_text)){}

    // ����������� �� ��������� ������ ����-���� string-view
    SearchServer(const std::string_view stop_words_text);

    // ����������� �� ������� ����-����
    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    // ���������� ��������� �� ������
    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    // ����� � ����� ������ MAX_RESULT_DOCUMENT_COUNT �������� ����������� ���������� 
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;


    // ����� �������� FindTopDocuments

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(
        const ExecutionPolicy& policy,
        std::string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(
        const ExecutionPolicy& policy,
        std::string_view raw_query,
        DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(
        const ExecutionPolicy& policy,
        std::string_view raw_query) const;


    // ����� ���������� ���������� � ����
    int GetDocumentCount() const;

    //����� ������ � �������� �� begin & end
    //int GetDocumentId(int index) const;
    std::set<int>::iterator begin();
    std::set<int>::iterator end();

    // ���������������� (������������) ������ MatchDocument
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

    // ���������������� (������ ����������) ������ MatchDocument
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::sequenced_policy& policy,
        std::string_view raw_query,
        int document_id) const;

    // ������������ (�������������) ������ MatchDocument
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        const std::execution::parallel_policy& policy,
        std::string_view raw_query,
        int document_id) const;

    // ����� ���� � �������� ��� ���������
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    // ������������� ������ � ������������ ���������
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);

    // ������������� ������ � ������������� ���������
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

    // ������ ��� ������ � ������������ ������
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // --- variables ---

    // ������ ����-����. �������� �������� less<> ��� ������ �� string_view
    const std::set<std::string, std::less<>> stop_words_;
    
    // ������� ����: �����, (����� ���������, ������� ����� � ���������)
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    // ������� ����������: ����� ���������, (�����, ������� ����� � ���������)
    std::map<int, std::map<std::string, double>> documents_to_word_freqs_;
    
    //������� ����������: ����� ��������� ��-��
    std::map<int, DocumentData> documents_;
    
    // ������������� ������ ����������. ����������� ��� ���������� ���������
    // (� ������� ��� vector � �������� ������� ����������)
    std::set<int> document_ids_;


    // --- methods ---

    bool IsStopWord(std::string_view word) const;
    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;

    // ���������������� �������
    Query ParseQuery(std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    // ������������ ������ FindAllDocuments
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(Query& query, DocumentPredicate document_predicate) const;

    // ������������� ������ FindAllDocuments
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(
        const ExecutionPolicy&policy,
        Query& query,
        DocumentPredicate document_predicate) const;
};


// ----- ���� ������� �� ���������� ������ -----

//public:
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument(std::string("Some of stop words are invalid"));
    }
}


// ������������ ������ FindTopDocuments
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    
    // ������� ��������� Query (2xvector<string_view>)
    auto query = ParseQuery(raw_query);

    // ��������� ��������� ��������� - ��� Query � list
    std::sort(query.plus_words.begin(), query.plus_words.end());
    auto new_end = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(new_end, query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    new_end = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(new_end, query.minus_words.end());

    // ������� ��� ����������� ���������
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

// ����� �������� ������ FindTopDocuments

// ������������� ���������� FindTopDocuments
template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(
    const ExecutionPolicy& policy,
    std::string_view raw_query,
    DocumentPredicate document_predicate) const {

    // ������ ���� �������� ������
    const bool is_parallel = std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>;

    // ���� �� ������������, �� ��������� � ������������ ������
    if (!is_parallel) {
        return FindTopDocuments(raw_query, document_predicate);
        
    }


    // ������� ��������� Query (2xvector<string_view>)
    auto query = ParseQuery(raw_query);

    // ��������� ��������� ��������� - ��� Query � list
    std::sort(query.plus_words.begin(), query.plus_words.end());
    auto new_end = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(new_end, query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    new_end = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(new_end, query.minus_words.end());

    // ������� ��� ����������� ���������
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

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    const ExecutionPolicy& policy,
    std::string_view raw_query,
    DocumentStatus status) const {

    // ������ ��������-������ �� �������
    auto predicate = [status](int document_id, DocumentStatus document_status, int rating) {return document_status == status; };

    // ������ ���� �������� ������
    const bool is_parallel = std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>;

    if (is_parallel) {
        return FindTopDocuments(policy, raw_query, predicate);
    }
    else {
        return FindTopDocuments(raw_query, predicate);
    }
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(
    const ExecutionPolicy& policy,
    std::string_view raw_query) const {

    // ������ ���� �������� ������
    const bool is_parallel = std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>;

    if (is_parallel) {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }
    else {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
}



//private:

// ������������ ������ FindAllDocuments
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(Query& query, DocumentPredicate document_predicate) const {
    // ��������� ��� �������� ��������� ����������
    std::map<int, double> document_to_relevance;

    // ������� ���������, ���������� ���� �����
    for (auto& word_view : query.plus_words) {
        // ����������� ��������� � ������ ��� ����������� ������ ���������� �������
        std::string word_string{ word_view };

        // ���� ����� ���, ��������� � ���������� �����
        if (word_to_document_freqs_.count(word_string) == 0) {
            continue;
        }

        // ������� ��������������� ������� �����
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_string);

        // �������� �� ���������� �� ������ ������� ��� ������� � ����������, ��������� � ���� ������
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word_string)) {
            // ����� ���������� � ���������
            const auto& document_data = documents_.at(document_id);

            // ��������� �������� �� ������������� ��������. ���� ��, �� ����������� ������������� ���������
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }


    // ������� ��������, ���������� ����� �����
    for (auto& word_view : query.minus_words) {
        // ����������� ��������� � ������ ��� ����������� ������ ���������� �������
        std::string word_string{ word_view };

        // ���� ����� ���, ��������� � ���������� �����
        if (word_to_document_freqs_.count(word_string) == 0) {
            continue;
        }

        // ��������� ����� ��������� �������� ����� ����� � ������� �� �� ������
        for (const auto [document_id, _] : word_to_document_freqs_.at(word_string)) {
            document_to_relevance.erase(document_id);
        }
    }

    // ��������� ��� �������� ��������� ����������
    std::vector<Document> matched_documents;

    // ��������� ��������� �� ������ � �����, ���������� ������ ���������
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

// ������������� ������ FindAllDocuments
template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(
    const ExecutionPolicy& policy,
    Query& query,
    DocumentPredicate document_predicate) const {

    // ��������� ��� �������� ��������� ����������, 
    //std::map<int, double> document_to_relevance;
    ConcurrentMap<int, double> document_to_relevance(MAX_BUCKETS_DURING_SEARCH);

    // ��������� ���� �� ����� � ���� �, ���� ����, ������������
    auto is_plus_presented = [&](auto& word_view) {
        // ����������� ��������� � ������ ��� ����������� ������ ���������� �������
        std::string word_string{ word_view };

        // ���� ����� ���, ���������� ���
        if (word_to_document_freqs_.count(word_string) != 0) {
            // ������� ��������������� ������� �����
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_string);

            // �������� �� ���������� �� ������ ������� ��� ������� � ����������, ��������� � ���� ������
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word_string)) {
                // ����� ���������� � ���������
                const auto& document_data = documents_.at(document_id);

                // ��������� �������� �� �������������� �������. ���� ��,
                // �� ����������� ������������� ���������
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    };

    // ������� ���������, ���������� ���� �����
    for_each(policy,
        query.plus_words.begin(), query.plus_words.end(),
        is_plus_presented);

    
    // ��������� ���� �� ����� � ���� �, ���� ����, ������������
    auto is_minus_presented = [&](auto& word_view) {
        // ����������� ��������� � ������ ��� ����������� ������ ���������� �������
        std::string word_string{ word_view };

        // ���� ����� ���, ��������� � ���������� �����
        if (word_to_document_freqs_.count(word_string) != 0) {
            // ��������� ����� ��������� �������� ����� ����� � ������� �� �� ������
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word_string)) {
               // document_to_relevance.erase(document_id);
                document_to_relevance[document_id].ref_to_value = 0;
            }
        }
    };

    for_each(policy,
        query.minus_words.begin(), query.minus_words.end(),
        is_minus_presented);

    // ��������� ��� �������� ��������� ����������
    std::vector<Document> matched_documents;

    // ��������� ��������� �� ������ � �����, ���������� ������ ���������
    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        if (relevance > 0) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
    }


    return matched_documents;
}