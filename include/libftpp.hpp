/**
 * @file libftpp.hpp
 * @brief Unified header file including all toolbox components.
 */
#pragma once

#include "data_structures/pool.hpp"

namespace ftpp {

    /**
     * @brief Template alias exposing the Pool class under the ftpp namespace.
     * @tparam TType The type of the resource elements managed within the pool.
     */
    template <typename TType>
    using Pool = ::Pool<TType>;

}