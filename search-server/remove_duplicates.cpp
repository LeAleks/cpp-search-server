#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server, ostream& out) {
    set<int> duplicates_id; //Список id дубликатов
    auto range_start = search_server.begin(); //Итератор начала перебора документов

    for (auto id_0 : search_server) {
        //Составлем список слов для документа 0
        set<string> id_0_words;
        for (auto& [word, id] : search_server.GetWordFrequencies(id_0)) {
            id_0_words.insert(word);
        }

        //Проходим по другим документам
        for (auto doc_it = range_start; doc_it != search_server.end(); ++doc_it) {
            int id_n = *doc_it;
            //Пропускаем тот же id, и проверяем, что id уже не был включен в удаление
            if ((id_n == id_0) || duplicates_id.count(id_n)) {
                continue;
            }

            //Составлем список слов для документа n
            set<string> id_n_words;
            for (auto& [word, id] : search_server.GetWordFrequencies(id_n)) {
                id_n_words.insert(word);
            }

            //Сравниваем два списка
            bool duplicate = id_0_words == id_n_words;
            if (duplicate) {
                duplicates_id.insert(id_n);
            }
        }
        ++range_start;
    }

    //Удалям документы по списку дубликатов
    for (auto id : duplicates_id) {
        out << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}