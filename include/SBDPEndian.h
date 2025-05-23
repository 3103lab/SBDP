// SPDX-License-Identifier: LicenseRef-SBPD-1.0
/******************************************************************************
 * @file    SBDPEndian.h
 * @brief   SimpleBinaryDictionaryProtocol Endian Utility
 * @author  Satoh
 * @note    Copyright (c) 2025 Satoh(3103lab.com)
 *****************************************************************************/
#pragma once

#include <cstdint>

namespace sbdp {

/******************************************************************************
 * @brief   16bit値をホストバイトオーダーからネットワークバイトオーダーへ変換
 * @arg     x (in) 変換する16bit値
 * @return  ネットワークバイトオーダーに変換された16bit値
 * @note    
 *****************************************************************************/
inline uint16_t htons(uint16_t x) {
    return (x << 8) | (x >> 8);
}

/******************************************************************************
 * @brief   16bit値をネットワークバイトオーダーからホストバイトオーダーへ変換
 * @arg     x (in) 変換する16bit値
 * @return  ホストバイトオーダーに変換された16bit値
 * @note    
 *****************************************************************************/
inline uint16_t ntohs(uint16_t x) {
    return htons(x);
}

/******************************************************************************
 * @brief   32bit値をホストバイトオーダーからネットワークバイトオーダーへ変換
 * @arg     x (in) 変換する32bit値
 * @return  ネットワークバイトオーダーに変換された32bit値
 * @note    
 *****************************************************************************/
inline uint32_t htonl(uint32_t x) {
    return ((x & 0x000000FFUL) << 24) |
           ((x & 0x0000FF00UL) << 8)  |
           ((x & 0x00FF0000UL) >> 8)  |
           ((x & 0xFF000000UL) >> 24);
}

/******************************************************************************
 * @brief   32bit値をネットワークバイトオーダーからホストバイトオーダーへ変換
 * @arg     x (in) 変換する32bit値
 * @return  ホストバイトオーダーに変換された32bit値
 * @note    
 *****************************************************************************/
inline uint32_t ntohl(uint32_t x) {
    return htonl(x);
}

/******************************************************************************
 * @brief   64bit値をホストバイトオーダーからネットワークバイトオーダーへ変換
 * @arg     x (in) 変換する64bit値
 * @return  ネットワークバイトオーダーに変換された64bit値
 * @note    
 *****************************************************************************/
inline uint64_t htonll(uint64_t x) {
    return ((x & 0x00000000000000FFULL) << 56) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x00000000FF000000ULL) << 8)  |
           ((x & 0x000000FF00000000ULL) >> 8)  |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0xFF00000000000000ULL) >> 56);
}

/******************************************************************************
 * @brief   64bit値をネットワークバイトオーダーからホストバイトオーダーへ変換
 * @arg     x (in) 変換する64bit値
 * @return  ホストバイトオーダーに変換された64bit値
 * @note    
 *****************************************************************************/
inline uint64_t ntohll(uint64_t x) {
    return htonll(x);
}


} // namespace sbdp
