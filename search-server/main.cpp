#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

//Структура хранения данных документа
struct Document {
	int id;
	double relevance;
};

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}


class SearchServer {

public:

	//Перевод строки стоп-слов в вектор stop_words_
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {stop_words_.insert(word);}
    }

	//Добавление документов в базу в виде словаря ключевых слов {слово, set document_id}
    void AddDocument(int document_id, const string& document) {
		//На вход функции приходит id и одна строка со словами
		
		//Увеличиваем document_count_ на 1 при вызове AddDocument.
		++document_count_;
		
		//Создаем вектор со словами из строки, исключая стоп слова
		const vector<string> words = SplitIntoWordsNoStop(document);
		
		//Считаем долю одного слова в векторе
		double part = 1.0 / words.size();
		
		//В цикле нужно провести три этапа: добавление слова, рассчет TF, добавление нового doc_id
		//В основном цикле проходит проверка по наличию слова в словаре
		//Далее нужно проверить, равен ли текущий id указанным в словаре
		//Если такого id нет (первое упоминание документа), то внести id и указать только 1 part
		//Если такой id присутствует (в документе несколько одинаковых слов), то нужно увеличить на 1 part соответствующий id
		
		//Проходим по вектору слов (цикл прохода по словам принятого документа)
		for (const string& word : words){
			//Проверяем, есть ли слово в словаре word_to_documents_freqs_
			word_to_documents_freqs_[word][document_id] += part;
			
/*			if (word_to_documents_freqs_.count(word) == 0) {
				//Если слова нет, то добавляем пару в паре {word, {document_id, TF}}
				word_to_documents_freqs_[word][document_id] = part;
				//word_to_documents_freqs_[word][]; 							//создали слово
				//word_to_documents_freqs_.at(word)[document_id];				//создали в субсловаре id
				//word_to_documents_freqs_.at(word).at(document_id) = part;	//присвоили слову id значение TF = 1 part
			} else {
				//Слово есть, проверяем, был есть ли document_id (проверка на повтор слова)
				if (word_to_documents_freqs_.at(word).count(document_id) == 0) {
					//Слово есть, но id нет. Значит слово повторено в новом документе
					//добавляем только id и TF = 1 part
					word_to_documents_freqs_.at(word)[document_id] = part;				//создали в субсловаре id
					//word_to_documents_freqs_.at(word).at(document_id) = part;	//присвоили слову id значение part
				} else {
					//Слово уже есть, id тоже уже есть. Значит увеличиваем TF
					word_to_documents_freqs_.at(word).at(document_id) += part;	//TF увеличен на 1 part
				}
			}*/
		}
    }

	//Вывод вектора {id, relevance} с 5 самыми релевантными документами
    vector<Document> FindTopDocuments(const string& raw_query) const {
        //Вызов функции разбора строки запроса на слова
		const Query query_words = ParseQuery(raw_query);
		
		//Создание вектора по результатам вызова функции поиска всех документов
        auto matched_documents = FindAllDocuments(query_words);

		//Сортировка документов по убыванию релевантности
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {return lhs.relevance > rhs.relevance;});
		
		//Если найденных документов больше 5, то удаляем лишние
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
		
        return matched_documents;
    }

private:
//Хранение переменных и структур

	//Структура хранения разбитого поискового запроса
	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	//Словарь уникальных слов с id содерщащих их документов
	map<string, set<int>> word_to_documents_;
	
	//Словар уникальных слов с информацией о doc_id и TF
	map<string, map<int, double>> word_to_documents_freqs_;

	//Вспомогательный упорядоченный вектор для хранения стоп-слов
    set<string> stop_words_;
	
	//Количество документов заведенных в систему
	int document_count_ = 0;
	
//Хранение методов

	//Проверка на вхождение в список стоп-слов
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

	//Разбивка строки запроса на вектор слов, с удалением стоп-слов
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

	//Функция распределения слов поискового запроса на плюс и минус слова
    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
				query_words.minus_words.insert(word.substr(1));
			} else {
				query_words.plus_words.insert(word);
			}
        }
        return query_words;
    }

	//Функция поиска слов из запроса во всех документах с учетом TF-IDF
	vector<Document> FindAllDocuments(const Query query_words) const {
		//На входеполучаем структуру запроса из plus/minus слов
		
		//Перемнная для хранения словаря релевантных документов с TF-IDF
		map<int, double> document_to_relevance;
		
		//Проходим по плюс словам
		for (const string& word : query_words.plus_words) {
			//Находим плюс слово в word_to_documents_freqs_
			int match = count_if(word_to_documents_freqs_.begin(), word_to_documents_freqs_.end(),
								[word] (const pair<string, map<int, double>> w_t_d_f_) {return w_t_d_f_.first == word;});
			
			//Если слово найдено, то проводим рассчеты
			if (match != 0) {
				//Считаем IDF слова
				double word_IDF = log(static_cast<double> (document_count_) / word_to_documents_freqs_.at(word).size());
				
				//Добавляем информацию по документу в цикле для всех документов, связанных со словом
				for (const auto& [doc_id, temp] : word_to_documents_freqs_.at(word)) {
					//Если документа нет, то документ и текущий IDF-TF
					//Если документ есть, то добавляем к предыдущему значению IDF-TF текущее значение
					document_to_relevance[doc_id] += word_IDF * word_to_documents_freqs_.at(word).at(doc_id);
				}
			}
		}
		
		//Проходим по minus словам
		for (const string& word : query_words.minus_words) {
			//Находим минус слово в word_to_documents_freqs_
			int match = count_if(word_to_documents_freqs_.begin(), word_to_documents_freqs_.end(),
								[word] (const pair<string, map<int, double>> w_t_d_f_) {return w_t_d_f_.first == word;});
			
			//Если найдено слово, то удаляем документы с соответствующим id
			if (match !=0) {
				for (const auto& [doc_id, temp] : word_to_documents_freqs_.at(word)) {
					document_to_relevance.erase(doc_id);
				}
			}
		}
		
		//Переделываем map в vector
		vector<Document> doc_to_rel;
		for (auto [id, rel] : document_to_relevance) {
			doc_to_rel.push_back({id, document_to_relevance.at(id)});
		}
		return doc_to_rel;
	}
};

SearchServer CreateSearchServer() {
    //Создаем переменную класса SearchServer
	SearchServer search_server;
	
	//Передаем в сервер строку со стоп-словами
    search_server.SetStopWords(ReadLine());

	//Получаем информацию о количестве загружаемых документов
    const int document_count = ReadLineWithNumber();
	
	//Заполняем сервер документами
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    //Создаем новые сервер поиска
	const SearchServer search_server = CreateSearchServer();

	//Считывем строку поискового запроса
    const string query = ReadLine();
	
	//Проводим поиск и выводим на экран информацию о найденных документах
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << " }"s << endl;
    }
}
