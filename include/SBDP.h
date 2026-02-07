// SPDX-License-Identifier: LicenseRef-SBPD-1.0
/******************************************************************************
 * @file    SBDP.h
 * @brief   SimpleBinaryDictionaryProtocol Class
 * @author  Satoh
 * @note    
 * Copyright (c) 2024 Satoh(3103lab.com)
 *****************************************************************************/
#pragma once

#include "SBDPTypedef.h"
#include "SBDPEndian.h"

#include <cstddef>
#include <vector>
#include <cstring>

namespace sbdp {
    constexpr std::size_t k_unHeaderSize = sizeof(uint32_t);
    constexpr std::size_t k_unKeyLengthSize = sizeof(uint16_t);
    constexpr std::size_t k_unInt64ValueSize = sizeof(int64_t);
    constexpr std::size_t k_unUInt64ValueSize = sizeof(uint64_t);
    constexpr std::size_t k_unFloat64ValueSize = sizeof(float64_t);
    constexpr std::size_t k_unStringLengthSize = sizeof(uint32_t);
    constexpr std::size_t k_unBinaryLengthSize = sizeof(uint32_t);
    /******************************************************************************
     * @brief   バッファへデータを追加するユーティリティ
     * @arg     vecBuffer   (in) バッファ
     * @arg     tData       (in) 追加するデータ
     * @return  なし
     * @note
     *****************************************************************************/
    template<typename T_>
    inline void AppendBytes(std::vector<uint8_t>& vecBuffer, const T_& tData) {
        const uint8_t* pData = reinterpret_cast<const uint8_t*>(&tData);
        vecBuffer.insert(vecBuffer.end(), pData, pData + sizeof(T_));
    }

    /******************************************************************************
     * @brief   ベクターの連結
     * @arg     vecBuffer   (in) 連結先ベクター
     * @arg     vecData     (in) 連結するデータ
     * @return  なし
     * @note
     *****************************************************************************/
    inline void AppendVector(std::vector<uint8_t>& vecBuffer,
                             const std::vector<uint8_t>& vecData) {
        vecBuffer.insert(vecBuffer.end(), vecData.begin(), vecData.end());
    }

    /******************************************************************************
     * @brief   SBDP プロトコルに基づくメッセージのエンコード
     * @arg     msgData   (in) エンコードするSBDPメッセージ
     * @return  エンコード結果
     * @note
     *****************************************************************************/
    inline std::vector<uint8_t> EncodeMessage(const Message& msgData) {
        std::vector<uint8_t> vecPayload;
        for (const auto& [strKey, svValue] : msgData) {
            uint16_t unKeyLen = static_cast<uint16_t>(strKey.size());
            uint16_t unNetKeyLen = htons(unKeyLen);
            AppendBytes(vecPayload, unNetKeyLen);
            vecPayload.insert(vecPayload.end(), strKey.begin(), strKey.end());

            if (std::holds_alternative<int64_t>(svValue)) {
                vecPayload.push_back(TYPE_INT64);
                int64_t snValue = std::get<int64_t>(svValue);
                int64_t snNetValue =
                    static_cast<int64_t>(htonll(static_cast<uint64_t>(snValue)));
                AppendBytes(vecPayload, snNetValue);
            }
            else if (std::holds_alternative<uint64_t>(svValue)) {
                vecPayload.push_back(TYPE_UINT64);
                uint64_t unValue = std::get<uint64_t>(svValue);
                uint64_t unNetValue = htonll(unValue);
                AppendBytes(vecPayload, unNetValue);
            }
            else if (std::holds_alternative<float64_t>(svValue)) {
                vecPayload.push_back(TYPE_FLOAT64);
                float64_t dbValue = std::get<float64_t>(svValue);
                uint64_t unNetValue = 0;
                std::memcpy(&unNetValue, &dbValue, sizeof(dbValue));
                unNetValue = htonll(unNetValue);
                AppendBytes(vecPayload, unNetValue);
            }
            else if (std::holds_alternative<std::string>(svValue)) {
                vecPayload.push_back(TYPE_STRING);
                const std::string& strValue = std::get<std::string>(svValue);
                uint32_t unStrLen = static_cast<uint32_t>(strValue.size());
                uint32_t unNetStrLen = htonl(unStrLen);
                AppendBytes(vecPayload, unNetStrLen);
                vecPayload.insert(vecPayload.end(), strValue.begin(), strValue.end());
            }
            else if (std::holds_alternative<std::vector<uint8_t>>(svValue)) {
                vecPayload.push_back(TYPE_BINARY);
                const std::vector<uint8_t>& vecBinary =
                    std::get<std::vector<uint8_t>>(svValue);
                uint32_t unBinLen = static_cast<uint32_t>(vecBinary.size());
                uint32_t unNetBinLen = htonl(unBinLen);
                AppendBytes(vecPayload, unNetBinLen);
                vecPayload.insert(vecPayload.end(), vecBinary.begin(), vecBinary.end());
            }
        }

        uint32_t unPayloadLen = static_cast<uint32_t>(vecPayload.size());
        uint32_t unNetPayloadLen = htonl(unPayloadLen);
        std::vector<uint8_t> vecMessage;
        AppendBytes(vecMessage, unNetPayloadLen);
        AppendVector(vecMessage, vecPayload);
        return vecMessage;
    }

