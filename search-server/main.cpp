// -------- Начало модульных тестов поисковой системы ----------

//Перегруз << для вывода DocumentStatus. Required for correct operation
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


//Тест проверяет, что новый документ добавлен и находится по ключевым словам из него (кроме стоп и минус слов)
void TestAddingDocument() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    //Проверяем, что документ добавляется
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //        assert(server.GetDocumentCount() == 1);
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "Should be added one document"s);
    }

    //Проверяем, что все характеристи документа перенесены верно
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        //        assert(doc0.id == doc_id);
        ASSERT_EQUAL(doc0.id, doc_id);
        //        assert(doc0.rating == 2);
        ASSERT_EQUAL(doc0.rating, 2);
        ASSERT_EQUAL(doc0.relevance, 0);
        //        assert(doc0.relevance == 0);
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

//Тест проверяет, что поисковая система корректно удаляет докуместы, сожержащие минус-слова из поиска
void TestExludeDocumentsWithMinusWords() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список минус-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        //        assert(server.FindTopDocuments("-in"s).empty());
        ASSERT_HINT(server.FindTopDocuments("-in"s).empty(),
            "Document with minus word should be removed from search responce"s);
    }

    //Задаем второй документ, чтобы проверить, что удаляется только он, если только
    //в нем есть минус слово
    const int doc_id2 = 24;
    const string content2 = "cat in the night city"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //Проверяем, что находятся оба документа
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat night"s);
        //        assert(found_docs.size() == 2);
        ASSERT_EQUAL(found_docs.size(), 2u);
    }


    //Проверяем, что один документ удаляется
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat -night"s);
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Only one document should be found"s);
        const Document& doc0 = found_docs[0];
        //        assert(doc0.id == doc_id);
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Incorrect documend was removed"s);
    }

}

//Тест проверяет, что поиск подходящих документов работает корректно
void TestMatchingDocuments() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов или минус слов,
    // находит нужный документ и выводит все совпадающие слова из запроса и верный статус
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark city", doc_id);
        vector<string> right_order = { "cat"s, "city"s, "in"s };
        //        assert(matched_words[0] == "cat"s);
        //        ASSERT_EQUAL(matched_words[0], "cat");
        //        assert(matched_words[1] == "city"s);
        //        ASSERT_EQUAL(matched_words[1], "city");
        //        assert(matched_words[2] == "in"s);
        //        ASSERT_EQUAL(matched_words[2], "in");
        ASSERT_EQUAL_HINT(matched_words, right_order, "Incorrect order of found words"s);
        //        assert(status == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }

    //Проверяем, что наличие минус слова возвращает пустой список слов
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [matched_words, status] = server.MatchDocument("cat lost in a dark -city", doc_id);
        //        assert(matched_words.size() == 0);
        ASSERT_EQUAL_HINT(matched_words.size(), 0u, "Document with minus words should be excluded from the search responce"s);
        //        assert(status == DocumentStatus::ACTUAL);
        ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
    }
}

//Тест проверяет, что relevance рассчитывется корректно, и документы сортируются в порядке неубывания relevance
void TestRelevanceSorting() {
    //Задаем тела документов
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 10;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 100;
    const string content2 = "dogs afraid of the black man"s;
    const vector<int> ratings2 = { 1, 2, 3 };


    //Проверяем расчет relevance
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black"s);
        double relevance_accuracy = 0.001;
        double real_accuracy = abs((found_docs[0].relevance - 0.183102) / 0.183102);
        //        assert(real_accuracy <= relevance_accuracy);
        ASSERT_HINT((real_accuracy <= relevance_accuracy), "Incorrect calculated relevance");
    }

    // Заносим все три документа в базу. Проверяем, что документы сортируются не по возрастанию relevance
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const auto found_docs = server.FindTopDocuments("cat black"s);
        //        assert((found_docs[0].relevance >= found_docs[1].relevance) && (found_docs[1].relevance >= found_docs[2].relevance));
        ASSERT_HINT(((found_docs[0].relevance >= found_docs[1].relevance) && (found_docs[1].relevance >= found_docs[2].relevance)),
            "Incorrect relevance sorting. Should be no-descending"s);
    }
}

//Тест проверяет, что работает фильтрация найденных документов по предикату, заданным пользователем
void TestPredicatSorting() {
    //Задаем тела документов
    const int doc0_id = 1;
    const string content0 = "cat in the cat city"s;
    const vector<int> ratings0 = { 1, 2, 3 };

    const int doc1_id = 2;
    const string content1 = "cat in the countryside"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc2_id = 3;
    const string content2 = "dogs afraid of the black cat"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    //Проверяем, что выполняется условие нечетности
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 != 0; });
        //        assert(found_docs.size() == 2);
        ASSERT_EQUAL_HINT(found_docs.size(), 2u, "Incorrect number of found documents"s);
        //        const Document& doc0 = found_docs[0];
        //        const Document& doc1 = found_docs[1];
        vector<int> correct_order = { doc2_id, doc0_id };
        vector<int> real_order = { found_docs[0].id , found_docs[1].id };
        //        assert(doc0.id == doc2_id);
        //        assert(doc1.id == doc0_id);
        ASSERT_EQUAL_HINT(real_order, correct_order, "Incorrect document order sorted by relevance or id");
    }

    //Проверяем, что выполняется условие четности
    {
        SearchServer server;
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
        const vector<Document> found_docs = server.FindTopDocuments("black cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL_HINT(found_docs.size(), 1u, "Incorrect number of found documents"s);
        //        const Document& doc0 = found_docs[0];
        //        assert(doc0.id == doc1_id);
        ASSERT_EQUAL_HINT(found_docs[0].id, doc1_id, "Incorrect document id");
    }
}

//Тест проверяет, что поисковая система корректно фильтрует найденные документы по статусу в запросе
void TestMatchingStatus() {
    //Задаем тело документа
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    // Сначала убеждаемся, что поиск слова
    // находит документ с нужным статусом
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        const auto found_docs = server.FindTopDocuments("in"s, DocumentStatus::IRRELEVANT);
        //        assert(found_docs.size() == 1);
        ASSERT_EQUAL(found_docs.size(), 1u);
        //       const Document& doc0 = found_docs[0];
       //        assert(doc0.id == doc_id);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в документ с неверным статусом,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, ratings);
        //        assert(server.FindTopDocuments("in"s).empty());
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Document with wrong status should be excluded"s);
    }

}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestPredicatSorting);
    RUN_TEST(TestMatchingStatus);
}

// --------- Окончание модульных тестов поисковой системы -----------