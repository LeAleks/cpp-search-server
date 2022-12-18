#pragma once

#include "search_server.h"

// ----- ���������� �������� ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT � RUN_TEST -----

//�������� << ��� ������ DocumentStatus
std::ostream& operator<<(std::ostream& out, const DocumentStatus& status);

//�������� << ��� ������ map<string, int>
std::ostream& operator<<(std::ostream& out, const std::map<std::string, int>& container);

//�������� << ��� ������ map<string, double>
std::ostream& operator<<(std::ostream& out, const std::map<std::string, double>& container);

//�������� << ��� ������ vector
template<typename t>
std::ostream& operator<<(std::ostream& out, const std::vector<t>& container);

//�������� << ��� ������ set
template<typename t>
std::ostream& operator<<(std::ostream& out, const std::set<t>& container);



//������� �������� ���� �������� �� ���������, � ����� ��������������� ����������
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
    const std::string& file, const std::string& func, unsigned line, const std::string& hint);



//������ �������� ���� �������� ��� ������ ���������
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

//������ �������� ���� �������� � ������� ���������
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//������� �������� ��� �������� �����, � ����� ��������������� ����������
void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
                unsigned line, const std::string& hint);



//������ �������� �������� ��� ������ ���������
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

//������ �������� �������� � ������� ���������
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void RunTestImpl(const T& t, const std::string& func_name);

#define RUN_TEST(func)  RunTestImpl(func, #func)


// -------- ������ ��������� ������ ��������� ������� ----------

//���� ���������, ��� ����� �������� �������� � ��������� �� �������� ������ �� ���� (����� ���� � ����� ����)
void TestAddingDocument();

// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent();

//���� ���������, ��� ��������� ������� ��������� ������� ���������, ���������� �����-����� �� ������
void TestExludeDocumentsWithMinusWords();

//���� ���������, ��� ����� ���������� ���������� �������� ���������
void TestMatchingDocuments();

//���� ��������� ������������ �������� �������������
void TestRelevanceCalculation();

//���� ���������, ��� relevance ������������� ���������, � ��������� ����������� � ������� ���������� relevance
void TestRelevanceSorting();

//���� ���������, ��� �������� ���������� ��������� ���������� �� ���������, �������� �������������
void TestFilteringByPredicat();

//���� ���������, ��� ��������� ������� ��������� ��������� ��������� ��������� �� ������� � �������
void TestMatchingStatus();

//���� ���������, ��� ��������� ������� ��������� ������� ���������� � ������� ���� � ���������
void TestGetWordFrequencies();

//���� ���������, ��� ��������� ������� ��������� ������� �������� �� id
void TestGetRemoveDocument();

//���� ���������, ��� ��������� ������� ��������� ������� �������� ��������� ����������
void TestRemoveDuplicate();


// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer();

// --------- ��������� ��������� ������ ��������� ������� -----------




// --------- ���� ������� �� ���������� -----------

//�������� << ��� ������ vector
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

//�������� << ��� ������ set
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


//������� �������� ���� �������� �� ���������, � ����� ��������������� ����������
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