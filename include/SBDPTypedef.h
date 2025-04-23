// SPDX-License-Identifier: LicenseRef-SBPD-1.0
/******************************************************************************
 * @file    SBDPTypedef.h
 * @brief   SimpleBinaryDictionaryProtocol Type Define
 * @author  Satoh
 * @note    
 * Copyright (c) 2024 Satoh(3103lab.com)
 *****************************************************************************/
#pragma once

#include <string>
#include <map>
#include <variant>
#include <cstdint>
#include <stdexcept>

namespace sbdp {

    // 型コードの定義
    enum ValueType : uint8_t {
        TYPE_INT64  = 1,
        TYPE_UINT64 = 2,
        TYPE_FLOAT  = 3,
        TYPE_STRING = 4,
        TYPE_BINARY = 5
    };

    using SimpleValue = std::variant<int64_t, uint64_t, float, std::string, std::vector<uint8_t>>;
    using Message = std::map<std::string, SimpleValue>;
} // namespace sbdp

