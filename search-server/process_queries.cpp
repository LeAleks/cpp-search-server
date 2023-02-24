#include "process_queries.h"
#include <algorithm>
#include <execution>
#include <string_view>


std::vector<std::vector<Document>> ProcessQueries(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {

	// Контейнер для сбора данных для вывода
	std::vector<std::vector<Document>> documents_lists(queries.size());

	// Простое, но не эффективное решение
	// for (const std::string& query : queries) {
	// 	 documents_lists.push_back(search_server.FindTopDocuments(query));
	// }

	std::transform(
		std::execution::par,			// Включаем параллельное вычисление
		queries.begin(), queries.end(),	// Начало и конец входного диапазона
		documents_lists.begin(),		// Начало выходного диапазона
		[&search_server](std::string_view query) { return search_server.FindTopDocuments(query); }
	);

	return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {

	// Контейнер данных для вывода
	std::list<Document> found_documents;

	// Проводим параллельный поиск документов
	auto documents_lists = ProcessQueries(search_server, queries);

	// Простое, но не самое эффективное решение
	// Преобразуем вектор векторов в единый список
	for (auto& documents : documents_lists) {
		// Каждый отдельно найденный документ всталяем в список
		for (auto& document : documents) {
			found_documents.push_back(document);
		}
	}

	return found_documents;
}