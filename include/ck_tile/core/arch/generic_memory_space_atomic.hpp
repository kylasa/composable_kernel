// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2024, Advanced Micro Devices, Inc. All rights reserved.

#pragma once
#include "ck_tile/core/numeric/vector_type.hpp"
#include "ck_tile/core/numeric/type_convert.hpp"
#include "ck_tile/core/container/thread_buffer.hpp"

namespace ck_tile {

template<typename T, typename ComputeType>
CK_TILE_HOST_DEVICE T add(const T& a, const T& b)
{
    return type_convert<T>(type_convert<ComputeType>(a) + type_convert<ComputeType>(b));
}

CK_TILE_HOST_DEVICE bf16x2_t add_bf16x2_t(const bf16x2_t& a, const bf16x2_t& b)
{
    bf16x2_t rtn;
    rtn[0] = add<bf16_t, float>(a[0], b[0]);
    rtn[1] = add<bf16_t, float>(a[1], b[1]);
    return rtn;
}

CK_TILE_HOST_DEVICE fp8x4_t add_fp8x4_t(const fp8x4_t& a, const fp8x4_t& b)
{
    fp8x4_t rtn;
    rtn[0] = add<fp8_t, float>(a[0], b[0]);
    rtn[1] = add<fp8_t, float>(a[1], b[1]);
    rtn[2] = add<fp8_t, float>(a[2], b[2]);
    rtn[3] = add<fp8_t, float>(a[3], b[3]);
    return rtn;
}

CK_TILE_HOST_DEVICE bf8x4_t add_bf8x4_t(const bf8x4_t& a, const bf8x4_t& b)
{
    bf8x4_t rtn;
    rtn[0] = add<bf8_t, float>(a[0], b[0]);
    rtn[1] = add<bf8_t, float>(a[1], b[1]);
    rtn[2] = add<bf8_t, float>(a[2], b[2]);
    rtn[3] = add<bf8_t, float>(a[3], b[3]);
    return rtn;
}

// Caution: DO NOT REMOVE
// intentionally have only declaration but no definition to cause compilation failure when trying to
// instantiate this template. The purpose is to make the implementation of atomic_add explicit for
// each datatype.
template <typename X>
CK_TILE_DEVICE void atomic_add(X* p_dst, const X& x);

template <>
CK_TILE_DEVICE void atomic_add<bf16x2_t>(bf16x2_t* p_dst, const bf16x2_t& x)
{
    union U32BF162_ADDR
    {
        uint32_t* u32_a;
        bf16x2_t* bf162_a;
    };

    union U32BF162
    {
        uint32_t u32;
        bf16x2_t bf162;
    };

    U32BF162_ADDR dword_addr;
    U32BF162 cur_v;
    U32BF162 new_;
    uint32_t old_v, new_v;
    dword_addr.bf162_a = p_dst;
    cur_v.u32          = *dword_addr.u32_a;

    do
    {
        old_v      = cur_v.u32;
        new_.bf162 = add_bf16x2_t(cur_v.bf162, x);
        new_v      = new_.u32;
        cur_v.u32  = atomicCAS(dword_addr.u32_a, old_v, new_v);
    } while(cur_v.u32 != old_v);
}

template<>
CK_TILE_DEVICE void atomic_add<fp8x4_t>(fp8x4_t* p_dst, const fp8x4_t& x)
{
    union U32FP84_ADDR
    {
        uint32_t* u32_a;
        fp8x4_t* fp84_a;
    };

    union U32FP84
    {
        uint32_t u32;
        fp8x4_t fp84;
    };

    U32FP84_ADDR dword_addr;
    U32FP84 cur_v;
    U32FP84 new_;
    uint32_t old_v, new_v;
    
    dword_addr.fp84_a = p_dst;
    cur_v.u32 = *dword_addr.u32_a;

    do{
        old_v       = cur_v.u32;
        new_.fp84   = add_fp8x4_t(cur_v.fp84, x);
        new_v       = new_.u32;
        cur_v.u32   = atomicCAS(dword_addr.u32_a, old_v, new_v);
    } while(cur_v.u32 != old_v);
}

template <typename T, index_t N>
CK_TILE_DEVICE void atomic_add_g(T* p_dst, const thread_buffer<T, N>& x)
{
    static_assert((std::is_same<T, int32_t>::value && (N == 1)) ||
                      (std::is_same<T, uint32_t>::value && (N == 1)) ||
                      (std::is_same<T, float>::value && (N == 1 || N == 2)) ||
                      (std::is_same<T, double>::value && (N == 1 || N == 2)) ||
                      (std::is_same<T, bf16_t>::value && (N == 2 || N == 4))
                      (std::is_same<T, fp8_t>::value && (N == 4)) || 
                      (std::is_same<T, bf8_t>::value && (N == 4)),
                  "wrong! not implemented");

    constexpr auto I0 = number<0>{};
    constexpr auto I1 = number<1>{};

    if constexpr(std::is_same<T, float>::value)
    {
        if constexpr(N == 1)
        {
            atomicAdd(p_dst, bit_cast<float>(x));
        }
        else if constexpr(N == 2)
        {
            atomicAdd(c_style_pointer_cast<float*>(p_dst), x.template get_as<float>()[I0]);
            atomicAdd(c_style_pointer_cast<float*>(p_dst) + 1, x.template get_as<float>()[I1]);
        }
    }
    else if constexpr(std::is_same<T, double>::value)
    {
        if constexpr(N == 1)
        {
            return atomicAdd(p_dst, bit_cast<double>(x));
        }
        else if constexpr(N == 2)
        {
            atomicAdd(c_style_pointer_cast<double*>(p_dst), x.template get_as<double>()[I0]);
            atomicAdd(c_style_pointer_cast<double*>(p_dst) + 1, x.template get_as<double>()[I1]);
        }
    }
    else if constexpr(std::is_same<T, int32_t>::value)
    {
        if constexpr(N == 1)
        {
            atomicAdd(p_dst, bit_cast<int32_t>(x));
        }
    }
    else if constexpr(std::is_same<T, uint32_t>::value)
    {
        if constexpr(N == 1)
        {
            atomicAdd(p_dst, bit_cast<uint32_t>(x));
        }
    }
    else if constexpr(std::is_same<T, bf16_t>::value)
    {
        if constexpr(N == 2)
        {
            atomic_add(c_style_pointer_cast<bf16x2_t*>(p_dst), bit_cast<bf16x2_t>(x));
        }
        else if constexpr(N == 4)
        {
            atomic_add(c_style_pointer_cast<bf16x2_t*>(p_dst), x.template get_as<bf16x2_t>()[I0]);
            atomic_add(c_style_pointer_cast<bf16x2_t*>(p_dst) + 1,
                       x.template get_as<bf16x2_t>()[I1]);
        }
    }
    else if constexpr(std::is_same<T, fp8_t>::value)
    {
        if constexpr(N == 4)
        {
            // Writing 4 fp8_t's to the destination 32 bits
            // which is same as one bf16x2_t in terms
            atomic_add(c_style_pointer_cast<fp8x4_t*>(p_dst), x.template get_as<fp8x4_t>()[I0]);
        }
    }
    else if constexpr(std::is_same<T, bf8_t>::value)
    {
        if constexpr(N == 4)
        {
            atomic_add(c_style_pointer_cast<bf8x4_t*>(p_dst), x.template get_as<bf8x4_t>()[I0]);
        }
    }    
}

template <typename T, index_t N>
CK_TILE_DEVICE void atomic_max_g(T* p_dst, const thread_buffer<T, N>& x)
{
    static_assert((std::is_same<T, int32_t>::value && (N == 1)) ||
                      (std::is_same<T, uint32_t>::value && (N == 1)) ||
                      (std::is_same<T, float>::value && (N == 1 || N == 2)) ||
                      (std::is_same<T, double>::value && (N == 1)),
                  "wrong! not implemented");

    constexpr auto I0 = number<0>{};
    constexpr auto I1 = number<1>{};

    if constexpr(std::is_same<T, float>::value)
    {
        if constexpr(N == 1)
        {
            atomicMax(p_dst, bit_cast<float>(x));
        }
        else if constexpr(N == 2)
        {
            atomicMax(c_style_pointer_cast<float*>(p_dst), x.template get_as<float>()[I0]);
            atomicMax(c_style_pointer_cast<float*>(p_dst) + 1, x.template get_as<float>()[I1]);
        }
    }
    else if constexpr(std::is_same<T, double>::value)
    {
        if constexpr(N == 1)
        {
            atomicMax(p_dst, bit_cast<double>(x));
        }
    }
    else if constexpr(std::is_same<T, int32_t>::value)
    {
        if constexpr(N == 1)
        {
            atomicMax(p_dst, bit_cast<int32_t>(x));
        }
    }
    else if constexpr(std::is_same<T, uint32_t>::value)
    {
        if constexpr(N == 1)
        {
            atomicMax(p_dst, bit_cast<uint32_t>(x));
        }
    }
}

} // namespace ck_tile