    /******************************************************************************
     * @brief   SBDP プロトコルに基づくメッセージのデコード
     * @arg     vecMessage   (in) エンコードされたSBDPメッセージ
     * @return  デコード結果
     * @note
     *****************************************************************************/
    inline Message DecodeMessage(const std::vector<uint8_t>& vecMessage) {
        Message msgDecoded;
        size_t unOffset = 0;

        if (vecMessage.size() < k_unHeaderSize) {
            throw std::runtime_error("Message too short");
        }

        uint32_t unNetPayloadLen = 0;
        std::memcpy(&unNetPayloadLen, vecMessage.data(), k_unHeaderSize);
        unOffset += k_unHeaderSize;

        uint32_t unPayloadLen = ntohl(unNetPayloadLen);
        if (vecMessage.size() < k_unHeaderSize + unPayloadLen) {
            throw std::runtime_error("Incomplete message");
        }
        if (vecMessage.size() > k_unHeaderSize + unPayloadLen) {
            throw std::runtime_error("Message too big");
        }

        while (unOffset < k_unHeaderSize + unPayloadLen) {
            uint16_t unNetKeyLen = 0;
            if (unOffset + k_unKeyLengthSize > vecMessage.size()) {
                throw std::runtime_error("Key length read error");
            }
            std::memcpy(&unNetKeyLen, vecMessage.data() + unOffset, k_unKeyLengthSize);
            unOffset += k_unKeyLengthSize;

            uint16_t unKeyLen = ntohs(unNetKeyLen);
            if (unOffset + unKeyLen > vecMessage.size()) {
                throw std::runtime_error("Key string region insufficient");
            }

            std::string strKey(
                reinterpret_cast<const char*>(vecMessage.data() + unOffset),
                unKeyLen);
            unOffset += unKeyLen;

            if (unOffset >= vecMessage.size()) {
                throw std::runtime_error("Type code read error");
            }

            uint8_t unTypeCode = vecMessage[unOffset++];
            if (unTypeCode == TYPE_INT64) {
                if (unOffset + k_unInt64ValueSize > vecMessage.size()) {
                    throw std::runtime_error("int64 read error");
                }
                int64_t snNetValue = 0;
                std::memcpy(&snNetValue, vecMessage.data() + unOffset, k_unInt64ValueSize);
                unOffset += k_unInt64ValueSize;
                int64_t snValue =
                    static_cast<int64_t>(ntohll(static_cast<uint64_t>(snNetValue)));
                msgDecoded[strKey] = snValue;
            }
            else if (unTypeCode == TYPE_UINT64) {
                if (unOffset + k_unUInt64ValueSize > vecMessage.size()) {
                    throw std::runtime_error("uint64 read error");
                }
                uint64_t unNetValue = 0;
                std::memcpy(&unNetValue, vecMessage.data() + unOffset, k_unUInt64ValueSize);
                unOffset += k_unUInt64ValueSize;
                uint64_t unValue = ntohll(unNetValue);
                msgDecoded[strKey] = unValue;
            }
            else if (unTypeCode == TYPE_FLOAT64) {
                if (unOffset + k_unFloat64ValueSize > vecMessage.size()) {
                    throw std::runtime_error("float64_t read error");
                }
                uint64_t unNetValue = 0;
                std::memcpy(&unNetValue, vecMessage.data() + unOffset, k_unFloat64ValueSize);
                unOffset += k_unFloat64ValueSize;
                unNetValue = ntohll(unNetValue);
                float64_t dbValue = 0;
                std::memcpy(&dbValue, &unNetValue, sizeof(dbValue));
                msgDecoded[strKey] = dbValue;
            }
            else if (unTypeCode == TYPE_STRING) {
                if (unOffset + k_unStringLengthSize > vecMessage.size()) {
                    throw std::runtime_error("String length read error");
                }
                uint32_t unNetStrLen = 0;
                std::memcpy(&unNetStrLen, vecMessage.data() + unOffset, k_unStringLengthSize);
                unOffset += k_unStringLengthSize;
                uint32_t unStrLen = ntohl(unNetStrLen);
                if (unOffset + unStrLen > vecMessage.size()) {
                    throw std::runtime_error("String data insufficient");
                }
                std::string strValue(
                    reinterpret_cast<const char*>(vecMessage.data() + unOffset),
                    unStrLen);
                unOffset += unStrLen;
                msgDecoded[strKey] = strValue;
            }
            else if (unTypeCode == TYPE_BINARY) {
                if (unOffset + k_unBinaryLengthSize > vecMessage.size()) {
                    throw std::runtime_error("Binary length read error");
                }
                uint32_t unNetBinLen = 0;
                std::memcpy(&unNetBinLen, vecMessage.data() + unOffset, k_unBinaryLengthSize);
                unOffset += k_unBinaryLengthSize;
                uint32_t unBinLen = ntohl(unNetBinLen);
                if (unOffset + unBinLen > vecMessage.size()) {
                    throw std::runtime_error("Binary data insufficient");
                }
                std::vector<uint8_t> vecBinary(
                    vecMessage.begin() + unOffset,
                    vecMessage.begin() + unOffset + unBinLen);
                unOffset += unBinLen;
                msgDecoded[strKey] = vecBinary;
            }
            else {
                throw std::runtime_error("Unknown type code");
            }
        }

        return msgDecoded;
    }

} // namespace sbdp
