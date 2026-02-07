# C++ コーディング規約（v0.1）

**目的**
>本ドキュメントはプロジェクト全体で一貫した可読性・保守性を確保するためのガイドラインを提供します。
---

## 0. Coding Philosophy

1. **Readability First** – 可読性を最優先し、マイクロ最適化は計測結果を得てから行う。
2. **KISS (Keep It Short and Simple)** – 仕様を満たす最小限の実装を心がけ、過度な抽象化を避ける。
3. **Don’t Repeat Yourself (DRY)** – 重複コードとロジックを排除して保守コストを下げる。
4. **Bounded Context** – 1 関数 = 1 責務、1 クラス = 1 ドメイン。
5. **Fail Fast** – 契約違反は即クラッシュし原因を明示する。
6. **Observable System** – ログとメトリクスで動作を可視化し、障害解析を容易に。
7. **RAII & Ownership** – リソースは必ず RAII で管理し、生ポインタは原則禁止。
8. **Measured Performance** – 速度要件はベンチとプロファイルで検証し、必要箇所のみ最適化。
9. **YAGNI (You Aren’t Gonna Need It)** – 未来の機能を先回りして実装せず、現時点で必要なものだけを作る。

> これらの原則は“判断が分かれたときの優先順位”として機能します。個別ルールよりも上位に位置付け、迷ったら本方針に立ち返ってください。

---
## 1. ファイル構成

- **拡張子**
  - ヘッダー：`.h` / `.hpp`
  - 実装：`.cpp`
- **インクルードガード**：必ず `#pragma once` を使用
- **ファイルヘッダ**：80文字以内で以下タグを含む
```cpp
/******************************************************************************
 * @file    LockedQueue.h
 * @brief   Queue with Lock
 *
 *          Light C++ Common Library
 *
 * @author  Hikari Satoh
 * @note    
 *  
 * Copyright (c) 2025 Hikari Satoh
 *               https://3103lab.com
 ******************************************************************************
 */
  ```

---

## 2. インクルード順

1. 自クラスのヘッダー
2. 標準ライブラリ
3. サードパーティライブラリ
4. プロジェクト内ヘッダー

```cpp
#include "MyClass.h"
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "OtherModule.h"
```

---

## 3. 命名規則

### 3.1 スコープ別プレフィックス

| スコープ     | 例                   | 備考                                 |
| ------------ | -------------------- | ------------------------------------ |
| ローカル     | `unCount`, `unindex` | ** 型プレフィックス**                |
| メンバー     | `m_unCount`          | `m_` + 型プレフィックス + PascalCase |
| 静的メンバー | `s_unMaxRetries`     | `s_` + 型プレフィックス + PascalCase |
| グローバル   | `g_bShutdown`        | `g_` + 型プレフィックス + PascalCase |
| 定数         | `k_unMaxSize`        | `k_` + 型プレフィックス + PascalCase |

### 3.2 型別プレフィックス（ハンガリアン小文字）

| 型                | プレフィックス | 例                          |
| ----------------- | -------------- | --------------------------- |
| `std::string`     | `str`          | `strName`                   |
| `char*`           | `psz`          | `pszBuffer`                 |
| `bool`            | `b`            | `bEnabled`                  |
| 符号付整数        | `sn`           | `snOffset`                  |
| 符号無し整数      | `un`           | `unIndex`                   |
| 32-bit 浮動小数   | `fp`           | `fpRatio`                   |
| 64-bit 浮動小数   | `db`           | `dbVoltage`                 |
| `std::unique_ptr` | `up`           | `upSession`                 |
| `std::shared_ptr` | `sp`           | `spNode`                    |
| `std::weak_ptr`   | `wp`           | `wpObserver`                |
| **生ポインタ**    | **使用禁止**   | 必要なら `pRawXxx`          |

#### 例

