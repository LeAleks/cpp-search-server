#pragma once

#include "search_server.h"

// ----- Реализация макросов ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST -----

//Перегруз << для вывода DocumentStatus
std::ostream& operator<<(std::ostream& out, const DocumentStatus& status);

//Перегруз << для вывода map<string, int>
std::ostream& operator<<(std::ostream& out, const std::map<std::string, int>& container);

//Перегруз << для вывода map<string_view, int>
std::ostream& operator<<(std::ostream& out, const std::map<std::string_view, int>& container);

//Перегруз << для вывода map<string, double>
std::ostream& operator<<(std::ostream& out, const std::map<std::string, double>& container);

//Перегруз << для вывода map<string_view, double>
std::ostream& operator<<(std::ostream& out, const std::map<std::string_view, double>& container);

//Перегруз << для вывода vector
template<typename t>
std::ostream& operator<<(std::ostream& out, const std::vector<t>& container);

//Перегруз << для вывода set
template<typename t>
std::ostream& operator<<(std::ostream& out, const std::set<t>& container);



//Фнукция проверки двух значений на равенство, и вывод диагностической информации
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
    const std::string& file, const std::string& func, unsigned line, const std::string& hint);



//Макрос проверки двух занчений без вывода подсказки
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

//Макрос проверки двух занчений с выводом подсказки
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//Функция проверки что значение верно, и вывод диагностической информации
void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
                unsigned line, const std::string& hint);



//Макрос проверки значения без вывода подсказки
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

//Макрос проверки значения с выводом подсказки
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T& t, const std::string& func_name);

#define RUN_TEST(func)  RunTestImpl(func, #func)


// -------- Начало модульных тестов поисковой системы ----------

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------




// --------- Тела функций из объявления -----------

//Перегруз << для вывода vector
template<typename t>
std::ostream& operator<<(std::ostream& out, const std::vector<t>& container) {
    out << '[';
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            std::cout << element;
            is_first = false;
            continue;
        }
        out << ", " << element;
    }
    std::cout << ']';
    return out;
}

//Перегруз << для вывода set
template<typename t>
std::ostream& operator<<(std::ostream& out, const std::set<t>& container) {
    out << "{";
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            std::cout << element;
            is_first = false;
            continue;
        }
        out << ", " << element;
    }
    std::cout << "}";
    return out;
}


//Фнукция проверки двух значений на равенство, и вывод диагностической информации
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
    const std::string& file, const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "(" << line << "): " << func << ": ";
        std::cout << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cout << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        abort();
    }
}



template <typename T>
void RunTestImpl(const T& t, const std::string& func_name) {
    t();
    std::cerr << func_name << " OK" << std::endl;
}