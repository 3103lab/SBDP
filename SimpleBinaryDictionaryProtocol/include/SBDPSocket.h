// SPDX-License-Identifier: LicenseRef-SBPD-1.0
/******************************************************************************
 * @file    SBDPSocket.h
 * @brief   SimpleBinaryDictionaryProtocol Socket Class
 * @author  Satoh
 * @note    
 * Copyright (c) 2024 Satoh(3103lab.com)
 *****************************************************************************/
#pragma once

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int SOCKET;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
#endif

#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace sbdp {

    // エンディアン変換関数（Windows/Linux共通）
    inline uint64_t htonll(uint64_t value) {
        return (((uint64_t)htonl(static_cast<uint32_t>(value & 0xFFFFFFFFULL))) << 32)
               | htonl(static_cast<uint32_t>(value >> 32));
    }
    inline uint64_t ntohll(uint64_t value) {
        return (((uint64_t)ntohl(static_cast<uint32_t>(value & 0xFFFFFFFFULL))) << 32)
               | ntohl(static_cast<uint32_t>(value >> 32));
    }

    /******************************************************************************
     * @brief   ソケット初期化
     * @arg     なし
     * @return  結果 true:正常 false:異常
     * @note    本処理を実行後、終了時にはcleanup_socketsを実行してください
     *****************************************************************************/
    inline bool init_sockets() {
    #ifdef _WIN32
        WSADATA wsaData;
        return (WSAStartup(MAKEWORD(2,2), &wsaData) == 0);
    #else
        return true;
    #endif
    }

    /******************************************************************************
     * @brief   ソケットクリーンアップ
     * @arg     なし
     * @return  なし
     * @note
     *****************************************************************************/
    inline void cleanup_sockets() {
    #ifdef _WIN32
        WSACleanup();
    #endif
    }

    /******************************************************************************
     * @brief   バッファ内の全データを送信（ブロッキング）
     * @arg     cSocket (in) 送信に使用するソケット
     * @arg     data    (in) 送信するデータ
     * @arg     len     (in) 送信するデータ長
     * @return  結果 true:正常 false:異常
     * @note    
     *****************************************************************************/
    inline bool send_all(SOCKET sock, const uint8_t* data, size_t len) {
        size_t total_sent = 0;
        while (total_sent < len) {
            int sent = send(sock, reinterpret_cast<const char*>(data + total_sent),
                            static_cast<int>(len - total_sent), 0);
            if (sent == SOCKET_ERROR || sent <= 0)
                return false;
            total_sent += sent;
        }
        return true;
    }

    /******************************************************************************
     * @brief   指定バイト数を受信（ブロッキング）
     * @arg     cSocket (in)  受信に使用するソケット
     * @arg     data    (out) 受信するデータ格納バッファ
     * @arg     len     (in)  受信するデータ長
     * @return  結果 true:正常 false:異常
     * @note    
     *****************************************************************************/
    inline bool recv_all(SOCKET sock, uint8_t* buffer, size_t len) {
        size_t total_recv = 0;
        while (total_recv < len) {
            int recvd = recv(sock, reinterpret_cast<char*>(buffer + total_recv),
                             static_cast<int>(len - total_recv), 0);
            if (recvd <= 0)
                return false;
            total_recv += recvd;
        }
        return true;
    }

    /******************************************************************************
     * @brief   タイムアウト付き指定バイト数を受信 （ミリ秒単位）
     * @arg     cSocket     (in)  受信に使用するソケット
     * @arg     data        (out) 受信するデータ格納バッファ
     * @arg     len         (in)  受信するデータ長
     * @arg     unTimeoutMs (in)  タイムアウト(ミリ秒)
     * @return  結果 true:正常 false:異常
     * @note    
     *****************************************************************************/
    inline bool recv_all_with_timeout(SOCKET sock, uint8_t* buffer, size_t len, uint64_t unTimeoutMs) {
        size_t total_recv = 0;
        while (total_recv < len) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock, &read_fds);

            struct timeval tv {};
            tv.tv_sec = static_cast<long>(unTimeoutMs / 1000);
            tv.tv_usec = (unTimeoutMs % 1000) * 1000;

            int ret = select( static_cast<int>(sock) + 1, &read_fds, nullptr, nullptr, &tv);
            if (ret <= 0) {
                return false; // timeout or error
            }

            int recvd = recv(sock, reinterpret_cast<char*>(buffer + total_recv),
                static_cast<int>(len - total_recv), 0);
            if (recvd <= 0)
                return false;
            total_recv += recvd;
        }
        return true;
    }

    // Socket クラス（コピー禁止、ムーブ可能）
    class Socket {
    public:
        // コンストラクタ・デストラクタ
        Socket() : sock(INVALID_SOCKET) { }
        ~Socket() { close(); }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other) noexcept : sock(other.sock) {
            other.sock = INVALID_SOCKET;
        }
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();
                sock = other.sock;
                other.sock = INVALID_SOCKET;
            }
            return *this;
        }

        /******************************************************************************
         * @brief   ソケット作成（AF_INET, SOCK_STREAM）
         * @arg     なし
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool create() {
            sock = ::socket(AF_INET, SOCK_STREAM, 0);
            return (sock != INVALID_SOCKET);
        }

        /******************************************************************************
         * @brief   指定ポートでバインドする（サーバー用）
         * @arg     なし
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool bind(unsigned short port) {
            sockaddr_in addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);
            return (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR);
        }

        /******************************************************************************
         * @brief   待ち受け開始（サーバー用）
         * @arg     なし
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool listen(int backlog = SOMAXCONN) {
            return (::listen(sock, backlog) != SOCKET_ERROR);
        }

        /******************************************************************************
         * @brief   接続要求を受け入れて新しい Socket を返す（サーバー用）
         * @arg     なし
         * @return  クライアントソケット
         * @note    
         *****************************************************************************/
        Socket accept() {
            Socket newSock;
            sockaddr_in clientAddr;
        #ifdef _WIN32
            int addrLen = sizeof(clientAddr);
        #else
            socklen_t addrLen = sizeof(clientAddr);
        #endif
            newSock.sock = ::accept(sock, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
            if (newSock.sock == INVALID_SOCKET)
                throw std::runtime_error("Accept failed");
            return newSock;
        }

        /******************************************************************************
         * @brief   指定ホスト、ポートに接続（クライアント用）
         * @arg     host     (in)  接続先ホスト
         * @arg     port     (in)  接続先ポート
         * @return  クライアントソケット
         * @note    
         *****************************************************************************/
        bool connect(const std::string& host, unsigned short port) {
            addrinfo hints{};
            hints.ai_family = AF_INET;        // IPv4
            hints.ai_socktype = SOCK_STREAM;  // TCP

            addrinfo* result = nullptr;
            std::string portStr = std::to_string(port);

            if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
                return false;
            }

            bool connected = false;
            for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
                if (::connect(sock, rp->ai_addr, static_cast<int>(rp->ai_addrlen)) != SOCKET_ERROR) {
                    connected = true;
                    break;
                }
            }

            freeaddrinfo(result);
            return connected;
        }

        /******************************************************************************
         * @brief   内部の SOCKET ハンドルの取得
         * @arg     なし
         * @return  SOCKET ハンドル
         * @note    
         *****************************************************************************/
        SOCKET get_handle() const { return sock; }

        /******************************************************************************
         * @brief   バッファ全体の送信
         * @arg     data    (out) 送信するデータ格納バッファ
         * @arg     len     (in)  送信するデータ長
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool send_all(const uint8_t* data, size_t len) {
            return sbdp::send_all(sock, data, len);
        }

        /******************************************************************************
         * @brief   タイムアウト付き指定バイト数を受信 （ミリ秒単位）
         * @arg     data        (out) 受信するデータ格納バッファ
         * @arg     len         (in)  受信するデータ長
         * @arg     unTimeoutMs (in)  タイムアウト(ミリ秒)
         * @return  結果 true:正常 false:異常
         * @note    unTimeoutMsを0にした場合、タイムアウトなしになります
         *****************************************************************************/
        bool recv_all(uint8_t* buffer, size_t len, uint64_t unTimeoutMs) {
            if (unTimeoutMs == 0) {
                return sbdp::recv_all(sock, buffer, len);
            }
            return sbdp::recv_all_with_timeout(sock, buffer, len, unTimeoutMs);
        }

        /******************************************************************************
         * @brief   ソケットのクローズ
         * @arg     なし
         * @return  なし
         * @note    
         *****************************************************************************/
        void close() {
            if (sock != INVALID_SOCKET) {
            #ifdef _WIN32
                ::closesocket(sock);
            #else
                ::close(sock);
            #endif
                sock = INVALID_SOCKET;
            }
        }

    private:
        SOCKET sock;
    };

    /******************************************************************************
     * @brief   接続先アドレス文字列の取得
     * @arg     socket (in) ソケット
     * @return  接続先アドレス文字列
     * @note    
     *****************************************************************************/
    inline std::string get_peer_address(const Socket& socket) {
        sockaddr_in addr;
        #ifdef _WIN32
            int addrLen = sizeof(addr);
        #else
            socklen_t addrLen = sizeof(addr);
        #endif
        if (getpeername(socket.get_handle(), reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0)
            return "[error retrieving address]";

        char host[NI_MAXHOST];
        // getnameinfo() によりIPアドレスを数値文字列として取得
        if (getnameinfo(reinterpret_cast<sockaddr*>(&addr), addrLen,
            host, sizeof(host), nullptr, 0, NI_NUMERICHOST) != 0)
            return "[unknown]";
        return std::string(host);
    }

} // namespace sbdp