```cpp
std::uint32_t unCount;         // 符号無し32ビット整数
std::int64_t  snTotalTime;     // 符号有り64ビット整数
bool          bIsReady;        // 真偽値
std::string   strName;         // 文字列クラス
const std::size_t kMaxSize = 100; // 定数
void (*fnCallback)();          // 関数ポインタ
HANDLE        hFile;           // ハンドル
char          szBuffer[256];   // ゼロ終端文字列配列
const char*   pszText;         // ゼロ終端文字列ポインタ
```

### 3.3 関数／メソッド名

- **命名規則**: 動詞＋目的語 の形式を基本とし、処理内容を明確に表す。

- **Public メソッド**: CamelCase

  ```cpp
  FindUser(), ProcessData(), Initialize(), Shutdown()
  ```

- **Protected/Private メソッド**: 戻り値型プレフィックス + CamelCase

  ```cpp
  pcFindUser(), unGetCount(), bIsReady(), fnHandleCallback(), hOpenFile()
  ```

- "And" の使用制限

  - **原則**: 関数名は動詞＋目的語１つ。
  - **やむを得ず "And" を使う場合**:
    1. 最大2つまでに制限する。例: `DoFooAndBar()`
    2. 関数名は動詞 (Do/Set) で始める。
    3. 関数コメントで「この関数は X と Y を同時に実行する」旨を明記する。
  - **3つ以上の組み合わせが必要な場合**:
    1. 小さな関数に分割する。
    2. 呼び出し元で順次呼び出す。
    3. 必要に応じてユーティリティ関数としてラップする。

### 3.4 テンプレート

- 型パラメータに末尾 `_` を付与

```cpp
template<class T_>
```

### 3.5 定数

- 定数は `constexpr` を原則とする。`const` は後方互換や一時的定義のみ。
- 定数プレフィックスは `k_` を使用。
  ```cpp
  constexpr std::size_t kMaxSize = 100;
  constexpr float32_t    kRate    = 0.1f;
  ```

## 4. コードスタイル

### 4.1 インデント・空白

| 項目          | 規約                              |
| ------------- | --------------------------------- |
| インデント幅  | 4 スペース（タブ禁止）            |
| 行長          | **80 桁** 以内を目安              |
| 空行          | セクション間 1 行、過剰な空行禁止 |

### 4.2 ブレース

- **K&R スタイル**（オープニングブレースは行末）
- 単行 `if` でもブレース必須。

```cpp
if (bReady) {
    Process();
}
```

---

### 4.3 危険関数の禁止（C 文字列操作）

| 区分                   | 使用禁止関数                         | 許可・推奨関数（例）                                      |
| ---------------------- | ------------------------------------ | --------------------------------------------------------- |
| **コピー／結合・比較** | `strcpy`, `strcat`, `strcmp`         | `strncpy`, `strncat`, `strncmp`, `std::string` 系メソッド |
| **フォーマット出力**   | `sprintf`, `vsprintf`                | `snprintf`, `vsnprintf`, `fmt::format`                    |
| **入力**               | `gets`                               | `fgets`, `std::getline`                                   |
| **その他**             | `strtok`（状態保持で非スレッド安全） | `strtok_r`, `boost::split`, `std::string_view` 操作       |

---

### 4.4 未初期化変数の禁止

- **ローカル変数は宣言と同時に必ず初期化**すること。例外はループ変数などごく一部に限る。
- **メンバー変数はコンストラクタ初期化リスト**で初期化し、クラス内での未初期化を許容しない。
- CI のコンパイルオプションで `-Wuninitialized` / `-Wmaybe-uninitialized` を `-Werror` に含め、警告をビルド失敗とする。
- “値なし” 状態が必要な場合は `std::optional` や `std::variant` などを使用し、**未定義動作を避ける**。

### 4.5 関数設計指針

