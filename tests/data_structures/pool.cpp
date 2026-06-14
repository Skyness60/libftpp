#include "../../include/libftpp.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

// =========================================================================
// TEST TYPES DEFINITIONS
// =========================================================================

// A heavy class with an internal pointer to check for leak hygiene.
struct HeavyResource {
    int* data;
    int id;

    explicit HeavyResource(int idValue)
        : data(new int[100]), id(idValue) {
        for (int i = 0; i < 100; ++i) {
            data[i] = id + i;
        }

        std::cout
            << "    [Heavy Ctor] #"
            << id
            << " allocated internal heap memory."
            << std::endl;
    }

    ~HeavyResource() {
        delete[] data;

        std::cout
            << "    [Heavy Dtor] #"
            << id
            << " freed internal heap memory."
            << std::endl;
    }

    HeavyResource(const HeavyResource&) = delete;
    HeavyResource& operator=(const HeavyResource&) = delete;
};


// A class with no default constructor and multiple overloads.
struct MultiParamObject {
    int a;
    double b;
    char c;

    explicit MultiParamObject(int x)
        : a(x), b(0.0), c('X') {}

    MultiParamObject(int x, double y, char z)
        : a(x), b(y), c(z) {}
};


// Tracks every construction and destruction.
struct LifetimeTracker {
    static int constructions;
    static int destructions;
    static int alive;

    int id;

    explicit LifetimeTracker(int value)
        : id(value) {
        ++constructions;
        ++alive;
    }

    ~LifetimeTracker() {
        ++destructions;
        --alive;
    }

    LifetimeTracker(const LifetimeTracker&) = delete;
    LifetimeTracker& operator=(const LifetimeTracker&) = delete;
};

int LifetimeTracker::constructions = 0;
int LifetimeTracker::destructions = 0;
int LifetimeTracker::alive = 0;


// Constructor capable of throwing intentionally.
struct ThrowingObject {
    static int attempts;
    static int constructions;
    static int destructions;
    static int alive;

    int value;

    explicit ThrowingObject(int input)
        : value(input) {
        ++attempts;

        if (input == 666) {
            throw std::runtime_error(
                "Intentional constructor failure"
            );
        }

        ++constructions;
        ++alive;
    }

    ~ThrowingObject() {
        ++destructions;
        --alive;
    }

    ThrowingObject(const ThrowingObject&) = delete;
    ThrowingObject& operator=(const ThrowingObject&) = delete;
};

int ThrowingObject::attempts = 0;
int ThrowingObject::constructions = 0;
int ThrowingObject::destructions = 0;
int ThrowingObject::alive = 0;


// Object accepting a move-only argument.
struct MoveOnlyObject {
    std::unique_ptr<int> resource;

    explicit MoveOnlyObject(std::unique_ptr<int> pointer)
        : resource(std::move(pointer)) {}

    MoveOnlyObject(const MoveOnlyObject&) = delete;
    MoveOnlyObject& operator=(const MoveOnlyObject&) = delete;
};


// Object requiring strong memory alignment.
struct alignas(64) AlignedObject {
    std::uint64_t values[8];

    explicit AlignedObject(std::uint64_t seed) {
        for (std::size_t i = 0; i < 8; ++i) {
            values[i] = seed + i;
        }
    }
};


// Large object used to detect overlapping slots.
struct LargeObject {
    static int alive;

    char bytes[8192];
    int id;

    explicit LargeObject(int value)
        : id(value) {
        ++alive;

        for (std::size_t i = 0; i < sizeof(bytes); ++i) {
            bytes[i] = static_cast<char>(
                (static_cast<std::size_t>(value) + i) % 127
            );
        }
    }

    ~LargeObject() {
        --alive;
    }

    LargeObject(const LargeObject&) = delete;
    LargeObject& operator=(const LargeObject&) = delete;
};

int LargeObject::alive = 0;


// Object containing several nested dynamic allocations.
struct NestedResource {
    static int alive;

