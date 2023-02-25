#pragma once
#include "search_server.h"
#include "document.h"
#include <vector>
#include <string>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики,
    // сделав соотвествующие вызовы FindTopDocuments в каждый метод
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> search_result = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(search_result);
        return search_result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int64_t GetNoResultRequests() const;

private:
    void AddRequest(const std::vector<Document>& search_result);

    struct QueryResult {
        bool is_empty_result_;
        int number_in_line_;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int time_pass_ = 0;
};