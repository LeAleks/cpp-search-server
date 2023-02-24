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
    // ����������� ����������. ������������ � ������ ������ ���
    // ������������ �������������
    struct Bucket {
        // ��� ����������� ������ ��������� mutex � ������
        std::mutex m_;
        std::map<Key, Value> part_;
    };

public:
    // static_assert � ������ ������ �� ���� ��������� ���������������� ��� �������
    // ������������ � �������� ���� ����� ���-����, ����� ����� �����
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    // ��������� Access ������ ����� ���� ��� ��, ��� � ������� Synchronized (��. 3):
    // ������������� ������ �� �������� ������� (����������) � ������������ �������������
    // ������� � ����.
    struct Access {
        // �������������� ����������� ������� ����� �������� � ���������� � ��� ��������
        std::lock_guard<std::mutex> guard;
        // ������ �� �������� �� ��������������� ����������
        Value& ref_to_value;

        // ����������� ��������� ��� ��������� ������ �������� �� ������ ��
        // ����������
        Access(const Key& key, Bucket& bucket) :
            // ��������� ������ � ���������� �� ������ �������
            guard(bucket.m_),
            // ���������� ������ �� �������� �� ���������� ��� key
            ref_to_value(bucket.part_[key]) {
        }
    };

    // ����������� ������ ConcurrentMap<Key, Value> ��������� ���������� �����������,
    // �� ������� ���� ������� �� ������������ ������
    explicit ConcurrentMap(size_t bucket_count) : buckets_(bucket_count) {}

    // operator[] ������ ����� ���� ��� ��, ��� ����������� �������� � map:
    // ���� ���� key ���� � �������, ������ ������������ ������ ������ Access,
    // ���������� ������ �� ��������������� ��� ��������. ���� key � ������� ���,
    // � ���� ���� �������� ���� (key, Value()) � ������� ������ ������ Access,
    // ���������� ������ �� ������ ��� ����������� ��������.
    Access operator[](const Key& key) {
        // ���������� � ������ ���������� ��������� ����. ��� ����� �������� ��������
        // ������ ������� �� �������. ������� �� ������� ���������� � ����� ������
        // �� 0 �� buckets_.size()-1 ��������� ����. ����� �� ��������� "-" ��������
        // ���� � uint64_t, ��� ����� ����� ��� ������ ������� ������������� ������.
        // � ����������� ����� ������� ������ ������� ����� ����� �� ����������, �� �
        // ����� map ����� ���������� 
        auto bucket_index = static_cast<uint64_t>(key) % buckets_.size();

        // �������� ������ �� ���������� �� �������
        auto& bucket = buckets_[bucket_index];

        // ���������� ������ �� ���������� � ����
        return { key, bucket }; //Access to map key;
    }


    // ����� BuildOrdinaryMap ������ ������� ������ ����� ������� � ����������
    // ���� ������� �������. ��� ���� �� ������ ���� ����������������,
    // �� ���� ��������� ��������, ����� ������ ������ ��������� ��������
    // � ConcurrentMap.
    std::map<Key, Value> BuildOrdinaryMap() {
        // ������������� ��������� ��� ��������
        std::map<Key, Value> result;

        // �������� ������ �� ���� �����������
        for (auto& [mutex, bucket] : buckets_) {
            // ��������� ������ � ���������� ��� ����� �������
            std::lock_guard lock(mutex);

            // ��������� ������ �� ���������� � ��������� ��������
            result.insert(bucket.begin(), bucket.end());
        }

        return result;
    }

private:
    // �������� �������� �����������, ���������� bucket_count ������
    std::vector<Bucket> buckets_;
};