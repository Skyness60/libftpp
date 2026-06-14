#pragma once

#include <vector>

template <typename TType>
class Pool {
    public:
        Pool() {
            m_pool.clear();
        }
        ~Pool() {
            m_pool.clear();
        }
        class Object {
            public:
                Object(TType* ptr) : m_ptr(ptr) {}
                ~Object() { delete m_ptr; }

                TType* operator->() { return m_ptr; }
                TType& operator*() { return *m_ptr; }

            private:
                TType* m_ptr;
        };

        void resize(size_t size) {
            m_pool.reserve(size);
        }


    private:
        std::vector<Object> m_pool;
};