- 実装部は **60 行以内**、**30 ステートメント以内** を目安とする。
- 入れ子ブロックは **3 段まで**。4 段以上になる場合は関数抽出・ガード節で平坦化する。
- 循環的複雑度 (Cyclomatic Complexity) は **10 以下**。CI で計測し、超過時はリファクタリングを行う。
- **単一責任の原則 (SRP)** に従い、1 関数 = 1 タスクとする。異なるドメイン操作を混在させない。
- **副作用を伴う関数** は `UpdateXxx` / `ResetXxx` など *動詞＋目的語*、**純粋関数** は `CalcXxx`, `ToXxx`, `GetXxx` などで命名を区別する。
- **早期 return** を用いてエラーパス・特殊ケースを先に処理し、主要ロジックをインデント 1 段で保つ。
- 例外を投げる関数はコメントに **例外型** と **例外安全保証レベル**（強保証／基本保証）を明記する。

---

## 5. 関数ヘッダコメント

- **Doxygen風ブロックコメント** を基本にタグ付け
- 項目は省略せず、void型の場合などは「なし」と明記する。
  ```cpp
  /******************************************************************************
   * @brief   デキュー（シャットダウン・タイムアウト対応）
   * @param   pcData    (out) デキューしたデータ
   * @param   unTimeout (in)  タイムアウト時間（ms）
   * @return  結果
   * @retval  true=正常, false=タイムアウト/シャットダウン
   * @note    タイムアウト0で無限待ち
   ******************************************************************************/
  ```

---

## 6. クラス設計

- **コピー／ムーブ**：不要なら `= delete`
- **デストラクタ**：リソース解放や `Shutdown()` 呼び出しを明示
- **オーバーライドメソッド**：基底クラスの仮想関数を実装する際は、宣言末尾に `override` を必ず付与

---

## 7. スレッド安全

- 排他：`std::mutex` + `std::unique_lock`
- 待機：`std::condition_variable`
- フラグ：`std::atomic_bool`

---

## 8. エラーハンドリング

本プロジェクトでは、**エラーの性質と呼び出し側が取り得るアクション**に応じて 4 段階でエラーを伝達する。

| レイヤ                    | 想定原因                                     | 伝達方法                        | 型／戻り値例                                                     | ガイドライン                                                                                |
| ------------------------- | -------------------------------------------- | ------------------------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| **0: 契約違反**           | プリコンディション不備・プログラムバグ       | **例外** (`std::logic_error` 系)| `throw std::invalid_argument`                                    | 呼び出し側が必ず修正すべき。catch 不要・fail-fast 推奨                                      |
| **1: 外的要因 (NW/OS)**   | ネットワーク切断・タイムアウト・syscall 失敗 | **例外** (`std::system_error`)  | `throw std::system_error(errno, std::system_category(), "recv")` | ドライバ層で throw。**API 境界で捕捉し ResultCode に変換**し、スタック＋what() をログに残す |
| **2: 業務ロジック**       | ビジネスルール違反・ドメインエラー           | **結果コード enum**             | `enum class MsgSendResult { Ok, Timeout, Disconnected };`        | 成功 = `Ok(0)`。enum 値の追加は末尾に行い削除・並べ替え禁止                                 |
| **3: 軽微・高頻度判定**   | 継続可能な軽微エラー                         | **bool + std::error\_code**     | `bool Parse(..., std::error_code& ec)`                           | 旧 API 互換や tight loop など性能重視の場面のみ                                             |

> **注意**: `noexcept` 関数・スレッドエントリ・コールバックでは例外を外へ漏らさないこと。必ず `catch (...)` で捕捉し `ResultCode` 等に変換する。

### 8.1 ResultCode enum の命名規則

```cpp
enum class XxxResult : uint8_t {
    Ok = 0,
    RecoverableError1,
    RecoverableError2,
    FatalError,
};
```

- 名前は `XxxResult` もしくは `XxxStatus`。
- 0 は必ず `Ok`（成功）。
- 後方互換確保のため **追加は末尾**、削除・並び替え禁止。
- 戻り値が ResultCode の関数は \*\*Doxygen \*\*`` に各値の意味を明記。