    std::vector<std::unique_ptr<int[]> > blocks;

    NestedResource(
        std::size_t blockCount,
        std::size_t blockSize
    ) {
        ++alive;

        blocks.reserve(blockCount);

        for (std::size_t i = 0; i < blockCount; ++i) {
            std::unique_ptr<int[]> block(
                new int[blockSize]
            );

            for (std::size_t j = 0; j < blockSize; ++j) {
                block[j] = static_cast<int>(
                    i * blockSize + j
                );
            }

            blocks.push_back(std::move(block));
        }
    }

    ~NestedResource() {
        --alive;
    }

    NestedResource(const NestedResource&) = delete;
    NestedResource& operator=(const NestedResource&) = delete;
};

int NestedResource::alive = 0;


// Object used to verify that each pool slot is isolated.
struct IsolationObject {
    int id;
    int values[256];

    explicit IsolationObject(int identifier)
        : id(identifier) {
        for (std::size_t i = 0; i < 256; ++i) {
            values[i] = identifier + static_cast<int>(i);
        }
    }
};


// Object testing reference forwarding.
struct ReferenceObject {
    int& reference;

    explicit ReferenceObject(int& value)
        : reference(value) {}
};


// Object testing const-reference forwarding.
struct ConstReferenceObject {
    const std::string& reference;

    explicit ConstReferenceObject(
        const std::string& value
    )
        : reference(value) {}
};


// =========================================================================
// TEST UTILITIES
// =========================================================================

static void printTestTitle(
    int number,
    const char* title
) {
    std::cout
        << "\n[TEST "
        << number
        << "] "
        << title
        << std::endl;
}

static void printSuccess(const char* message) {
    std::cout
        << "  -> SUCCESS: "
        << message
        << std::endl;
}

static void printFailure(const char* message) {
    std::cout
        << "  -> FAILURE: "
        << message
        << std::endl;
}


// =========================================================================
// MAIN TEST SUITE
// =========================================================================

