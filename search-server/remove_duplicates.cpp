#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server, ostream& out) {
    set<int> duplicates_id; //Список id дубликатов
    vector<set<string>> unique_sets_of_words;

    for (int doc_id : search_server) {
        //Составляем список слов документа
        set<string> id_words;
        for (auto& [word, id] : search_server.GetWordFrequencies(doc_id)) {
            id_words.insert(word);
        }

        //Проверяем, что такого списка еще не было
        bool not_unique = count(unique_sets_of_words.begin(), unique_sets_of_words.end(), id_words);
 
        //Если был, то документ - дубликат. Если нет - то добавляем спсиок слов как уникальный
        if (not_unique) {
            duplicates_id.insert(doc_id);
        }
        else {
            unique_sets_of_words.push_back(id_words);
        }
    }

    //Удалям документы по списку дубликатов
    for (auto id : duplicates_id) {
        out << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}