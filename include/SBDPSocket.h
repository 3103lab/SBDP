// SPDX-License-Identifier: LicenseRef-SBPD-1.0
/******************************************************************************
 * @file    SBDPSocket.h
 * @brief   SimpleBinaryDictionaryProtocol Socket Class
 * @author  Satoh
 * @note    
 * Copyright (c) 2024 Satoh(3103lab.com)
 *****************************************************************************/
#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <cerrno>

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
#include "SBDP.h"

namespace sbdp {

    /******************************************************************************
     * @brief   ソケット初期化
     * @arg     なし
     * @return  結果 true:正常 false:異常
     * @note    本処理を実行後、終了時にはCleanupSocketsを実行してください
     *****************************************************************************/
    inline bool InitSockets() {
    #ifdef _WIN32
        WSADATA stWsaData;
        return (WSAStartup(MAKEWORD(2,2), &stWsaData) == 0);
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
    inline void CleanupSockets() {
    #ifdef _WIN32
        WSACleanup();
    #endif
    }

    // Socket クラス（コピー禁止、ムーブ可能）
    class Socket {
    public:
        // コンストラクタ・デストラクタ
        Socket() : m_hSocket(INVALID_SOCKET), m_bShutdown(false){ }
        ~Socket() { Close(); }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other) noexcept : m_hSocket(other.m_hSocket) {
            other.m_hSocket = INVALID_SOCKET;
        }
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                Close();
                m_hSocket = other.m_hSocket;
                other.m_hSocket = INVALID_SOCKET;
            }
            return *this;
        }

        /******************************************************************************
         * @brief   ソケット作成（AF_INET, SOCK_STREAM）
         * @arg     なし
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool Create() {
            m_hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
            return (m_hSocket != INVALID_SOCKET);
        }

        /******************************************************************************
         * @brief   指定ポートでバインドする（サーバー用）
         * @arg     なし
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool Bind(unsigned short unPort) {
            sockaddr_in stAddress;
            std::memset(&stAddress, 0, sizeof(stAddress));
            stAddress.sin_family = AF_INET;
            stAddress.sin_addr.s_addr = INADDR_ANY;
            stAddress.sin_port = htons(unPort);
            return (::bind(m_hSocket, reinterpret_cast<sockaddr*>(&stAddress), sizeof(stAddress)) != SOCKET_ERROR);
        }

        /******************************************************************************
         * @brief   待ち受け開始（サーバー用）
         * @arg     なし
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool Listen(int snBacklog = SOMAXCONN) {
            return (::listen(m_hSocket, snBacklog) != SOCKET_ERROR);
        }

        /******************************************************************************
         * @brief   接続要求を受け入れて新しい Socket を返す（サーバー用）
         * @arg     なし
         * @return  クライアントソケット
         * @note    
         *****************************************************************************/
        Socket Accept() {
            Socket cClientSocket;
            sockaddr_in stClientAddress;
        #ifdef _WIN32
            int snAddressLength = sizeof(stClientAddress);
        #else
            socklen_t snAddressLength = sizeof(stClientAddress);
        #endif
            cClientSocket.m_hSocket = ::accept(m_hSocket,
                                               reinterpret_cast<sockaddr*>(&stClientAddress),
                                               &snAddressLength);
            if (cClientSocket.m_hSocket == INVALID_SOCKET) {
                if (m_bShutdown.load()) {
                    throw std::system_error(static_cast<int>(std::errc::operation_canceled), std::generic_category(), "socket shutdown");
                }
                throw std::runtime_error("Accept failed");
            }
            return cClientSocket;
        }

        /******************************************************************************
         * @brief   指定ホスト、ポートに接続（クライアント用）
         * @arg     host     (in)  接続先ホスト
         * @arg     port     (in)  接続先ポート
         * @return  クライアントソケット
         * @note    
         *****************************************************************************/
        bool Connect(const std::string& strHost, unsigned short unPort) {
            addrinfo stHints {};
            stHints.ai_family = AF_INET;        // IPv4
            stHints.ai_socktype = SOCK_STREAM;  // TCP

            addrinfo* pstResult = nullptr;
            std::string strPort = std::to_string(unPort);

            if (getaddrinfo(strHost.c_str(), strPort.c_str(), &stHints, &pstResult) != 0) {
                return false;
            }

            bool bConnected = false;
            for (addrinfo* pstCurrent = pstResult; pstCurrent != nullptr; pstCurrent = pstCurrent->ai_next) {
                if (::connect(m_hSocket, pstCurrent->ai_addr,
                              static_cast<int>(pstCurrent->ai_addrlen)) != SOCKET_ERROR) {
                    bConnected = true;
                    break;
                }
            }

            freeaddrinfo(pstResult);
            return bConnected;
        }

        /******************************************************************************
         * @brief   接続先アドレス文字列の取得
         * @arg     なし
         * @return  接続先アドレス文字列
         * @note    
         *****************************************************************************/
        std::string GetPeerAddress() const {
            sockaddr_in stAddress;
        #ifdef _WIN32
            int snAddressLength = sizeof(stAddress);
        #else
            socklen_t snAddressLength = sizeof(stAddress);
        #endif
            if (getpeername(m_hSocket,
                            reinterpret_cast<sockaddr*>(&stAddress),
                            &snAddressLength) != 0) {
                return "[error retrieving address]";
            }

            char szHost[NI_MAXHOST];
            // getnameinfo() によりIPアドレスを数値文字列として取得
            if (getnameinfo(reinterpret_cast<sockaddr*>(&stAddress), snAddressLength,
                            szHost, sizeof(szHost), nullptr, 0, NI_NUMERICHOST) != 0) {
                return "[unknown]";
            }
            return std::string(szHost);
        }

        /******************************************************************************
         * @brief   バッファ全体の送信
         * @arg     data    (out) 送信するデータ格納バッファ
         * @arg     len     (in)  送信するデータ長
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool SendAll(const uint8_t* pData, size_t unLength) {
            return bSendAll(pData, unLength);
        }

        /******************************************************************************
         * @brief   タイムアウト付き指定バイト数を受信 （ミリ秒単位）
         * @arg     data        (out) 受信するデータ格納バッファ
         * @arg     len         (in)  受信するデータ長
         * @arg     unTimeoutMs (in)  タイムアウト(ミリ秒)
         * @return  結果 true:正常 false:異常
         * @note    unTimeoutMsを0にした場合、タイムアウトなしになります
         *****************************************************************************/
        bool RecvAll(uint8_t* pBuffer, size_t unLength, uint64_t unTimeoutMs) {
            if (unTimeoutMs == 0) {
                return bRecvAll(pBuffer, unLength);
            }
            return bRecvAllWithTimeout(pBuffer, unLength, unTimeoutMs);
        }

        /******************************************************************************
         * @brief   SBDP プロトコルメッセージ送信
         * @arg     msg     (in) 送信するメッセージ
         * @return  送信結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool SendMessage(const Message& msgData) {
            std::vector<uint8_t> vecData = EncodeMessage(msgData);
            return SendAll(vecData.data(), vecData.size());
        }

        /******************************************************************************
         * @brief   SBDP プロトコルメッセージ受信
         * @arg     unTimeoutMs (in) タイムアウト(ミリ秒)
         * @return  受信メッセージ
         * @note    
         *****************************************************************************/
        Message RecvMessage(uint64_t unTimeoutMs = 0) {
            uint8_t unHeader[k_unHeaderSize];
            if (!RecvAll(unHeader, k_unHeaderSize, unTimeoutMs)) {
                throw std::runtime_error("Header reception failed");
            }
            uint32_t unNetPayloadLength;
            std::memcpy(&unNetPayloadLength, unHeader, k_unHeaderSize);
            uint32_t unPayloadLength = ntohl(unNetPayloadLength);
            std::vector<uint8_t> vecBuffer(k_unHeaderSize + unPayloadLength);
            std::memcpy(vecBuffer.data(), unHeader, k_unHeaderSize);
            if (!RecvAll(vecBuffer.data() + k_unHeaderSize, unPayloadLength, unTimeoutMs)) {
                throw std::runtime_error("Payload reception failed");
            }
            return DecodeMessage(vecBuffer);
        }

        /******************************************************************************
         * @brief   ソケットのクローズ
         * @arg     なし
         * @return  なし
         * @note    
         *****************************************************************************/
        void Close() {
            if (m_hSocket != INVALID_SOCKET) {
            #ifdef _WIN32
                ::closesocket(m_hSocket);
            #else
                ::close(m_hSocket);
            #endif
                m_hSocket = INVALID_SOCKET;
            }
        }

        /******************************************************************************
         * @brief   ソケットのシャットダウン
         * @arg     なし
         * @return  なし
         * @note
         *****************************************************************************/
        void Shutdown() {
            if (m_hSocket != INVALID_SOCKET) {
#ifdef _WIN32
                ::shutdown(m_hSocket, SD_BOTH);
#else
                ::shutdown(m_hSocket, SHUT_RDWR);
#endif
				m_bShutdown.store(true);
            }
        }

    private:
        /******************************************************************************
         * @brief   ソケットエラーを例外として送出
         * @arg     pszApiName (in) API名
         * @return  なし
         * @note    
         *****************************************************************************/
        static void vThrowSocketError(const char* pszApiName) {
#ifdef _WIN32
            throw std::system_error(WSAGetLastError(), std::system_category(), pszApiName);
#else
            throw std::system_error(errno, std::system_category(), pszApiName);
#endif
        }

        /******************************************************************************
         * @brief   バッファ内の全データを送信（ブロッキング）
         * @arg     pData    (in) 送信するデータ
         * @arg     unLength (in) 送信するデータ長
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool bSendAll(const uint8_t* pData, size_t unLength) {
            size_t unTotalSent = 0;
            while (unTotalSent < unLength) {
                if (m_bShutdown.load()) {
                    throw std::system_error(static_cast<int>(std::errc::operation_canceled), std::generic_category(), "socket shutdown");
                }

                int snSent = send(m_hSocket, reinterpret_cast<const char*>(pData + unTotalSent),
                                  static_cast<int>(unLength - unTotalSent), 0);
                if (snSent == SOCKET_ERROR || snSent <= 0) {
                    vThrowSocketError("send");
                }
                unTotalSent += static_cast<size_t>(snSent);
            }
            return true;
        }

        /******************************************************************************
         * @brief   指定バイト数を受信（ブロッキング）
         * @arg     hSocket  (in)  受信に使用するソケット
         * @arg     pBuffer  (out) 受信するデータ格納バッファ
         * @arg     unLength (in)  受信するデータ長
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool bRecvAll(uint8_t* pBuffer, size_t unLength) {
            size_t unTotalReceived = 0;
            while (unTotalReceived < unLength) {
                if (m_bShutdown.load()) {
                    throw std::system_error(static_cast<int>(std::errc::operation_canceled), std::generic_category(), "socket shutdown");
                }

                int snReceived = recv(m_hSocket, reinterpret_cast<char*>(pBuffer + unTotalReceived),
                                      static_cast<int>(unLength - unTotalReceived), 0);
                if (snReceived <= 0) {
                    vThrowSocketError("recv");
                }
                unTotalReceived += static_cast<size_t>(snReceived);
            }
            return true;
        }

        /******************************************************************************
         * @brief   タイムアウト付き指定バイト数を受信 （ミリ秒単位）
         * @arg     pBuffer     (out) 受信するデータ格納バッファ
         * @arg     unLength    (in)  受信するデータ長
         * @arg     unTimeoutMs (in)  タイムアウト(ミリ秒)
         * @return  結果 true:正常 false:異常
         * @note    
         *****************************************************************************/
        bool bRecvAllWithTimeout(uint8_t* pBuffer, size_t unLength, uint64_t unTimeoutMs) {
            size_t unTotalReceived = 0;
            while (unTotalReceived < unLength) {
                fd_set stReadFds;
                FD_ZERO(&stReadFds);
                FD_SET(m_hSocket, &stReadFds);

                struct timeval stTimeout {};
                stTimeout.tv_sec = static_cast<long>(  unTimeoutMs / 1000);
                stTimeout.tv_usec = static_cast<long>((unTimeoutMs % 1000) * 1000);

                int snSelectResult = select(static_cast<int>(m_hSocket) + 1,
                                            &stReadFds, nullptr, nullptr, &stTimeout);
                if (snSelectResult < 0) {
                    vThrowSocketError("select");
                }
                if (snSelectResult == 0) {
                    throw std::system_error( static_cast<int>(std::errc::timed_out), std::generic_category(), "select timeout");
                }
                if(m_bShutdown.load()) {
                    throw std::system_error( static_cast<int>(std::errc::operation_canceled), std::generic_category(), "socket shutdown");
				}
                int snReceived = recv(m_hSocket, reinterpret_cast<char*>(pBuffer + unTotalReceived),
                                      static_cast<int>(unLength - unTotalReceived), 0);
                if (snReceived <= 0) {
                    vThrowSocketError("recv");
                }
                unTotalReceived += static_cast<size_t>(snReceived);
            }
            return true;
        }
        
    private:
        SOCKET           m_hSocket;
        std::atomic_bool m_bShutdown;
    };

    /******************************************************************************
     * @brief   接続先アドレス文字列の取得
     * @arg     socket (in) ソケット
     * @return  接続先アドレス文字列
     * @note    
     *****************************************************************************/
    inline std::string GetPeerAddress(const Socket& cSocket) {
        return cSocket.GetPeerAddress();
    }

    /******************************************************************************
     * @brief   SBDP プロトコルメッセージ送信
     * @arg     cSocket (in) 送信に使用するソケット
     * @arg     msg     (in) 送信するメッセージ
     * @return  送信結果 true:正常 false:異常
     * @note
     *****************************************************************************/
    inline bool SendMessage(Socket& cSocket, const Message& msgData) {
        return cSocket.SendMessage(msgData);
    }

    /******************************************************************************
     * @brief   SBDP プロトコルメッセージ受信
     * @arg     cSocket     (in) 受信に使用するソケット
     * @arg     unTimeoutMs (in) タイムアウト(ミリ秒)
     * @return  受信メッセージ
     * @note
     *****************************************************************************/
    inline Message RecvMessage(Socket& cSocket, uint64_t unTimeoutMs = 0) {
        return cSocket.RecvMessage(unTimeoutMs);
    }
} // namespace sbdp
