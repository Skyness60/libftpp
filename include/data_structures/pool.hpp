/**
 * @file pool.hpp
 * @brief Fixed-capacity, header-only memory pool with RAII object proxies.
 * @version 1.0
 *
 * @details
 * Provides a pre-allocated, fixed-capacity pool of typed objects.
 * Memory is reserved once via @ref Pool::resize and reused across the
 * lifetime of the pool without any further dynamic allocation during
 * @ref Pool::acquire calls.
 *
 * Slot lookup and release are both O(1) via an embedded free-list stack.
 * Over-aligned types (e.g. @c alignas(64)) are handled correctly through
 * a manual over-allocation and pointer-alignment technique that relies
 * solely on @c new char[] and @c delete[] — no system allocator calls,
 * no C runtime headers required.
 *
 * @par Alignment strategy
 * @ref Pool::resize allocates a raw @c char buffer large enough to hold
 * @p capacity objects plus one extra alignment unit of padding.  It then
 * advances the buffer pointer to the first address that satisfies
 * @c alignof(TType) and stores the original @c char* just before that
 * aligned region so that @ref Pool::~Pool can recover and @c delete[] it.
 *
 * @par Zero-include policy
 * This header intentionally includes nothing.  The only language
 * primitives used are @c new[] / @c delete[], placement @c new,
 * @c alignof, @c sizeof, and @c decltype — all part of the core language,
 * requiring no header.
 *
 * @par Thread safety
 * None.  Synchronise externally if multiple threads share a pool.
 *
 * @par Ownership model
 * @ref Pool::Object is a unique-ownership RAII proxy.  It is move-only:
 * copy construction and copy assignment are deleted.  Destruction of the
 * proxy calls @c ~TType() on the managed object and returns the slot to
 * the pool without invoking @c delete.
 */

#pragma once

// ---------------------------------------------------------------------------

/**
 * @class Pool
 * @brief Fixed-capacity, allocation-free object pool.
 *
 * @tparam TType
 *   Type of the objects managed by this pool.  The type need not be
 *   default-constructible.  Over-aligned types (@c alignas(N)) are
 *   fully supported.
 *
 * @par Typical usage
 * @code
 * Pool<Bullet> bullets;
 * bullets.resize(256);                     // one-time allocation
 *
 * {
 *     auto b = bullets.acquire(x, y, vel); // O(1), no malloc
 *     b->update();
 * }                                        // slot auto-released here
 * @endcode
 */
template <typename TType>
class Pool
{
    // ------------------------------------------------------------------
    // Internal type utilities — avoids pulling in <type_traits>.
    // ------------------------------------------------------------------

    /** @cond INTERNAL */
    typedef decltype(sizeof(0)) size_type;

    template <typename T> struct ft_remove_ref      { typedef T type; };
    template <typename T> struct ft_remove_ref<T&>  { typedef T type; };
    template <typename T> struct ft_remove_ref<T&&> { typedef T type; };

    template <typename T>
    static constexpr T&& ft_forward(typename ft_remove_ref<T>::type& v) noexcept
    { return static_cast<T&&>(v); }

    template <typename T>
    static constexpr T&& ft_forward(typename ft_remove_ref<T>::type&& v) noexcept
    { return static_cast<T&&>(v); }
    /** @endcond */

public:

    // ==================================================================
    /**
     * @class Object
     * @brief Unique-ownership RAII proxy for a pool-managed instance.
     *
     * @details
     * Returned by @ref Pool::acquire.  When the @c Object is destroyed
     * or move-assigned over, it:
     *  -# calls @c TType::~TType() on the managed memory,
     *  -# returns the underlying slot to the parent pool in O(1).
     *
     * No heap deallocation is performed; the raw memory belongs to the
     * pool for its entire lifetime.
     *
     * An @c Object whose internal pointer is @c nullptr (returned when
     * the pool is exhausted) is safe to destroy and evaluates to
     * @c false in a boolean context.
     *
     * @note
     * The proxy holds a reference to its parent pool.  The pool must
     * therefore outlive all proxies it has issued.
     */
    class Object
    {
    public:

