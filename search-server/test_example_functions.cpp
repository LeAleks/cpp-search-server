// ----- ���������� �������� ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT � RUN_TEST -----

#include "test_example_functions.h"
#include "remove_duplicates.h"

#include <iostream>
#include <numeric>
#include <cmath>
#include <execution>
#include <functional>
#include <string_view>

using namespace std;

//�������� << ��� ������ DocumentStatus
ostream& operator<<(ostream& out, const DocumentStatus& status) {
    out << "Status: "s;
    switch (status) {
    case DocumentStatus::ACTUAL:
        out << "ACTUAL"s;
        break;
    case DocumentStatus::IRRELEVANT:
        out << "IRRELEVANT"s;
        break;
    case DocumentStatus::BANNED:
        out << "BANNED";
        break;
    case DocumentStatus::REMOVED:
        out << "REMOVED";
        break;
    default:
        out << "STATUS UNKONOWN"s;
        break;
    }
    return out;
}

//�������� << ��� ������ map<string, int>
ostream& operator<<(ostream& out, const map<string, int>& container) {
    out << "{"s;
    bool is_first = true;
    for (const auto& [name, age] : container) {
        if (is_first) {
            cout << name << ": " << age;
            is_first = false;
            continue;
        }
        out << ", " << name << ": " << age;
    }
    cout << "}"s;
    return out;
}

//�������� << ��� ������ map<string_view, int>
ostream& operator<<(ostream& out, const map<string_view, int>& container) {
    out << "{"s;
    bool is_first = true;
    for (const auto& [name, age] : container) {
        string name_string{ name };
        if (is_first) {
            cout << name_string << ": " << age;
            is_first = false;
            continue;
        }
        out << ", " << name_string << ": " << age;
    }
    cout << "}"s;
    return out;
}

//�������� << ��� ������ map<string, double>
ostream& operator<<(ostream& out, const map<string, double>& container) {
    out << "{"s;
    bool is_first = true;
    for (const auto& [name, age] : container) {
        if (is_first) {
            cout << name << ": " << age;
            is_first = false;
            continue;
        }
        out << ", " << name << ": " << age;
    }
    cout << "}"s;
    return out;
}

//�������� << ��� ������ map<string_view, double>
ostream& operator<<(ostream& out, const map<string_view, double>& container) {
    out << "{"s;
    bool is_first = true;
    for (const auto& [name, age] : container) {
        string name_string{ name };
        if (is_first) {
            cout << name_string << ": " << age;
            is_first = false;
            continue;
        }
        out << ", " << name_string << ": " << age;
    }
    cout << "}"s;
    return out;
}

//������� �������� ��� �������� �����, � ����� ��������������� ����������
void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}





// -------- ������ ��������� ������ ��������� ������� ----------

//���� ���������, ��� ����� �������� �������� � ��������� �� �������� ������ �� ���� (����� ���� � ����� ����)
void TestAddingDocument() {
    //������ ���� ���������
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    //���������, ��� �������� �����������
    {
        SearchServer server(""s);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "There should be no documents in the base"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "Should be added one document"s);
    }

    //���������, ��� ��� ������������ ��������� ���������� �����, rating ���������� ���������
    //(�.�. 1 ��������, �� relevance ������ ����� 0)
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
        int correct_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        ASSERT_EQUAL(doc0.rating, correct_rating);
        ASSERT_EQUAL(doc0.relevance, 0);
    }
}

// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // ������� ����������, ��� ����� �����, �� ��������� � ������ ����-����,
    // ������� ������ ��������
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // ����� ����������, ��� ����� ����� �� �����, ��������� � ������ ����-����,
    // ���������� ������ ���������
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

//���� ���������, ��� ��������� ������� ��������� ������� ���������, ���������� �����-����� �� ������
void TestExludeDocumentsWithMinusWords() {
    //������ ���� ���������
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // ������� ����������, ��� ����� �����, �� ��������� � ������ ����-����,
    // ������� ������ ��������
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // ����� ����������, ��� ����� ����� �� �����, ��������� � ������ �����-����,
    // ���������� ������ ���������
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("-in"s).empty(),
            "Document with minus word should be removed from search responce"s);
    }

    //������ ������ ��������, ����� ���������, ��� ��������� ������ ��, ���� ������
    //� ��� ���� ����� �����
    const int doc_id2 = 24;
    const string content2 = "cat in the night city"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //���������, ��� ��������� ��� ���������
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat night"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
    }


    //���������, ��� ���� �������� ���������
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat -night"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Only one document should be found"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect documend was removed"s);
    }

}

