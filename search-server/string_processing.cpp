#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;

    //������� ������ �� ������� �������, �� ����������� ��������
    text.remove_prefix(min(text.find_first_not_of(" "), text.size()));

    // ��������� ����� ������
    const uint64_t pos_end = text.npos;

    while (text.size()) {
        //������� ����� ������� ������� �������
        uint64_t space = text.find(' ');

        //�������� � �������������� ������ ������� string_view, ���������� ������� ������ substr,
        //��� ��������� ������� ����� 0, � �������� � ��������� ������� ������� ��� npos.
        result.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));

        //�������� ������ str ���, ����� ��� ��������� �� ������� �� ��������.
        //��� ����� ������� ������� remove_prefix, ���������� ������ str �� ��������� � ���������
        //���������� �������.
        text.remove_prefix(min(text.find_first_not_of(" ", space), text.size()));

    }

    return result;
}