        /**
         * @brief Constructs a proxy bound to a specific pool slot.
         *
         * @param ptr    Pointer to the in-place-constructed object, or
         *               @c nullptr when the pool is exhausted.
         * @param pool   Reference to the originating @ref Pool.
         * @param index  Slot index within the pool's storage block.
         */
        Object(TType* ptr, Pool& pool, size_type index) noexcept
            : m_ptr(ptr), m_pool(pool), m_index(index)
        {}

        /**
         * @brief Destroys the managed object and releases the slot.
         *
         * @details
         * Calls @c TType::~TType() on the managed storage then pushes
         * the slot back onto the pool's free-list.  Does nothing if the
         * internal pointer is @c nullptr (exhausted-pool sentinel).
         */
        ~Object()
        {
            if (m_ptr)
            {
                m_ptr->~TType();
                m_pool.releaseSlot(m_index);
            }
        }

        /**
         * @brief Transfers ownership from @p other to this proxy.
         *
         * @param other  Source proxy; its pointer is set to @c nullptr
         *               so that its destructor becomes a no-op.
         */
        Object(Object&& other) noexcept
            : m_ptr(other.m_ptr), m_pool(other.m_pool), m_index(other.m_index)
        {
            other.m_ptr = nullptr;
        }

        /**
         * @brief Move-assigns ownership from @p other.
         *
         * @details
         * If @c *this currently manages an object, that object is
         * destroyed and its slot released before taking ownership of
         * @p other's resource.
         *
         * @param other  Source proxy; left in a valid, empty state.
         * @return       Reference to @c *this.
         */
        Object& operator=(Object&& other) noexcept
        {
            if (this != &other)
            {
                if (m_ptr)
                {
                    m_ptr->~TType();
                    m_pool.releaseSlot(m_index);
                }
                m_ptr   = other.m_ptr;
                m_index = other.m_index;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        /** @brief Copy construction is deleted — ownership is unique. */
        Object(const Object&) = delete;

        /** @brief Copy assignment is deleted — ownership is unique. */
        Object& operator=(const Object&) = delete;

        /**
         * @brief Provides pointer-style access to the managed object.
         * @return Pointer to the managed @c TType instance.
         */
        TType* operator->() noexcept { return m_ptr; }

        /** @brief @c const overload of @ref operator->. */
        const TType* operator->() const noexcept { return m_ptr; }

        /**
         * @brief Dereferences the managed object.
         * @return Reference to the managed @c TType instance.
         */
        TType& operator*() noexcept { return *m_ptr; }

        /** @brief @c const overload of @ref operator*. */
        const TType& operator*() const noexcept { return *m_ptr; }

        /**
         * @brief Tests whether this proxy holds a valid object.
         * @return @c true if the internal pointer is non-null.
         */
        explicit operator bool() const noexcept { return m_ptr != nullptr; }

    private:
        TType*    m_ptr;   ///< Pointer into the pool's raw storage block.
        Pool&     m_pool;  ///< Reference to the owning pool instance.
        size_type m_index; ///< Slot index used to release back to the pool.
    };
    // ==================================================================

    /**
     * @brief Default-constructs an empty, unallocated pool.
     * @note Call @ref resize before any @ref acquire.
     */
    Pool() noexcept
        : m_storage(nullptr)
        , m_rawBuffer(nullptr)
        , m_freeList(nullptr)
        , m_capacity(0)
        , m_freeTop(0)
    {}

    /**
     * @brief Destroys the pool and releases its raw memory block.
     *
     * @warning
     * All @ref Object instances issued by this pool must be destroyed
     * before the pool itself is destroyed.  A surviving proxy calling
     * @ref releaseSlot on a destroyed pool is undefined behaviour.
     */
    ~Pool()
    {
        delete[] m_rawBuffer;
        delete[] m_freeList;
    }

    /** @brief Pools are not copyable. */
    Pool(const Pool&) = delete;

    /** @brief Pools are not copy-assignable. */
    Pool& operator=(const Pool&) = delete;

    /**
     * @brief Pre-allocates storage for @p capacity objects.
     *
     * @details
     * Allocates a raw @c char buffer large enough to hold @p capacity
     * instances of @c TType with correct alignment, even for over-aligned
     * types such as @c alignas(64).
     *
     * The alignment is achieved without any system allocator call: the
     * buffer is over-allocated by @c (alignof(TType) - 1) bytes, then
     * the first usable pointer is advanced to the nearest address that
     * satisfies @c alignof(TType).  The original @c char* is kept in
     * @c m_rawBuffer so the destructor can @c delete[] it correctly.
     *
     * No @c TType constructor is invoked during @ref resize.
     *
     * @param capacity  Maximum number of live objects the pool can hold.
     *
     * @pre  @ref resize must not have been called previously on this
     *       pool instance.  A double call triggers undefined behaviour.
     *
     * @note Passing @p capacity = 0 is valid; every subsequent
     *       @ref acquire will immediately return an empty proxy.
     */
    void resize(size_type capacity)
    {
        if (m_rawBuffer)
        {
            return ;
        }

        m_capacity = capacity;
        m_freeTop  = capacity;

        if (capacity == 0)
            return;

        // Over-allocate by (align - 1) to guarantee we can find an
        // aligned address inside the buffer regardless of where the
        // allocator places it.
        const size_type align      = alignof(TType);
        const size_type bufferSize = capacity * sizeof(TType) + (align - 1);

        m_rawBuffer = new char[bufferSize];

        // Advance to the first address inside the buffer that satisfies
        // the alignment requirement of TType.
        const size_type raw  = reinterpret_cast<size_type>(m_rawBuffer);
        const size_type aligned = (raw + align - 1) & ~(align - 1);

        m_storage = reinterpret_cast<TType*>(aligned);

        m_freeList = new size_type[capacity];
        for (size_type i = 0; i < capacity; ++i)
            m_freeList[i] = i;
    }

    /**
     * @brief Constructs a @c TType in-place and returns its proxy.
     *
     * @details
     * Pops one slot from the internal free-list (O(1)), constructs a
     * @c TType at that address via placement @c new with perfect
     * argument forwarding, and wraps it in an @ref Object proxy.
     *
     * If the pool is exhausted, returns a proxy whose internal pointer
     * is @c nullptr (evaluates to @c false).
     *
     * If the @c TType constructor throws, the slot is pushed back onto
     * the free-list before the exception propagates — the pool remains
     * in a fully usable state.
     *
     * @tparam TArgs  Parameter types forwarded to @c TType's constructor.
     * @param  args   Arguments forwarded to @c TType's constructor.
     * @return        An @ref Object proxy managing the new instance, or
     *                an empty proxy if the pool is exhausted.
     *
     * @throws  Whatever @c TType's constructor may throw.
     */
    template <typename... TArgs>
    Object acquire(TArgs&&... args)
    {
        if (m_freeTop == 0)
            return Object(nullptr, *this, 0);

        const size_type index = m_freeList[--m_freeTop];

        try
        {
            TType* ptr = ::new (
                static_cast<void*>(m_storage + index)
            ) TType(ft_forward<TArgs>(args)...);

            return Object(ptr, *this, index);
        }
        catch (...)
        {
            m_freeList[m_freeTop++] = index;
            throw;
        }
    }

    /**
     * @brief Returns the total slot capacity set by @ref resize.
     * @return Maximum number of concurrently live objects.
     */
    size_type capacity() const noexcept { return m_capacity; }

    /**
     * @brief Returns the number of slots currently in use.
     * @return Number of live @ref Object instances issued by this pool.
     */
    size_type size() const noexcept { return m_capacity - m_freeTop; }

    /**
     * @brief Tests whether every slot is currently occupied.
     * @return @c true if @ref size() == @ref capacity().
     */
    bool full() const noexcept { return m_freeTop == 0; }

    /**
     * @brief Tests whether no slot is currently occupied.
     * @return @c true if @ref size() == 0.
     */
    bool empty() const noexcept { return m_freeTop == m_capacity; }

private:

    friend class Object;

    /**
     * @brief Returns slot @p index to the free-list in O(1).
     * @param index  Slot index to mark as available.
     */
    void releaseSlot(size_type index) noexcept
    {
        m_freeList[m_freeTop++] = index;
    }

    TType*     m_storage;   ///< First aligned TType address inside m_rawBuffer.
    char*      m_rawBuffer; ///< Original char[] buffer returned by new[].
    size_type* m_freeList;  ///< Stack of free slot indices — O(1) push / pop.
    size_type  m_capacity;  ///< Total number of slots allocated by resize().
    size_type  m_freeTop;   ///< Current top of the free-list stack.
};