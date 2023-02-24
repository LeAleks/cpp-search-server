#pragma once
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <string_view>

// Разбивка строки на отдельные слова
std::vector<std::string_view> SplitIntoWords(std::string_view text);

// Удаление пустых ячеек массива, и добаление их в set для обеспечения уникальности
template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(StringContainer& strings){
    // Список для возврата уникальных значений
    std::set<std::string, std::less<>> non_empty_strings;

    // Пропуск пустых ячеек из входного массива
    // emplace позволяет преобразовать string_view в string без дополнитульных комманд
    for (auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}