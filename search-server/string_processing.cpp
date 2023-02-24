#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;

    //Смещаем начало до первого символа, не являющегося пробелом
    text.remove_prefix(min(text.find_first_not_of(" "), text.size()));

    // Указываем конец текста
    const uint64_t pos_end = text.npos;

    while (text.size()) {
        //Находим номер позиции первого пробела
        uint64_t space = text.find(' ');

        //Добавьте в результирующий вектор элемент string_view, полученный вызовом метода substr,
        //где начальная позиция будет 0, а конечная — найденная позиция пробела или npos.
        result.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));

        //Сдвиньте начало str так, чтобы оно указывало на позицию за пробелом.
        //Это можно сделать методом remove_prefix, передвигая начало str на указанное в аргументе
        //количество позиций.
        text.remove_prefix(min(text.find_first_not_of(" ", space), text.size()));

    }

    return result;
}