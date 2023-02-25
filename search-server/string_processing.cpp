#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> result;

    //—мещаем начало до первого символа, не ¤вл¤ющегос¤ пробелом
    text.remove_prefix(min(text.find_first_not_of(" "), text.size()));

    // ”казываем конец текста
    const uint64_t pos_end = text.npos;

    while (text.size()) {
        //Ќаходим номер позиции первого пробела
        uint64_t space = text.find(' ');

        //ƒобавьте в результирующий вектор элемент string_view, полученный вызовом метода substr,
        //где начальна¤ позици¤ будет 0, а конечна¤ Ч найденна¤ позици¤ пробела или npos.
        result.push_back(space == pos_end ? text.substr(0) : text.substr(0, space));

        //—двиньте начало str так, чтобы оно указывало на позицию за пробелом.
        //Ёто можно сделать методом remove_prefix, передвига¤ начало str на указанное в аргументе
        //количество позиций.
        text.remove_prefix(min(text.find_first_not_of(" ", space), text.size()));

    }

    return result;
}