//���� ���������, ��� ����� ���������� ���������� �������� ���������
void TestMatchingDocuments() {
    //������ ���� ���������
    const int doc0_id = 42;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = { 1, 2, 3 };
    const int doc1_id = 24;
    const string content1 = "dog under water"s;
    const vector<int> ratings1 = { 3, 1, 2 };
    
    // ������� ����������, ��� ����� �����, �� ��������� � ������ ����-���� ��� ����� ����,
    // ������� ������ �������� � ������� ��� ����������� ����� �� ������� � ������ ������
    {
        SearchServer server(""sv);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark city", doc0_id);
        vector<string_view> right_order = { "cat"sv, "city"sv, "in"sv };
        ASSERT_EQUAL_HINT(matched_words, right_order, "Incorrect order of found words"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    
    //���������, ��� ��������, �� ���������� ���� �� �������, �� ����� ������� (���������� ������ ������ ����)
    {
        SearchServer server(""s);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark city", doc1_id);
        ASSERT_EQUAL_HINT(matched_words.size(), 0u, "Document without query words should be excluded from the search responce"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    
    //���������, ��� ������� ����� ����� ���������� ������ ������ ����
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark -city", doc0_id);
        ASSERT_EQUAL_HINT(matched_words.size(), 0u, "Document with minus words should be excluded from the search responce"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    
    //��������, ��� ������� ����� ����� �� ������ �� ����� ���������, �� ���������� ����� �����
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto [matched_words, status] = server.MatchDocument("city dog -cat", doc1_id);
        vector<string_view> right_order = { "dog"sv };
        ASSERT_EQUAL_HINT(matched_words, right_order, "Incorrect order of found words"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    

    // ---- �������� ������������� ������ -----

    // ������� ����������, ��� ����� �����, �� ��������� � ������ ����-���� ��� ����� ����,
    // ������� ������ �������� � ������� ��� ����������� ����� �� ������� � ������ ������
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument(execution::par, "cat lost in a dark city", doc0_id);
        vector<string_view> right_order = { "cat"sv, "city"sv, "in"sv };
        ASSERT_EQUAL_HINT(matched_words, right_order, "Incorrect order of found words"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    
    //���������, ��� ��������, �� ���������� ���� �� �������, �� ����� ������� (���������� ������ ������ ����)
    {
        SearchServer server(""s);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto [matched_words, status] = server.MatchDocument(execution::par, "cat lost in a dark city", doc1_id);
        ASSERT_EQUAL_HINT(matched_words.size(), 0u, "Document without query words should be excluded from the search responce"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    
    //���������, ��� ������� ����� ����� ���������� ������ ������ ����
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto [matched_words, status] = server.MatchDocument(execution::par, "cat lost in a dark -city", doc0_id);
        ASSERT_EQUAL_HINT(matched_words.size(), 0u, "Document with minus words should be excluded from the search responce"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
    
    //��������, ��� ������� ����� ����� �� ������ �� ����� ���������, �� ���������� ����� �����
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto [matched_words, status] = server.MatchDocument(execution::par, "city dog -cat", doc1_id);
        vector<string_view> right_order = { "dog"sv };
        ASSERT_EQUAL_HINT(matched_words, right_order, "Incorrect order of found words"s);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }

}

//���� ��������� ������������ �������� �������������
void TestRelevanceCalculation() {
    //������ ���� ����������
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 10;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 100;
    const string content2 = "dogs afraid of the black man"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //��������� ������ relevance ��� doc2_id - �� ���� �������� 'black'. ���������� ���� ���������� �����, ����� ��� ����������� �������� �� 1 �� ����� 0.
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black"s);
        double relevance_accuracy = 0.001;
        double TF = 1.0 / 6.0;
        double pre_IDF = 3.0 / 1.0;
        double IDF = log(pre_IDF);
        double TF_IDF = TF * IDF;
        double document_relevance_accuracy = abs((found_docs[0].relevance - TF_IDF) / TF_IDF);
        ASSERT_HINT((document_relevance_accuracy <= relevance_accuracy), "Incorrect calculated relevance");
    }

}

//���� ���������, ��� relevance ������������� ���������, � ��������� ����������� � ������� ���������� relevance
void TestRelevanceSorting() {
    //������ ���� ����������
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 10;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 100;
    const string content2 = "dogs afraid of the black man"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    // ������� ��� ��� ��������� � ����. ���������, ��� ��������� ����������� � ��������� �������
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat black"s);
        vector<int> correct_id_order = { 100, 1, 10 };
        vector<int> real_id_order = { found_docs[0].id, found_docs[1].id, found_docs[2].id };
        ASSERT_EQUAL_HINT(real_id_order, correct_id_order, "Incorrect order of found documents"s);
        ASSERT_HINT(((found_docs[0].relevance >= found_docs[1].relevance) && (found_docs[1].relevance >= found_docs[2].relevance)),
            "Incorrect relevance sorting. Should be no-descending"s);
    }
}

//���� ���������, ��� �������� ���������� ��������� ���������� �� ���������, �������� �������������
void TestFilteringByPredicat() {
    //������ ���� ����������
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 2;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 3;
    const string content2 = "dogs afraid of the black cat"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //���������, ��� ����������� ������� ����������
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 != 0; });
        ASSERT_EQUAL_HINT(found_docs.size(), 2u, "Incorrect number of found documents"s);
        vector<int> correct_order = { doc2_id, doc0_id };
        vector<int> real_order = { found_docs[0].id , found_docs[1].id };
        ASSERT_EQUAL_HINT(real_order, correct_order, "Incorrect document order sorted by relevance or id");
    }

    //���������, ��� ����������� ���������� �������
    {
        SearchServer server(""s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::BANNED, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::IRRELEVANT, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect number of found documents"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc1_id, "Incorrect document id");
    }
}

//���� ���������, ��� ��������� ������� ��������� ��������� ��������� ��������� �� ������� � �������
void TestMatchingStatus() {
    //������ ���� ���������
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // ������� ����������, ��� ����� �����
    // ������� �������� � ������ ��������
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
    }

    // ����� ����������, ��� ����� ����� �� �����, ��������� � �������� � �������� ��������,
    // ���������� ������ ���������
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Document with wrong status should be excluded"s);
    }

}

//���� ���������, ��� ��������� ������� ��������� ������� ���������� � ������� ���� � ���������

void TestGetWordFrequencies() {
    //������ ���� ���������
    const int doc_id = 42;
    const string content = "fluffy fat cat in the cat house"s;
    const vector<int> ratings = { 1, 2, 3 };

    //������� ������
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    //������� ��������� � ���������
    auto answer = server.GetWordFrequencies(42);
    map<string_view, double> right_answer{ {"cat"sv, 0.4}, { "fat"sv, 0.2 }, { "fluffy"sv, 0.2 }, { "house"sv, 0.2 } };
    ASSERT_EQUAL_HINT(answer, right_answer, "Incorrect word list or word frequency in the document");

}

//���� ���������, ��� ��������� ������� ��������� ������� �������� �� id
void TestGetRemoveDocument() {
    //������ ���� ����������
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 2;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 3;
    const string content2 = "dogs afraid of the black cat"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    // ���������������� �����
    {
        //������� ������
        SearchServer server("in the"s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);

        //������� ������ ��������
        server.RemoveDocument(2);

        {
            //���������, ��� �������� ������ id
            set<int> right_answer{ 1, 3 };
            set<int> answer(server.begin(), server.end());
            ASSERT_EQUAL_HINT(answer, right_answer, "Incorrect id list");
        }

        {
            //���������, ��� �������� ������ id
            set<int> right_answer{ 1, 3 };
            set<int> answer(server.begin(), server.end());
            ASSERT_EQUAL_HINT(answer, right_answer, "Incorrect id list");
        }

        {
            //���������, ��� �� ���������� id ��� ������� ������� ���� ������������ ������ �������
            ASSERT_HINT(server.GetWordFrequencies(2).empty(), "Returning map isn't empty"s);
        }

        {
            //��������, ��� ����� �� ����������� ����� ��� ��������� ����� ��� �� ������
            ASSERT_HINT(server.FindTopDocuments("countryside"s).empty(), "Returning map isn't empty"s);

        }

    }


    // ������������ �����
    {
        //������� ������
        SearchServer server("in the"s);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);

        //������� ������ ��������
        server.RemoveDocument(execution::par, 2);

        {
            //���������, ��� �������� ������ id
            set<int> right_answer{ 1, 3 };
            set<int> answer(server.begin(), server.end());
            ASSERT_EQUAL_HINT(answer, right_answer, "Incorrect id list");
        }

        {
            //���������, ��� �������� ������ id
            set<int> right_answer{ 1, 3 };
            set<int> answer(server.begin(), server.end());
            ASSERT_EQUAL_HINT(answer, right_answer, "Incorrect id list");
        }

        {
            //���������, ��� �� ���������� id ��� ������� ������� ���� ������������ ������ �������
            ASSERT_HINT(server.GetWordFrequencies(2).empty(), "Returning map isn't empty"s);
        }

        {
            //��������, ��� ����� �� ����������� ����� ��� ��������� ����� ��� �� ������
            ASSERT_HINT(server.FindTopDocuments("countryside"s).empty(), "Returning map isn't empty"s);

        }

    }

}

//���� ���������, ��� ��������� ������� ��������� ������� �������� ��������� ����������
void TestRemoveDuplicate() {
    //������� ������
    SearchServer search_server1("and with"s);

    search_server1.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server1.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // �������� ��������� 2, ����� �����
    search_server1.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ������� ������ � ����-������, ������� ����������
    search_server1.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ��������� ���� ����� ��, ������� ���������� ��������� 1
    search_server1.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ���������� ����� �����, ���������� �� ��������
    search_server1.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ��������� ���� ����� ��, ��� � id 6, �������� �� ������ �������, ������� ����������
    search_server1.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ���� �� ��� �����, �� �������� ����������
    search_server1.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // ����� �� ������ ����������, �� �������� ����������
    search_server1.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });


    //���������, ��� ������ id ���������� ����� �������� ���������� ������
    set<int> right_answer{ 1, 2, 6, 8, 9 };
    RemoveDuplicates(search_server1, cerr);
    set<int> answer(search_server1.begin(), search_server1.end());
    ASSERT_EQUAL_HINT(answer, right_answer, "Incorrect id list");
}

// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer() {
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestRelevanceCalculation);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestFilteringByPredicat);
    RUN_TEST(TestMatchingStatus);
    RUN_TEST(TestGetWordFrequencies);
    RUN_TEST(TestGetRemoveDocument);
    RUN_TEST(TestRemoveDuplicate);

}

// --------- ��������� ��������� ������ ��������� ������� -----------
