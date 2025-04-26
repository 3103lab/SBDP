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

#include <vector>
#include <cstring>

namespace sbdp {
    /******************************************************************************
     * @brief   バッファへデータを追加するユーティリティ
     * @arg     buf   (in) バッファ
     * @arg     data  (in) 追加するデータ
     * @return  なし
     * @note
     *****************************************************************************/
    template<typename T>
    inline void append_bytes(std::vector<uint8_t>& buf, const T& data) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&data);
        buf.insert(buf.end(), p, p + sizeof(T));
    }

    /******************************************************************************
     * @brief   ベクターの結合
     * @arg     buf   (in) 結合先のベクター
     * @arg     data  (in) 結合するデータ
     * @return  なし
     * @note
     *****************************************************************************/
    inline void append_vector(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data) {
        buf.insert(buf.end(), data.begin(), data.end());
    }

    /******************************************************************************
     * @brief   SBDP プロトコルに基づくメッセージのエンコード
     * @arg     msg   (in) エンコードするSBDPメッセージ
     * @return  エンコード結果
     * @note
     *****************************************************************************/
    inline std::vector<uint8_t> encode_message(const Message& msg) {
        std::vector<uint8_t> payload;
        for (const auto& [key, value] : msg) {
            // キー: キー長 (2バイト, ネットワークオーダー) + キー文字列 (UTF-8)
            uint16_t key_len = static_cast<uint16_t>(key.size());
            uint16_t net_key_len = htons(key_len);
            append_bytes(payload, net_key_len);
            payload.insert(payload.end(), key.begin(), key.end());
            // 型コード (1バイト) と値
            if (std::holds_alternative<int64_t>(value)) {
                payload.push_back(TYPE_INT64);
                int64_t v = std::get<int64_t>(value);
                int64_t net_v = static_cast<int64_t>(htonll(static_cast<uint64_t>(v)));
                append_bytes(payload, net_v);
            }
            else if (std::holds_alternative<uint64_t>(value)) {
                payload.push_back(TYPE_UINT64);
                uint64_t v = std::get<uint64_t>(value);
                uint64_t net_v = htonll(v);
                append_bytes(payload, net_v);
            }
            else if (std::holds_alternative<float64_t>(value)) {
                payload.push_back(TYPE_FLOAT64);
                float64_t v = std::get<float64_t>(value);
                uint64_t  net_v;
                std::memcpy(&net_v, &v, sizeof(v));
                net_v = htonll(net_v);
                append_bytes(payload, net_v);
            }
            else if (std::holds_alternative<std::string>(value)) {
                payload.push_back(TYPE_STRING);
                const std::string& s = std::get<std::string>(value);
                uint32_t str_len = static_cast<uint32_t>(s.size());
                uint32_t net_str_len = htonl(str_len);
                append_bytes(payload, net_str_len);
                payload.insert(payload.end(), s.begin(), s.end());
            }
            else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                payload.push_back(TYPE_BINARY);
                const std::vector<uint8_t>& bin = std::get<std::vector<uint8_t>>(value);
                uint32_t bin_len = static_cast<uint32_t>(bin.size());
                uint32_t net_bin_len = htonl(bin_len);
                append_bytes(payload, net_bin_len);
                payload.insert(payload.end(), bin.begin(), bin.end());
            }
        }
        uint32_t payload_len = static_cast<uint32_t>(payload.size());
        uint32_t net_payload_len = htonl(payload_len);
        std::vector<uint8_t> message;
        append_bytes(message, net_payload_len);
        append_vector(message, payload);
        return message;
    }

    /******************************************************************************
     * @brief   SBDP プロトコルに基づくメッセージのデコード
     * @arg     msg   (in) エンコードされたSBDPメッセージ
     * @return  デコード結果
     * @note
     *****************************************************************************/
    inline Message decode_message(const std::vector<uint8_t>& message) {
        Message msg;
        size_t offset = 0;
        if (message.size() < 4)
            throw std::runtime_error("Message too short");
        uint32_t net_payload_len;
        std::memcpy(&net_payload_len, message.data(), sizeof(net_payload_len));
        offset += sizeof(net_payload_len);
        uint32_t payload_len = ntohl(net_payload_len);
        if (message.size() < 4 + payload_len)
            throw std::runtime_error("Incomplete message");
        while (offset < 4 + payload_len) {
            uint16_t net_key_len;
            if (offset + sizeof(net_key_len) > message.size())
                throw std::runtime_error("Key length read error");
            std::memcpy(&net_key_len, message.data() + offset, sizeof(net_key_len));
            offset += sizeof(net_key_len);
            uint16_t key_len = ntohs(net_key_len);
            if (offset + key_len > message.size())
                throw std::runtime_error("Key string region insufficient");
            std::string key(reinterpret_cast<const char*>(message.data() + offset), key_len);
            offset += key_len;
            if (offset >= message.size())
                throw std::runtime_error("Type code read error");
            uint8_t type_code = message[offset++];
            if (type_code == TYPE_INT64) {
                if (offset + 8 > message.size())
                    throw std::runtime_error("int64 read error");
                int64_t net_v;
                std::memcpy(&net_v, message.data() + offset, 8);
                offset += 8;
                int64_t v = static_cast<int64_t>(ntohll(static_cast<uint64_t>(net_v)));
                msg[key] = v;
            }
            else if (type_code == TYPE_UINT64) {
                if (offset + 8 > message.size())
                    throw std::runtime_error("uint64 read error");
                uint64_t net_v;
                std::memcpy(&net_v, message.data() + offset, 8);
                offset += 8;
                uint64_t v = ntohll(net_v);
                msg[key] = v;
            }
            else if (type_code == TYPE_FLOAT64) {
                if (offset + 8 > message.size())
                    throw std::runtime_error("float64_t read error");
                uint64_t net_v;
                std::memcpy(&net_v, message.data() + offset, 8);
                offset += 8;
                net_v = ntohll(net_v);
                float64_t v;
                std::memcpy(&v, &net_v, sizeof(v));
                msg[key] = v;
            }
            else if (type_code == TYPE_STRING) {
                if (offset + 4 > message.size())
                    throw std::runtime_error("String length read error");
                uint32_t net_str_len;
                std::memcpy(&net_str_len, message.data() + offset, 4);
                offset += 4;
                uint32_t str_len = ntohl(net_str_len);
                if (offset + str_len > message.size())
                    throw std::runtime_error("String data insufficient");
                std::string s(reinterpret_cast<const char*>(message.data() + offset), str_len);
                offset += str_len;
                msg[key] = s;
            }
            else if (type_code == TYPE_BINARY) {
                if (offset + 4 > message.size())
                    throw std::runtime_error("Binary length read error");
                uint32_t net_bin_len;
                std::memcpy(&net_bin_len, message.data() + offset, 4);
                offset += 4;
                uint32_t bin_len = ntohl(net_bin_len);
                if (offset + bin_len > message.size())
                    throw std::runtime_error("Binary data insufficient");
                std::vector<uint8_t> bin(message.begin() + offset, message.begin() + offset + bin_len);
                offset += bin_len;
                msg[key] = bin;
            }
            else {
                throw std::runtime_error("Unknown type code");
            }
        }
        return msg;
    }

} // namespace sbdp
