#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server, ostream& out) {
    set<int> duplicates_id; //������ id ����������
    auto range_start = search_server.begin(); //�������� ������ �������� ����������

    for (auto id_0 : search_server) {
        //��������� ������ ���� ��� ��������� 0
        set<string> id_0_words;
        for (auto& [word, id] : search_server.GetWordFrequencies(id_0)) {
            id_0_words.insert(word);
        }

        //�������� �� ������ ����������
        for (auto doc_it = range_start; doc_it != search_server.end(); ++doc_it) {
            int id_n = *doc_it;
            //���������� ��� �� id, � ���������, ��� id ��� �� ��� ������� � ��������
            if ((id_n == id_0) || duplicates_id.count(id_n)) {
                continue;
            }

            //��������� ������ ���� ��� ��������� n
            set<string> id_n_words;
            for (auto& [word, id] : search_server.GetWordFrequencies(id_n)) {
                id_n_words.insert(word);
            }

            //���������� ��� ������
            bool duplicate = id_0_words == id_n_words;
            if (duplicate) {
                duplicates_id.insert(id_n);
            }
        }
        ++range_start;
    }

    //������ ��������� �� ������ ����������
    for (auto id : duplicates_id) {
        out << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}