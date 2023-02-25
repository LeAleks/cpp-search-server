#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>


template <typename Key, typename Value>
class ConcurrentMap {
private:
    // Бликируемый подсловарь. Определяется в начале класса для
    // последующего использования
    struct Bucket {
        // Для оптимизации памяти указываем mutex в начале
        std::mutex m_;
        std::map<Key, Value> part_;
    };

public:
    // static_assert в начале класса не даст программе скомпилироваться при попытке
    // использовать в качестве типа ключа что-либо, кроме целых чисел
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    // Структура Access должна вести себя так же, как в шаблоне Synchronized (ур. 3):
    // предоставлять ссылку на значение словаря (подсловаря) и обеспечивать синхронизацию
    // доступа к нему.
    struct Access {
        // Инициализируем запрещенную область перед доступом к субсловарю и его значению
        std::lock_guard<std::mutex> guard;
        // Ссылка на значение из соотвествующего подсловаря
        Value& ref_to_value;

        // Конструктор структуры для получения ссылки значения из ссылки на
        // подсловарь
        Access(const Key& key, Bucket& bucket) :
            // Блокируем доступ к подсловарю из других потоков
            guard(bucket.m_),
            // Определяем ссылку на значение из подсловаря для key
            ref_to_value(bucket.part_[key]) {
        }
    };

    // Конструктор класса ConcurrentMap<Key, Value> принимает количество подсловарей,
    // на которые надо разбить всё пространство ключей
    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count) {}

    // operator[] должен вести себя так же, как аналогичный оператор у map:
    // если ключ key есть в словаре, должен возвращаться объект класса Access,
    // содержащий ссылку на соответствующее ему значение. Если key в словаре нет,
    // в него надо добавить пару (key, Value()) и вернуть объект класса Access,
    // содержащий ссылку на только что добавленное значение.
    Access operator[](const Key& key) {
        // Определяем к какому субсловарю относится ключ. Для этого проводим операцию
        // взятия остатка от деления. Остаток от деления показывает к какой группе
        // от 0 до buckets_.size()-1 относится ключ. Чтобы не учитывать "-" приводим
        // ключ к uint64_t, тем самым делая его просто большим положительным числом.
        // В результатет общий порядок ключей векторе мапов будет не сортирован, но в
        // самих map будет сортировка 
        auto bucket_index = static_cast<uint64_t>(key) % buckets_.size();

        // Получаем ссылку на подсловарь по индексу
        auto& bucket = buckets_[bucket_index];

        // Возвращаем ссылки на подсловарь и ключ
        return { key, bucket }; //Access to map key;
    }


    // Метод BuildOrdinaryMap должен сливать вместе части словаря и возвращать
    // весь словарь целиком. При этом он должен быть потокобезопасным,
    // то есть корректно работать, когда другие потоки выполняют операции
    // с ConcurrentMap.
    std::map<Key, Value> BuildOrdinaryMap() {
        // Промежуточный контейнер для возврата
        std::map<Key, Value> result;

        // Проходим циклом по всем субсловарям
        for (auto& [mutex, bucket] : buckets_) {
            // Блокируем доступ к субсловарю для други потоков
            std::lock_guard lock(mutex);

            // Переносим данные из подсловаря в контейнер возврата
            result.insert(bucket.begin(), bucket.end());
        }

        return result;
    }

private:
    // Котейнер хранения подсловарей, содержащий bucket_count частей
    std::vector<Bucket> buckets_;
};