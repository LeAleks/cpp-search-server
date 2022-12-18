#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"
#include "log_duration.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"

#include <thread>


using namespace std;

int main() {
    TestSearchServer();

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    search_server.AddDocument(42, "fluffy fat cat in the cat house"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    for (auto [word, freq] : search_server.GetWordFrequencies(42)) {
        cout << word << " "s << freq << "\n";
    }
    cout << endl;

    for (auto id : search_server) {
        cout << id << " "s;
    }
    cout << endl;

    search_server.RemoveDocument(4);

    for (auto [word, freq] : search_server.GetWordFrequencies(4)) {
            cout << word << " "s << freq << "\n";
    }
    cout << endl;

    for (auto id : search_server) {
        cout << id << " "s;
    }
    cout << endl;


    SearchServer search_server1("and with"s);

    search_server1.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server1.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    search_server1.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    search_server1.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    search_server1.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    search_server1.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    search_server1.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    search_server1.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    search_server1.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    cout << "Before duplicates removed: "s << search_server1.GetDocumentCount() << endl;
    RemoveDuplicates(search_server1);
    cout << "After duplicates removed: "s << search_server1.GetDocumentCount() << endl;


    return 0;
}