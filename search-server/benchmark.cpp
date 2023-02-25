#include "benchmark.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"
#include "search_server.h"
#include "process_queries.h"

using namespace std;


// ----- Блок генераторов -----

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution(static_cast<int>('a'), static_cast <int>('z'))(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

vector<string> GenerateDictionaryNotSorted(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int max_word_count) {
    const int word_count = uniform_int_distribution(1, max_word_count)(generator);
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

string GenerateQueryWithMinus(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

vector<string> GenerateQueriesWithMinus(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQueryWithMinus(generator, dictionary, max_word_count));
    }
    return queries;
}

// ----- Проверка ProcessQueries -----

template <typename QueriesProcessor>
void Test(string_view mark, QueriesProcessor processor, const SearchServer& search_server, const vector<string>& queries) {
    LOG_DURATION(mark);
    const auto documents_lists = processor(search_server, queries);
}

#define TEST1(processor) Test(#processor, processor, search_server, queries)

void BenchmarkProcessQueries() {
    SearchServer search_server("and with"s);

    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 2'000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);
    SearchServer search_server1(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
    TEST1(ProcessQueries);
}

// ----- Проверка RemoveDocument -----

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
   
    const int document_count = search_server.GetDocumentCount();
    {
        LOG_DURATION(mark);
        for (int id = 0; id < document_count; ++id) {
            search_server.RemoveDocument(policy, id);
        }
    }
    cout << search_server.GetDocumentCount() << endl;
}

#define TEST2_1(mode) Test(#mode, search_server, execution::mode)

void Test(string_view mark, SearchServer search_server) {
    
    const int document_count = search_server.GetDocumentCount();
    {
        LOG_DURATION(mark);
        for (int id = 0; id < document_count; ++id) {
            search_server.RemoveDocument(id);
        }
    }
    cout << search_server.GetDocumentCount() << endl;
}

#define TEST2_2(mode) Test(#mode, search_server)

void BenchmarkRemoveDocument() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 10'000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);

    {
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }

        TEST2_2(orig);
    }
    {
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }

        TEST2_1(seq);
    }
    {
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
        }

        TEST2_1(par);
    }

}


// ----- Проверка MatchDocument -----

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
   
    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    {
        LOG_DURATION(mark);
        for (int id = 0; id < document_count; ++id) {
            const auto [words, status] = search_server.MatchDocument(policy, query, id);
            word_count += words.size();
        }
    }
    cout << word_count << endl;
}

#define TEST3(policy) Test(#policy, search_server, query, execution::policy)

void BenchmarkMatchDocument() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueriesWithMinus(generator, dictionary, 10'000, 70);

    const string query = GenerateQueryWithMinus(generator, dictionary, 500, 0.1);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    TEST3(seq);
    TEST3(par);
}


// ----- Проверка FindTopDocuments -----

template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    
    double total_relevance = 0;
    {
        LOG_DURATION(mark);
        for (const string_view query : queries) {
            for (const auto& document : search_server.FindTopDocuments(policy, query)) {
                total_relevance += document.relevance;
            }
        }
    }
    cout << total_relevance << endl;
}

#define TEST4(policy) Test(#policy, search_server, queries, execution::policy)

void BenchmarkFindTopDocuments() {
    cout << "Benchmark" << endl;
    mt19937 generator;

    const auto dictionary = GenerateDictionaryNotSorted(generator, 1000, 10);
    const auto documents = GenerateQueriesWithMinus(generator, dictionary, 10'000, 70);
    SearchServer search_server(dictionary[0]);

    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueriesWithMinus(generator, dictionary, 100, 70);

    TEST4(seq);
    TEST4(par);
}