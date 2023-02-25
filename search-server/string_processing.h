#pragma once
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <string_view>

// –азбивка строки на отдельные слова
std::vector<std::string_view> SplitIntoWords(std::string_view text);

// ”даление пустых ¤чеек массива, и добаление их в set дл¤ обеспечени¤ уникальности
template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(StringContainer& strings){
    // —писок дл¤ возврата уникальных значений
    std::set<std::string, std::less<>> non_empty_strings;

    // ѕропуск пустых ¤чеек из входного массива
    // emplace позвол¤ет преобразовать string_view в string без дополнитульных комманд
    for (auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}