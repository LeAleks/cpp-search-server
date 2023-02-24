#include "remove_duplicates.h"
#include <string_view>

using namespace std;

void RemoveDuplicates(SearchServer& search_server, ostream& out) {
    //������ id ����������
    set<int> duplicates_id;

    // ������ ���������� ������� ����
    vector<set<string>> unique_sets_of_words;

    // �������� �� ������ ���������� � ����
    for (int doc_id : search_server) {
        //���������� ������ ���� ���������
        set<string> id_words;
        for (auto& [word, id] : search_server.GetWordFrequencies(doc_id)) {
            string word_string{ word };
            id_words.insert(word_string);
        }

        //���������, ��� ������ ������ ��� �� ����
        bool not_unique = count(unique_sets_of_words.begin(), unique_sets_of_words.end(), id_words);
 
        //���� ���, �� �������� - ��������. ���� ��� - �� ��������� ������ ���� ��� ����������
        if (not_unique) {
            duplicates_id.insert(doc_id);
        }
        else {
            unique_sets_of_words.push_back(id_words);
        }
    }

    //������ ��������� �� ������ ����������
    for (auto id : duplicates_id) {
        out << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}