#ifndef AR_ALLOCATORS_H
#define AR_ALLOCATORS_H

#include "ar/resource_pool.hpp"

namespace AsyncRuntime {

    class coroutine_handler;

    inline resource_pool * GetResource();

    template<class T>
    class Allocator {
    public:

        typedef typename std::size_t size_type;
        typedef typename std::size_t difference_type;
        typedef T*         pointer;
        typedef const T*   const_pointer;
        typedef T&         reference;
        typedef const T&   const_reference;
        typedef T          value_type;

        template<typename U>
        struct rebind {
            typedef Allocator<U> other;
        };

        Allocator() : resource(GetResource()) { }

        Allocator(int t) : resource(GetResource()), tag(t) { }

        explicit Allocator(const coroutine_handler *handler);

        Allocator(const coroutine_handler *handler, int t);

        explicit Allocator(resource_pool *res) : resource(res) { }

        Allocator(resource_pool *res, int t) : resource(res), tag(t) { }

        template<typename U>
        constexpr explicit Allocator(const Allocator<U> & other) noexcept : resource(other.get_resource()), tag(other.get_tag()) { }

        pointer address(reference __x) const { return &__x; }

        const_pointer address(const_reference __x) const { return &__x; }

        T* allocate(size_type n) const {
            return static_cast<T *>(resource->allocate(n * sizeof(T), tag));
        }

        void deallocate(T* p, size_type n) const noexcept {
            resource->deallocate(p, n * sizeof(T), tag);
        }

        void construct(T* p, const T& val) {
            ::new((void*)p) T(val);
        }

        template<class U, class... Args>
        void construct(U* p, Args&&... args) {
            ::new(p) U(std::forward<Args>(args)...);
        }

        template<class U>
        void construct(U* p) {
            ::new(p) U();
        }

        template< class... Args >
        pointer construct_at(Args&&... args ) const {
            auto *ptr = allocate(1);
            return ::new (ptr) T(std::forward<Args>(args)...);
        }

        void destroy_at(T* p) const {
            p->~T();
            deallocate(p, 1);
        }

        template< class U >
        void destroy(T* p) {
            p->~T();
        }

        template< class U >
        void destroy(U* p) {
            p->~U();
        }

        size_type max_size() const throw() {
            return std::size_t(-1) / sizeof(T);
        }

        resource_pool *get_resource() const { return resource; }
        int get_tag() const { return tag; }

    private:
        resource_pool *resource = nullptr;
        int tag = 0;
    };

    template <typename T, typename U>
    inline bool operator == (const Allocator<T>& a, const Allocator<U>& b) { return a.get_resource() == b.get_resource() && a.get_tag() == b.get_tag(); }

    template <typename T, typename U>
    inline bool operator != (const Allocator<T>& a, const Allocator<U>& b) { return a.get_resource() != b.get_resource() || a.get_tag() != b.get_tag(); }

    template<typename T, typename... Args>
    inline std::shared_ptr<T> make_shared_ptr(coroutine_handler *handler, Args&&... args) {
        Allocator<T> allocator(handler);
        return std::allocate_shared<T>(allocator, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline std::shared_ptr<T> make_shared_ptr(resource_pool *resource, Args&&... args) {
        Allocator<T> allocator(resource);
        return std::allocate_shared<T>(allocator, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline std::shared_ptr<T> make_shared_ptr(Args&&... args) {
        Allocator<T> allocator{};
        return std::allocate_shared<T>(allocator, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline std::unique_ptr<T, std::function<void(T *)>> make_unique_ptr(coroutine_handler *handler, Args&&... args) {
        Allocator<T> alloc(handler);
        return make_unique_ptr(alloc, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline std::unique_ptr<T, std::function<void(T *)>> make_unique_ptr(resource_pool *resource, Args&&... args) {
        Allocator<T> alloc(resource);
        return make_unique_ptr(alloc, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline std::unique_ptr<T, std::function<void(T *)>> make_unique_ptr(Args&&... args) {
        Allocator<T> alloc{};
        return make_unique_ptr(alloc, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    inline std::unique_ptr<T, std::function<void(T *)>> make_unique_ptr(Allocator<T> & alloc, Args&&... args) {
        auto *ptr = alloc.allocate(1);
        alloc.construct(ptr, std::forward<Args>(args)...);

        auto deleter = [](T *p, Allocator<T> alloc) {
            alloc.destroy(p);
            alloc.deallocate(p, 1);
        };

        return {ptr, std::bind(deleter, std::placeholders::_1, alloc)};
    }


    template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>>
    class map : public std::map<_Key, _Tp, _Compare, Allocator<std::pair<const _Key, _Tp>>> {
        using Alloc = Allocator<std::pair<const _Key, _Tp>>;
    public:
        map() = default;

        map(resource_pool *resource)
            :std::map<_Key, _Tp, _Compare, Alloc>(Alloc(resource)) {
        }
    };


    template<typename _Key, typename _Tp,
             typename _Hash = std::hash<_Key>,
             typename _Pred = std::equal_to<_Key>>
    class unordered_map : public std::unordered_map<_Key, _Tp, _Hash, _Pred, Allocator<std::pair<const _Key, _Tp>>> {
        using Alloc = Allocator<std::pair<const _Key, _Tp>>;
    public:
        unordered_map() = default;

        unordered_map(resource_pool *resource)
            :std::unordered_map<_Key, _Tp, _Hash, _Pred, Alloc>(Alloc(resource)) {
        }
    };

    template<typename _Tp>
    class list : public std::list<_Tp, Allocator<_Tp>> {
    public:
        list() = default;

        list(resource_pool *resource) : std::list<_Tp, Allocator<_Tp>>(Allocator<_Tp>(resource)) {

        }
    };

    template<typename _Tp>
    class vector : public std::vector<_Tp, Allocator<_Tp>> {
    public:
        vector() = default;

        vector(resource_pool *resource) : std::vector<_Tp, Allocator<_Tp>>(Allocator<_Tp>(resource)) {
        }
    };
}

#endif //AR_ALLOCATORS_H
