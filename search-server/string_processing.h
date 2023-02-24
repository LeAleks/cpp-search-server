#pragma once
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <string_view>

// �������� ������ �� ��������� �����
std::vector<std::string_view> SplitIntoWords(std::string_view text);

// �������� ������ ����� �������, � ��������� �� � set ��� ����������� ������������
template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(StringContainer& strings){
    // ������ ��� �������� ���������� ��������
    std::set<std::string, std::less<>> non_empty_strings;

    // ������� ������ ����� �� �������� �������
    // emplace ��������� ������������� string_view � string ��� �������������� �������
    for (auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}