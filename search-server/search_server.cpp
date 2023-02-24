#include <algorithm>
#include <cmath>

#include <stdexcept>

#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
    // ���������, ��� ����� ��������� �������
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }

    // ��������� ������ ������ ��������� �� �����, �������� ����-�����
    auto words = SplitIntoWordsNoStop(document);

    //������ ������� ����� � ���������� �� � �������
    const double inv_word_count = 1.0 / words.size();

    // ����� ���������� ������� �� ������� ����������
    map<string, double>& words_in_doc = documents_to_word_freqs_[document_id];

    // ������� ������� ����� � ���������
    for (auto& word : words) {
        word_to_document_freqs_[string{ word }][document_id] += inv_word_count;
        words_in_doc[string{ word }] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    // ������ ��������-������ �� �������
    auto predicate = [status](int document_id, DocumentStatus document_status, int rating) {return document_status == status; };

    return FindTopDocuments(raw_query, predicate);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}


std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

// ���������������� (������������) ������ MatchDocument
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    // ��������, ��� id ���� � ����
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("Document id not found"s);
    }

    // ����������� ������ ������� � ������ SearchServer::Query (2xvector<string_view>)
    auto query = ParseQuery(raw_query);

    // ��������� ��������� ��������� - ��� Query � list
    sort(query.plus_words.begin(), query.plus_words.end());
    auto new_end = unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(new_end, query.plus_words.end());

    sort(query.minus_words.begin(), query.minus_words.end());
    new_end = unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(new_end, query.minus_words.end());

    // ��������� ��� ����� ��������� ���� � ���������
    vector<string_view> matched_words;

    // ����� � ��������� ����� ���� �� �������. ���� ����� ������ - ������� ��������� � ��������� � ������
    bool minus_is_not_presented = true;
    for (auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(string{ word }) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string{ word }).count(document_id)) {
            matched_words.clear();
            minus_is_not_presented = false;
            break;
        }
    }

    // ����� � ��������� ���� ���� �� �������. ���� ����� ������� - ��������� � ���������
    if (minus_is_not_presented) {
        for (auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(string{ word }) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(string{ word }).count(document_id)) {
                matched_words.push_back(word);
            }
        }
    }

    // ������� ������ ��������� ���� (��� ������ ������) � ������ ���������
    return { matched_words, documents_.at(document_id).status };
}

// ���������������� (������ ����������) ������ MatchDocument
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
    const execution::sequenced_policy& policy,
    string_view raw_query,
    int document_id) const {
    return MatchDocument(raw_query, document_id);
}

// ������������ (�������������) ������ MatchDocument
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
    const execution::parallel_policy& policy,
    string_view raw_query,
    int document_id) const {
    
    // ��������, ��� id ���� � ����
    if (document_ids_.count(document_id) == 0) {
        throw std::out_of_range("Document id not found"s);
    }

    // ����������� ������ ������� � ������ SearchServer::Query (2xvector<string_view>)
    auto query = ParseQuery(raw_query);

    // ��������� ��� ����� � �������� ��������� ���� � ���������
    vector<string_view> matched_words;

    // ������ �� ������� ������������ � ��������� ����
    auto& words_in_document = documents_to_word_freqs_.at(document_id);

    // ��������� ������� ����-���� � ��������� � ������������ ������
    bool minus_is_presented = any_of(policy,
        query.minus_words.begin(), query.minus_words.end(),
        [&](auto& word) {return words_in_document.count(string{ word }); });

    // ���� ����� ���� ���, ��������� � ������ � ����������� ���� ����
    if (!minus_is_presented) {
        // ������ ������ ���������� - ��� Query � list
        matched_words.resize(query.plus_words.size());

        //������-������� ��� ������ ������ ���� ����
        auto is_presented = [&](auto& word) {
            bool presented = false;

            if (words_in_document.count(string{ word })) {
                presented = true;
            }

            return presented;
        };
        
        // ��������� ����� � ���������
        auto new_end = copy_if(policy,
            query.plus_words.begin(), query.plus_words.end(),
            matched_words.begin(),
            is_presented);

        //������ ������ ����������
        matched_words.erase(new_end, matched_words.end());
    }

    // �������� ������ ���� � ������� ����
    sort(policy, matched_words.begin(), matched_words.end());
    auto last = unique(policy, matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    // ������� ������ ��������� ���� (��� ������ ������) � ������ ���������
    return { matched_words, documents_.at(document_id).status };
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    //������ ������� ��� ��������� ������� ������� �� ����������� ������ �� ������ map
    static map<string_view, double> response_;
    response_.clear();

    // ���� ��������� ���, �� ���������� ������ ���������
    if (documents_to_word_freqs_.count(document_id) == 0) {
        return response_;
    }


    for (auto& word_freq : documents_to_word_freqs_.at(document_id)) {
        string_view word_view = word_freq.first;
        response_.insert({ word_view, word_freq.second });
    }

    return response_;
}

void SearchServer::RemoveDocument(int document_id) {
    if(document_ids_.count(document_id) == 0){
        return;
    }

    for (auto& [word, freq] : documents_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    documents_to_word_freqs_.erase(document_id);
}

// ������������� ������ � ������������ ���������
void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
    RemoveDocument(document_id);
}

// ������������� ������ � ������������� ����������
void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
    if (document_ids_.count(document_id) == 0) {
        return;
    }

    // ����������� ������� ��� document_id
    auto& words_frequency = documents_to_word_freqs_.at(document_id);

    // ������� ������ ��� �������� ���������� �� ����� � ���������
    vector<const string*> words(words_frequency.size());

    // ����������� ������� ������� � ������ ���� ��������� document_id
    transform(policy,
        words_frequency.begin(), words_frequency.end(),
        words.begin(),
        [](pair<const string, double>& word_freq) {return &word_freq.first; });

    // ������� ��� �������� ������ ��������� � �����
    auto erase_id = [&](auto& word) { word_to_document_freqs_.at(*word).erase(document_id); };

    // �������� ���������� �� ����
    for_each(policy,
        words.begin(), words.end(),
        erase_id);

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    documents_to_word_freqs_.erase(document_id);
}

//private

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.find(word) == stop_words_.end() ? 0 : 1;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    // ������ �������� ���� ��� ��������
    vector<string_view> words;

    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string{ word } + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }


    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    //string word = text;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw invalid_argument("Query word "s + string{ text } + " is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    // ������ �������� �������� �� ������ ����
    Query result;

    // ��������� ����� �� ������ � ������������ � ����� �����
    for (string_view word : SplitIntoWords(text)) {
        // ������� ��������� ��� ������������� ����� �� �������
        auto query_word = ParseQueryWord(word);

        // ��������� ��� ����� � ��������� � ��������������� ������
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);// - ��� Query � list
            }
            else {
                result.plus_words.push_back(query_word.data); // - ��� Query � list
            }
        }
    }

    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


