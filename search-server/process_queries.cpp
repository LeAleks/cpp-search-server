#include "process_queries.h"
#include <algorithm>
#include <execution>
#include <string_view>


std::vector<std::vector<Document>> ProcessQueries(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {

	// ��������� ��� ����� ������ ��� ������
	std::vector<std::vector<Document>> documents_lists(queries.size());

	// �������, �� �� ����������� �������
	// for (const std::string& query : queries) {
	// 	 documents_lists.push_back(search_server.FindTopDocuments(query));
	// }

	std::transform(
		std::execution::par,			// �������� ������������ ����������
		queries.begin(), queries.end(),	// ������ � ����� �������� ���������
		documents_lists.begin(),		// ������ ��������� ���������
		[&search_server](std::string_view query) { return search_server.FindTopDocuments(query); }
	);

	return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {

	// ��������� ������ ��� ������
	std::list<Document> found_documents;

	// �������� ������������ ����� ����������
	auto documents_lists = ProcessQueries(search_server, queries);

	// �������, �� �� ����� ����������� �������
	// ����������� ������ �������� � ������ ������
	for (auto& documents : documents_lists) {
		// ������ �������� ��������� �������� �������� � ������
		for (auto& document : documents) {
			found_documents.push_back(document);
		}
	}

	return found_documents;
}