int main() {
    bool globalFailure = false;
    std::cout
        << "=================================================="
        << std::endl;
    std::cout
        << "     LIBFTPP - EXTREME EDGE-CASE TEST SUITE       "
        << std::endl;
    std::cout
        << "=================================================="
        << std::endl;

    // ---------------------------------------------------------------------
    // TEST 1: Basic acquisition
    // ---------------------------------------------------------------------
    printTestTitle(
        1,
        "Testing Basic Acquisition..."
    );

    {
        ftpp::Pool<int> pool;
        pool.resize(3);

        auto first = pool.acquire(10);
        auto second = pool.acquire(20);
        auto third = pool.acquire(30);

        if (first.operator->() != nullptr &&
            second.operator->() != nullptr &&
            third.operator->() != nullptr &&
            *first == 10 &&
            *second == 20 &&
            *third == 30) {
            printSuccess(
                "Three values were constructed correctly."
            );
        } else {
            printFailure(
                "Basic object acquisition failed."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 2: Pool exhaustion
    // ---------------------------------------------------------------------
    printTestTitle(
        2,
        "Testing Pool Exhaustion..."
    );

    {
        ftpp::Pool<int> pool;
        pool.resize(2);

        auto first = pool.acquire(1);
        auto second = pool.acquire(2);
        auto overflow = pool.acquire(3);

        if (first.operator->() != nullptr &&
            second.operator->() != nullptr &&
            overflow.operator->() == nullptr) {
            printSuccess(
                "Acquisition beyond capacity was rejected safely."
            );
        } else {
            printFailure(
                "Pool exhaustion behavior is invalid."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 3: Slot recycling
    // ---------------------------------------------------------------------
    printTestTitle(
        3,
        "Testing Slot Recycling..."
    );

    {
        ftpp::Pool<int> pool;
        pool.resize(1);

        void* firstAddress = nullptr;

        {
            auto first = pool.acquire(123);

            if (first.operator->() != nullptr) {
                firstAddress = static_cast<void*>(
                    first.operator->()
                );
            }
        }

        auto second = pool.acquire(456);

        if (second.operator->() != nullptr &&
            *second == 456 &&
            static_cast<void*>(second.operator->()) ==
                firstAddress) {
            printSuccess(
                "Released slot was reused at the same address."
            );
        } else {
            printFailure(
                "Released slot was not recycled correctly."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 4: Object isolation
    // ---------------------------------------------------------------------
    printTestTitle(
        4,
        "Testing Object Isolation..."
    );

    {
        ftpp::Pool<IsolationObject> pool;
        pool.resize(3);

        auto first = pool.acquire(1000);
        auto second = pool.acquire(2000);
        auto third = pool.acquire(3000);

        bool valid = (
            first.operator->() != nullptr &&
            second.operator->() != nullptr &&
            third.operator->() != nullptr
        );

        for (std::size_t i = 0; valid && i < 256; ++i) {
            if (first->values[i] !=
                    1000 + static_cast<int>(i) ||
                second->values[i] !=
                    2000 + static_cast<int>(i) ||
                third->values[i] !=
                    3000 + static_cast<int>(i)) {
                valid = false;
            }
        }

        if (valid) {
            printSuccess(
                "Neighboring objects do not overwrite each other."
            );
        } else {
            printFailure(
                "Memory overlap or data corruption detected."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 5: Perfect forwarding with constructor overloads
    // ---------------------------------------------------------------------
    printTestTitle(
        5,
        "Testing Multi-Overload Perfect Forwarding..."
    );

    {
        ftpp::Pool<MultiParamObject> pool;
        pool.resize(2);

        auto singleParameter = pool.acquire(42);
        auto multipleParameters = pool.acquire(
            10,
            3.14,
            'A'
        );

        if (singleParameter.operator->() != nullptr &&
            multipleParameters.operator->() != nullptr &&
            singleParameter->a == 42 &&
            singleParameter->b == 0.0 &&
            singleParameter->c == 'X' &&
            multipleParameters->a == 10 &&
            multipleParameters->b == 3.14 &&
            multipleParameters->c == 'A') {
            printSuccess(
                "Correct constructors were selected."
            );
        } else {
            printFailure(
                "Constructor argument forwarding corrupted data."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 6: Internal allocation cleanup
    // ---------------------------------------------------------------------
    printTestTitle(
        6,
        "Testing Internal Leak Safety..."
    );

    {
        ftpp::Pool<HeavyResource> pool;
        pool.resize(1);

        {
            std::cout
                << "  Acquiring heavy resource..."
                << std::endl;

            auto object = pool.acquire(999);

            if (object.operator->() == nullptr) {
                printFailure(
                    "HeavyResource acquisition failed."
                );
                globalFailure = true;
            }
        }

        printSuccess(
            "Temporary HeavyResource scope was closed."
        );
    }

    // ---------------------------------------------------------------------
    // TEST 7: Rapid recycling
    // ---------------------------------------------------------------------
    printTestTitle(
        7,
        "Testing Rapid Recycling..."
    );

    {
        ftpp::Pool<int> pool;
        pool.resize(1);

        void* constantAddress = nullptr;
        bool failed = false;

        for (int i = 0; i < 10000; ++i) {
            auto token = pool.acquire(i);

            if (token.operator->() == nullptr) {
                failed = true;
                break;
            }

            void* currentAddress = static_cast<void*>(
                token.operator->()
            );

            if (i == 0) {
                constantAddress = currentAddress;
            } else if (currentAddress != constantAddress) {
                failed = true;
                break;
            }

            if (*token != i) {
                failed = true;
                break;
            }
        }

        if (!failed && constantAddress != nullptr) {
            std::cout
                << "  -> SUCCESS: Completed 10000 cycles at "
                << constantAddress
                << "."
                << std::endl;
        } else {
            printFailure(
                "Rapid recycling changed address or corrupted data."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 8: Zero-sized pool
    // ---------------------------------------------------------------------
    printTestTitle(
        8,
        "Testing Zero-Size Pool Resiliency..."
    );

    {
        ftpp::Pool<double> pool;
        pool.resize(0);

        auto object = pool.acquire(3.14);

        if (object.operator->() == nullptr) {
            printSuccess(
                "Zero-sized pool rejected acquisition safely."
            );
        } else {
            printFailure(
                "Zero-sized pool returned an illegal object."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 9: Exact lifetime accounting
    // ---------------------------------------------------------------------
    printTestTitle(
        9,
        "Testing Exact Lifetime Accounting..."
    );

    LifetimeTracker::constructions = 0;
    LifetimeTracker::destructions = 0;
    LifetimeTracker::alive = 0;

    {
        ftpp::Pool<LifetimeTracker> pool;
        pool.resize(8);

        typedef decltype(pool.acquire(0)) Token;
        std::list<Token> objects;

        for (int i = 0; i < 8; ++i) {
            objects.push_back(pool.acquire(i));
        }

        if (LifetimeTracker::constructions == 8 &&
            LifetimeTracker::alive == 8) {
            printSuccess(
                "Exactly eight live objects were detected."
            );
        } else {
            printFailure(
                "Construction counters are invalid."
            );
            globalFailure = true;
        }

        std::list<Token>::iterator iterator =
            objects.begin();

        std::advance(iterator, 3);
        objects.erase(iterator);
        objects.pop_front();
        objects.pop_back();

        if (LifetimeTracker::alive == 5 &&
            LifetimeTracker::destructions == 3) {
            printSuccess(
                "Partial destruction accounting is correct."
            );
        } else {
            printFailure(
                "Partial destruction accounting is incorrect."
            );
            globalFailure = true;
        }

        objects.clear();

        if (LifetimeTracker::alive == 0 &&
            LifetimeTracker::constructions ==
                LifetimeTracker::destructions) {
            printSuccess(
                "Every object was destroyed exactly once."
            );
        } else {
            printFailure(
                "Missing or duplicated destructor calls detected."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 10: Constructor exception safety
    // ---------------------------------------------------------------------
    printTestTitle(
        10,
        "Testing Constructor Exception Safety..."
    );

    ThrowingObject::attempts = 0;
    ThrowingObject::constructions = 0;
    ThrowingObject::destructions = 0;
    ThrowingObject::alive = 0;

    {
        ftpp::Pool<ThrowingObject> pool;
        pool.resize(1);

        bool exceptionCaught = false;

        try {
            auto invalidObject = pool.acquire(666);
            (void)invalidObject;

            printFailure(
                "Expected exception was not propagated."
            );
            globalFailure = true;
        } catch (const std::runtime_error& exception) {
            exceptionCaught = true;

            std::cout
                << "  Constructor exception propagated: "
                << exception.what()
                << std::endl;
        } catch (...) {
            printFailure(
                "Unexpected exception type was propagated."
            );
            globalFailure = true;
        }

        auto recoveredObject = pool.acquire(42);

        if (exceptionCaught &&
            recoveredObject.operator->() != nullptr &&
            recoveredObject->value == 42 &&
            ThrowingObject::alive == 1) {
            printSuccess(
                "Failed construction did not poison the slot."
            );
        } else {
            printFailure(
                "Constructor failure permanently occupied the slot."
            );
            globalFailure = true;
        }
    }

    if (ThrowingObject::alive == 0 &&
        ThrowingObject::constructions ==
            ThrowingObject::destructions) {
        printSuccess(
            "Exception test produced no lifetime leak."
        );
    } else {
        printFailure(
            "Lifetime leak detected after exception test."
        );
        globalFailure = true;
    }

    // ---------------------------------------------------------------------
    // TEST 11: Move-only forwarding
    // ---------------------------------------------------------------------
    printTestTitle(
        11,
        "Testing Move-Only Argument Forwarding..."
    );

    {
        ftpp::Pool<MoveOnlyObject> pool;
        pool.resize(1);

        std::unique_ptr<int> originalResource(
            new int(123456)
        );

        auto object = pool.acquire(
            std::move(originalResource)
        );

        if (originalResource == nullptr &&
            object.operator->() != nullptr &&
            object->resource != nullptr &&
            *object->resource == 123456) {
            printSuccess(
                "unique_ptr ownership was transferred correctly."
            );
        } else {
            printFailure(
                "Move-only perfect forwarding is broken."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 12: Alignment
    // ---------------------------------------------------------------------
    printTestTitle(
        12,
        "Testing Over-Aligned Memory Storage..."
    );

    {
        ftpp::Pool<AlignedObject> pool;
        pool.resize(4);

        typedef decltype(pool.acquire(0)) Token;
        std::list<Token> objects;

        bool failed = false;

        for (std::uint64_t i = 0; i < 4; ++i) {
            objects.push_back(
                pool.acquire(i * 100)
            );

            AlignedObject* address =
                objects.back().operator->();

            if (address == nullptr) {
                failed = true;
                break;
            }

            const std::uintptr_t numericAddress =
                reinterpret_cast<std::uintptr_t>(
                    address
                );

            if (numericAddress %
                    alignof(AlignedObject) != 0) {
                failed = true;

                std::cout
                    << "  Misaligned address: "
                    << static_cast<void*>(address)
                    << std::endl;

                break;
            }
        }

        if (!failed) {
            std::cout
                << "  -> SUCCESS: Every slot respects "
                << alignof(AlignedObject)
                << "-byte alignment."
                << std::endl;
        } else {
            printFailure(
                "Pool violates alignment requirements."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 13: Exhaustion recovery
    // ---------------------------------------------------------------------
    printTestTitle(
        13,
        "Testing Exhaustion and Recovery..."
    );

    {
        ftpp::Pool<int> pool;
        pool.resize(2);

        void* releasedAddress = nullptr;

        {
            auto first = pool.acquire(10);
            auto second = pool.acquire(20);
            auto overflow = pool.acquire(30);

            if (first.operator->() != nullptr) {
                releasedAddress = static_cast<void*>(
                    first.operator->()
                );
            }

            if (first.operator->() != nullptr &&
                second.operator->() != nullptr &&
                overflow.operator->() == nullptr) {
                printSuccess(
                    "Third acquisition was rejected safely."
                );
            } else {
                printFailure(
                    "Pool exhaustion behavior is invalid."
                );
                globalFailure = true;
            }
        }

        auto recovered = pool.acquire(40);

        if (recovered.operator->() != nullptr &&
            *recovered == 40 &&
            static_cast<void*>(recovered.operator->()) ==
                releasedAddress) {
            printSuccess(
                "Pool recovered after complete exhaustion."
            );
        } else {
            printFailure(
                "Pool did not recover after tokens were released."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 14: Randomized allocation and release order
    // ---------------------------------------------------------------------
    printTestTitle(
        14,
        "Testing Randomized Recycling Patterns..."
    );

    LifetimeTracker::constructions = 0;
    LifetimeTracker::destructions = 0;
    LifetimeTracker::alive = 0;

    {
        ftpp::Pool<LifetimeTracker> pool;

        const std::size_t capacity = 64;
        const int cycles = 10000;

        pool.resize(capacity);

        typedef decltype(pool.acquire(0)) Token;
        std::list<Token> activeObjects;

        std::mt19937 generator(42);
        bool failed = false;

        for (int iteration = 0;
             iteration < cycles;
             ++iteration) {
            bool shouldAcquire = false;

            if (activeObjects.empty()) {
                shouldAcquire = true;
            } else if (activeObjects.size() ==
                       capacity) {
                shouldAcquire = false;
            } else {
                shouldAcquire =
                    (generator() % 2) == 0;
            }

            if (shouldAcquire) {
                Token token = pool.acquire(iteration);

                if (token.operator->() == nullptr) {
                    failed = true;
                    break;
                }

                activeObjects.push_back(
                    std::move(token)
                );
            } else {
                const std::size_t index =
                    generator() %
                    activeObjects.size();

                std::list<Token>::iterator iterator =
                    activeObjects.begin();

                std::advance(
                    iterator,
                    static_cast<std::ptrdiff_t>(index)
                );

                activeObjects.erase(iterator);
            }

            if (LifetimeTracker::alive !=
                static_cast<int>(
                    activeObjects.size()
                )) {
                failed = true;
                break;
            }
        }

        activeObjects.clear();

        if (!failed &&
            LifetimeTracker::alive == 0 &&
            LifetimeTracker::constructions ==
                LifetimeTracker::destructions) {
            std::cout
                << "  -> SUCCESS: Completed "
                << cycles
                << " randomized operations."
                << std::endl;
        } else {
            printFailure(
                "Random recycling corrupted pool state."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 15: Large objects
    // ---------------------------------------------------------------------
    printTestTitle(
        15,
        "Testing Large Object Slot Isolation..."
    );

    LargeObject::alive = 0;

    {
        ftpp::Pool<LargeObject> pool;
        pool.resize(3);

        auto first = pool.acquire(11);
        auto second = pool.acquire(22);
        auto third = pool.acquire(33);

        bool valid = (
            first.operator->() != nullptr &&
            second.operator->() != nullptr &&
            third.operator->() != nullptr
        );

        for (std::size_t i = 0;
             valid && i < sizeof(first->bytes);
             ++i) {
            const char expectedFirst =
                static_cast<char>(
                    (11 + i) % 127
                );

            const char expectedSecond =
                static_cast<char>(
                    (22 + i) % 127
                );

            const char expectedThird =
                static_cast<char>(
                    (33 + i) % 127
                );

            if (first->bytes[i] != expectedFirst ||
                second->bytes[i] != expectedSecond ||
                third->bytes[i] != expectedThird) {
                valid = false;
            }
        }

        if (valid &&
            first->id == 11 &&
            second->id == 22 &&
            third->id == 33 &&
            LargeObject::alive == 3) {
            printSuccess(
                "Large neighboring slots remain isolated."
            );
        } else {
            printFailure(
                "Large objects overlap or contain corrupted data."
            );
            globalFailure = true;
        }
    }

    if (LargeObject::alive == 0) {
        printSuccess(
            "All large objects were destroyed."
        );
    } else {
        printFailure(
            "Large object lifetime leak detected."
        );
        globalFailure = true;
    }

    // ---------------------------------------------------------------------
    // TEST 16: Nested heap cleanup
    // ---------------------------------------------------------------------
    printTestTitle(
        16,
        "Testing Nested Heap Allocation Cleanup..."
    );

    NestedResource::alive = 0;

    {
        ftpp::Pool<NestedResource> pool;
        pool.resize(4);

        typedef decltype(
            pool.acquire(
                static_cast<std::size_t>(0),
                static_cast<std::size_t>(0)
            )
        ) Token;

        std::list<Token> objects;

        for (std::size_t i = 0; i < 4; ++i) {
            objects.push_back(
                pool.acquire(
                    16 + i,
                    1024 + i * 256
                )
            );
        }

        bool valid = true;

        for (std::list<Token>::iterator iterator =
                 objects.begin();
             iterator != objects.end();
             ++iterator) {
            NestedResource* object =
                iterator->operator->();

            if (object == nullptr ||
                object->blocks.empty() ||
                object->blocks[0] == nullptr ||
                object->blocks[0][0] != 0) {
                valid = false;
                break;
            }
        }

        if (valid && NestedResource::alive == 4) {
            printSuccess(
                "Nested resources were allocated correctly."
            );
        } else {
            printFailure(
                "Nested resource state is corrupted."
            );
            globalFailure = true;
        }

        objects.clear();

        if (NestedResource::alive == 0) {
            printSuccess(
                "Every nested allocation was released."
            );
        } else {
            printFailure(
                "Nested allocation leak detected."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 17: Non-contiguous release and reuse
    // ---------------------------------------------------------------------
    printTestTitle(
        17,
        "Testing Non-Contiguous Slot Reuse..."
    );

    {
        ftpp::Pool<int> pool;
        pool.resize(5);

        typedef decltype(pool.acquire(0)) Token;

        std::list<Token> objects;

        for (int i = 0; i < 5; ++i) {
            objects.push_back(pool.acquire(i));
        }

        std::list<Token>::iterator middle =
            objects.begin();

        std::advance(middle, 2);

        void* middleAddress = static_cast<void*>(
            middle->operator->()
        );

        objects.erase(middle);

        auto replacement = pool.acquire(999);

        if (replacement.operator->() != nullptr &&
            *replacement == 999 &&
            static_cast<void*>(
                replacement.operator->()
            ) == middleAddress) {
            printSuccess(
                "Non-contiguous free slot was reused."
            );
        } else {
            printFailure(
                "Middle slot was not recycled correctly."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 18: Simultaneous pool independence
    // ---------------------------------------------------------------------
    printTestTitle(
        18,
        "Testing Independent Pools..."
    );

    {
        ftpp::Pool<int> firstPool;
        ftpp::Pool<int> secondPool;

        firstPool.resize(2);
        secondPool.resize(2);

        auto firstA = firstPool.acquire(100);
        auto firstB = firstPool.acquire(200);

        auto secondA = secondPool.acquire(300);
        auto secondB = secondPool.acquire(400);

        if (firstA.operator->() != nullptr &&
            firstB.operator->() != nullptr &&
            secondA.operator->() != nullptr &&
            secondB.operator->() != nullptr &&
            *firstA == 100 &&
            *firstB == 200 &&
            *secondA == 300 &&
            *secondB == 400 &&
            firstA.operator->() != secondA.operator->()) {
            printSuccess(
                "Separate pools maintain independent storage."
            );
        } else {
            printFailure(
                "Separate pools interfere with each other."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 19: Mutable reference forwarding
    // ---------------------------------------------------------------------
    printTestTitle(
        19,
        "Testing Mutable Reference Forwarding..."
    );

    {
        ftpp::Pool<ReferenceObject> pool;
        pool.resize(1);

        int externalValue = 42;

        auto object = pool.acquire(externalValue);

        if (object.operator->() != nullptr) {
            object->reference = 999;
        }

        if (externalValue == 999) {
            printSuccess(
                "Mutable lvalue reference was preserved."
            );
        } else {
            printFailure(
                "Reference argument was copied instead of forwarded."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // TEST 20: Const reference forwarding
    // ---------------------------------------------------------------------
    printTestTitle(
        20,
        "Testing Const Reference Forwarding..."
    );

    {
        ftpp::Pool<ConstReferenceObject> pool;
        pool.resize(1);

        const std::string externalString =
            "libftpp-reference-test";

        auto object = pool.acquire(externalString);

        if (object.operator->() != nullptr &&
            object->reference ==
                "libftpp-reference-test" &&
            &object->reference ==
                &externalString) {
            printSuccess(
                "Const lvalue reference was preserved."
            );
        } else {
            printFailure(
                "Const reference was copied or corrupted."
            );
            globalFailure = true;
        }
    }

    // ---------------------------------------------------------------------
    // FINAL RESULT
    // ---------------------------------------------------------------------
    std::cout
        << "\n=================================================="
        << std::endl;

    if (globalFailure) {
        std::cout
            << "       EXTREME TEST SUITE: FAILURE               "
            << std::endl;
    } else {
        std::cout
            << "       EXTREME TEST SUITE: SUCCESS               "
            << std::endl;
    }

    std::cout
        << "=================================================="
        << std::endl;

    return globalFailure ? 1 : 0;
}