### 8.2 例外捕捉サンプル（境界で変換）

```cpp
MsgSendResult Sender::Send(const Packet& pkt)
{
    try {
        driver_.Write(pkt);                // system_error が飛ぶ可能性
        return MsgSendResult::Ok;
    }
    catch (const std::system_error& se) {
        LOG_ERROR("Send failed: {}", se.what());
        return MsgSendResult::Disconnected;
    }
}
```

### 8.3 ファイル／ソケット操作の失敗検査

- **I/O API の戻り値・例外を必ず検査**する。`[[nodiscard]]` を付与し、呼び出し側が無視するとコンパイル警告となるようにする。
- POSIX 系 (`open`, `read`, `write`, `recv` など) の **戻り値 < 0** は直ちに `std::system_error`（`errno` 連携）へ変換して 6.1/6.2 の方針で処理。
- `std::filesystem` の操作は例外ベース設計となっているため、**try/catch でハンドリング**し、境界で `ResultCode` へマッピングする。
- ファイルディスクリプタやソケットは `unique_fd` / `unique_socket` など **RAII ラッパ**で管理し、`close` 忘れ・リークを防止。

---

## 9. その他ルール

- **行長**：80文字以内
- **依存関係**：必要最小限
- **名前空間**：プロジェクト単位で整理

---

## 10. プリミティブ型と浮動小数点型の使用制限

### 10.1 定幅整数型と浮動小数点

- **禁止**：可変サイズ型（`int`, `long`, `short`, `float`, `double`）
- **固定幅整数型** を使用
  ```cpp
  uint8_t, int8_t;
  uint16_t, int16_t;
  uint32_t, int32_t;
  uint64_t, int64_t;
  ```
- **浮動小数点型** は以下のエイリアス＋`static_assert`で保証
  ```cpp
  static_assert(sizeof(float)  == 4, "float must be 32-bit IEEE754");
  static_assert(sizeof(double) == 8, "double must be 64-bit IEEE754");
  using float32_t = float;    // 32-bit IEEE754
  using float64_t = double;   // 64-bit IEEE754
  ```

### 10.2 強い型エイリアス（Strong Typedef）

- **異なる意味の同一基礎型**（例: すべて `uint64_t` など）が混在しないよう、**型エイリアスで概念を分離**する。
  ```cpp
  using UserId  = uint64_t;
  using OrderId = uint64_t;
  using EpochMs = int64_t;
  ```
- 複複雑な型安全が求められる場合は、単一フィールド構造体などでラップし、暗黙変換を防ぐ。
  ```cpp
  struct Meter {
      double value;
  };
  ```
- 別ユニット間を跨ぐ演算や比較を行う場合は、明示的キャスト or ヘルパ関数を提供し、**型安全と可読性を両立**させる。
---

## 11. ポインタの使用

- **生ポインタ(raw pointer)**（`MyClass* pcObj`）は**原則禁止**。非所有参照が必要な場合は、参照または `std::weak_ptr` を検討。

- **スマートポインタ**による所有権管理を徹底:

  - `std::unique_ptr<T>` → `up` プレフィックス
    ```cpp
    std::unique_ptr<MyClass> upObj = std::make_unique<MyClass>(args);
    ```
  - `std::shared_ptr<T>` → `sp` プレフィックス
    - **原則** `std::make_shared<T>(args)` を使用
    ```cpp
    std::shared_ptr<MyClass> spObj = std::make_shared<MyClass>(args);
    ```
  - `std::weak_ptr<T>` → `wp` プレフィックス
    ```cpp
    std::weak_ptr<MyClass> wpObj = spObj;
    ```

- **推奨順序**:

  1. `std::make_shared<T>` / `std::make_unique<T>`
  2. 直接コンストラクタ呼び出し（必要時のみ）

- **生ポインタ(raw pointer) 禁止例**:

  ```cpp
  MyClass* pcRaw = new MyClass(...);  // NG
  ```

---

