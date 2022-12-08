#include "request_queue.h"
#include <algorithm>

using namespace std;


//public

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
    //ѕрисвоили ссылку на исходный search_server внутренней переменной search_server_
    //дл€ возможности обращени€ к классу SearchServer внутри класса RequestQueue
}

int64_t RequestQueue::GetNoResultRequests() const {
    // напишите реализацию
    return count_if(requests_.begin(), requests_.end(), [](const QueryResult result) {return result.is_empty_result_ == true; });
}

vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    // напишите реализацию
    std::vector<Document> search_result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(search_result);
    return search_result;
}

vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    // напишите реализацию
    std::vector<Document> search_result = search_server_.FindTopDocuments(raw_query);
    AddRequest(search_result);
    return search_result;
}

//private
void RequestQueue::AddRequest(const vector<Document>& search_result) {
    QueryResult result;
    result.is_empty_result_ = search_result.empty();
    result.number_in_line_ = time_pass_;

    requests_.push_front(result);
    if (requests_.size() > min_in_day_) {
        requests_.resize(min_in_day_);
    }
    ++time_pass_;
}