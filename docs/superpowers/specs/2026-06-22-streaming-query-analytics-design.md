# Streaming query & analytics results — design

Status: APPROVED (design), 2026-06-22
Scope: classic N1QL query + analytics streaming, end to end (public API → core → transport)
Out of scope this spec: search & views streaming (deferred; the core machinery is built to absorb them later)

## Problem

A tiny N1QL statement can produce an enormous response:

```
curl -N -u Administrator:password -H 'Content-Type: application/json' \
  http://HOST:8093/query/service \
  -d '{"statement":"SELECT REPEAT(\"ABCDEFGHJIJKLMNOPQRSTUVWXYZ\",200) AS padding_data
        FROM [0] AS dummy
        UNNEST [0,1,2,3,4,5,6,7,8,9] AS a UNNEST ... AS e;"}'
# => ~519 MB chunked response
```

Today `cluster.query()` buffers the **entire** HTTP body into a `std::string`, then
`tao::json::parse()`s the whole document, then copies every row into
`std::vector<codec::binary>` before the user sees anything (`core/operations/document_query.cxx`
`make_response`, `core/impl/query.cxx` `build_result`/`map_rows`,
`couchbase/query_result.hxx`). For the query above that is ~0.5 GB resident plus full
transfer latency before the first row is observable. Unacceptable.

The user must be able to start iterating after the **preamble + first row**, pulling rows
lazily — exactly the model the existing range scan feature uses
(`couchbase/scan_result.hxx`, `core/range_scan_orchestrator.cxx`).

## Key finding: the engine already exists

A pull-streaming engine is already present in the tree as `core::row_streamer`
(`core/row_streamer.{hxx,cxx}`), currently compiled only under
`#ifdef COUCHBASE_CXX_CLIENT_COLUMNAR`. It provides exactly the required shape:

- `start()` → returns the **preamble** (everything before the first row; it patches the
  dangling `[` so the preamble is parseable JSON), carrying signature and any early error.
- `next_row()` → one row, delivered through an `asio::experimental::concurrent_channel`
  (genuine pull backpressure), discriminating a `row_stream_end_signal` from a row.
- `metadata()` → the **late** trailer (status/metrics/warnings) available only after the
  last row.
- `cancel()` → tears down the body and channel.

The transport below it is also already correct: `http_session::read_some()`
(`core/io/http_session.cxx`) posts **exactly one** `async_read_some` per consumer pull and
never self-rearms, so the asio read loop is naturally demand-gated. Pausing reads closes
the TCP receive window and stalls the server's `send()` — confirmed for chunked HTTP/1.1
over plain TCP and TLS (backpressure at the ciphertext layer for TLS).

So this project is **one generalization**, not a new subsystem: wire the existing engine to
classic query/analytics, fix its backpressure unit, add the N1QL error/metadata
interpretation, and expose a public pull handle.

### Columnar note

Columnar mode is slated for removal and is not a correctness concern here. We borrow
`row_streamer` (and the streaming transport) freely. The only constraint on columnar is
that **the columnar build must still compile**; its runtime behavior need not be preserved.
This removes the previously-feared work of unifying the `error_union` vs `std::error_code`
signature fork in `write_and_stream` — we standardize the non-columnar path on
`std::error_code` and keep columnar's existing call sites compiling (thin shim if needed),
nothing more.

## Architecture

```
L0  http_session::write_and_stream / read_some   bytes off socket; ONE async_read_some per pull   [exists, correct]
L1  json::streaming_lexer                         bytes → preamble | row strings | trailer events   [exists]
L2  core::row_streamer                            event demux + bounded channel + BYTE backpressure  [exists; generalize + fix watermark]
L3  core::query_stream<Traits>                    preamble→signature/early-error, trailer→meta,       [NEW]
                                                  terminal trailing-error mapping
L4a public query_stream_result / analytics_stream_result   pull handle                               [NEW]
L4b core collect_all adapter → query_response     buffered path = a streaming consumer that accumulates [NEW seam, identical output]
L5  buffered query()/analytics_query(), transactions, prepared-statement cache, FFI wrappers
```

The JSON lexer (L1) lives strictly **below** the channel (L2), where it is today: the lexer
is a synchronous push parser and the channel is the impedance-matcher that converts push
into pull. Do not move it above the channel.

### Per-service traits

`core::row_streamer` is parameterized only by a JSONPointer and depth. Introduce a small
descriptor so query and analytics (and later search/views) share L2/L3:

```cpp
struct streaming_service_traits {
  std::string  row_pointer;     // query & analytics: "/results/^"; search: "/hits/^"; views: "/rows/^"
  std::uint32_t lexer_depth;    // promote the current hardcoded LEXER_DEPTH=4 to a parameter
  std::function<std::error_code(const tao::json::value& trailer_meta, std::uint32_t http_status)> map_error;
  std::function<service_meta(const tao::json::value& preamble, const tao::json::value& trailer)> parse_meta;
};
```

Query and analytics differ only in their `map_error`/`parse_meta`; both use `/results/^`.

### Shared parsing extraction

Extract from the buffered path (`core/operations/document_query.cxx`) into shared functions
that both the streaming L3 and the buffered collect-all (L4b) call, so there is one source
of truth for N1QL semantics:

- `parse_query_meta(const tao::json::value&)` — from the field-walk in `make_response`.
- `map_query_error(status, errors, http_status)` — from the N1QL error table
  (codes 1065/1080/3000/4040…/12009/13014, `extract_common_query_error_code`).

## Public API

C++ floor is C++17 (`cmake/StandardProjectSettings.cmake`); no `<coroutine>` usage exists in
the tree. Primary idiom is therefore callback `next()`, with future `next()` and an eager
input iterator for parity with `scan_result`. No coroutine idiom in this spec (a `co_await`
adapter can be layered later without breaking anything).

New entry points on **both** `cluster` and `scope` (parity with existing `query()`):

```cpp
// cluster.hxx / scope.hxx
void query_stream(std::string statement, const query_options&, query_stream_handler&&) const;
auto query_stream(std::string statement, const query_options& = {}) const
    -> std::future<std::pair<error, query_stream_result>>;

// analytics, same shape
void analytics_query_stream(std::string statement, const analytics_options&, analytics_stream_handler&&) const;
auto analytics_query_stream(std::string statement, const analytics_options& = {}) const
    -> std::future<std::pair<error, analytics_stream_result>>;
```

```cpp
using query_stream_handler = std::function<void(error, query_stream_result)>;
using query_row_handler    = std::function<void(error, std::optional<query_row>)>;

class query_stream_result {
public:
  void next(query_row_handler&&) const;                                    // primary
  auto next() const -> std::future<std::pair<error, std::optional<query_row>>>;

  [[nodiscard]] auto signature() const -> std::optional<codec::binary>;    // early, synchronous
  [[nodiscard]] auto meta_data() const                                     // late; resolves at end of stream
      -> std::future<std::pair<error, query_meta_data>>;

  void cancel();

  class iterator;                                                          // eager, single-thread drain
  auto begin() -> iterator;
  auto end() -> iterator;
private:
  std::shared_ptr<internal_query_stream_result> internal_{};
};

class query_row {
public:
  template<typename Serializer = codec::tao_json_serializer,
           typename Document   = typename Serializer::document_type>
  [[nodiscard]] auto content_as() const -> Document;                       // serializer path (JSON), not KV transcoder
  [[nodiscard]] auto content_as_binary() const -> const codec::binary&;
private:
  codec::binary content_{};
};
```

`analytics_stream_result` / `analytics_row` are the structural mirror (analytics result is
already shaped like query — same rows+meta).

Reuse `query_options` / `analytics_options` verbatim; no new options type.

### Resolution point

- Buffered `query()`/`analytics_query()`: unchanged — resolves only after all rows + trailer
  parsed.
- `query_stream()`: resolves as soon as **headers + preamble** are parsed — signature and any
  upfront error are known, the handle is returned, rows are pulled afterward. This is the
  "give the user the handle fast" win.
- An **upfront** error (HTTP 4xx/5xx, or a query error emitted before the results array)
  resolves `query_stream()` itself with `{error, {}}` — no handle handed out.

### Error & metadata semantics (N1QL-specific crux)

N1QL emits rows first; status/errors/warnings/metrics arrive **after** the results array. So
a query can return HTTP 200, stream N valid rows, then a trailing `errors` block.

Three-state `next` contract — impossible to confuse success-end with failure-end:

| `error`  | `std::optional<query_row>` | meaning                                            |
|----------|----------------------------|----------------------------------------------------|
| falsy    | has value                  | a row                                              |
| falsy    | empty                      | stream completed **successfully**                  |
| truthy   | empty                      | stream ended **with a query error** (after N rows) |

A row is never delivered together with an error. The terminal error is produced by L3 running
the shared `map_query_error()` over the parsed trailer.

- `signature()` — synchronous, available once the stream resolves (preamble), empty before.
- `meta_data()` — returns a **future** that resolves only at end of stream. Called early it
  does **not** error and does **not** return empty; it resolves later. If the query ends in a
  terminal error, the future resolves with `{error, partial_meta}` so metadata access cannot
  silently swallow the failure.
- The eager `iterator` carries a terminal error in `pair.first`; it is flagged to resist the
  silent-swallow footgun (see Footguns).

## Backpressure & networking hardening

1. **Byte watermarks** in `row_streamer`, replacing the current row-count watermark
   (`ROW_BUFFER_SIZE=100` / refeed at `75`, which is the wrong unit for variable row sizes):
   resume reads below a low-water mark, pause above a high-water mark, measured in **bytes**
   of buffered row data. Defaults: **high 2 MB, low 512 KB**, both configurable (WAN BDP can
   need more). Keep a row-count cap as a secondary safety bound. Keep the 16 KB per-syscall
   read size. Leave `SO_RCVBUF` at the OS default — enlarging it raises the memory floor the
   server can push before backpressure engages.
2. **Per-row / trailer size ceiling** (default 64 MB) in the lexer: a single pathological row
   or a giant metadata trailer must raise a synthetic error rather than grow `body_chunk` /
   the lexer buffer without bound. The unavoidable minimum memory is one row in flight; this
   makes "one row" explicitly bounded.
3. **Idle timer armed only while a read is in flight.** Because reads are demand-gated, a
   legitimately slow consumer produces zero socket traffic and must not be timed out as a dead
   server. Arm the inter-chunk idle timer only between posting `async_read_some` and its
   completion; a fire then means the server stalled, not the consumer.
4. **Mid-stream cancel hard-closes the connection** (`http_session::stop()` → shutdown+close);
   the connection cannot return to the keep-alive pool, costing one handshake (TLS:
   re-handshake) on the next op to that node. Documented, accepted — drain-and-discard would
   re-download the body we are trying to avoid.

Networking risk acknowledged: an intermediary L7 proxy that buffers the whole response
defeats end-to-end backpressure (it absorbs the body at line rate). Couchbase query is
normally reached directly, which is favorable; documented as an assumption.

## Internal consumers (no regression)

- **Transactions never stream.** `cluster_ref().execute(query_request)` internally runs
  collect-all (L4b); `handle_query_error` and txdata consumption are untouched. Streaming a
  transaction query would be a correctness hazard given the trailing-error semantics.
- **Prepared-statement first execution forced buffered.** The PREPARE round-trip
  (`adhoc=false`, first execution) relies on parsing buffered `rows[0]` then
  `throw retry_http_request{}` to replay; this cannot be cleanly rewound mid-stream. Stream
  only `adhoc=true` and cache-hit executions; force collect-all otherwise.
- The bare push `row_callback` field on `query_request` is reimplemented as an internal
  pull-driver loop over `next_row()` (identical behavior) and marked `[[deprecated]]`.
- **Buffered output is byte-identical.** `query_response` (and the public buffered
  `query_result`) is produced by a collect-all consumer that drains the stream and accumulates;
  rows and metadata must match today's output exactly (equivalence test below).

## Cancellation, lifetime, thread-safety

- `cancel()` is best-effort and idempotent.
- The stream's lifetime is owned by the public result handle; `internal_query_stream_result`'s
  destructor calls `cancel()` (mirrors `internal_scan_result`). Dropping the handle mid-stream
  must not leak the connection.
- `next()` is **not** safe to call concurrently, and a second `next()` must not be issued
  before the prior handler fires (single outstanding pull, matching the channel). `cancel()`
  may be called from any thread. Documented.

## Testing strategy (TDD)

The curl example is the headline falsifiable test. Tests are written first and confirmed to
fail for the right reason before implementation.

1. **Memory-bounded streaming (headline).** Run the ~519 MB `REPEAT(...,200) UNNEST×5` query
   through `query_stream()`, pull only the first 5 rows, assert process RSS stays bounded
   (target well under 50 MB, vs ~0.5 GB today) and time-to-first-row ≪ full transfer time.
2. **Backpressure unit.** Slow consumer (sleep between `next()`); assert buffered bytes never
   exceed the high-water mark and that socket reads pause (instrument `read_some` call count).
3. **Trailing-error.** A query that streams rows then fails → the terminal `next()` yields a
   truthy error with empty optional; `meta_data()` future resolves with the error.
4. **Single-giant-value.** One row larger than the ceiling → synthetic error, no OOM.
5. **Buffered equivalence.** collect-all over the stream == today's buffered `query_result`
   (rows + meta byte-identical). Guards transactions and prepared-statement paths.
6. **Prepared-statement.** `adhoc=false` first execution still takes the buffered path and the
   retry-replay (`retry_http_request`) unwinds correctly.
7. **Cancel-mid-stream under ASan/TSan.** Abandon iteration at row 5 of a large result; no
   leak, no use-after-free across session retry. (Architect risk #3.)
8. **Analytics parity.** Items 1, 3, 5 repeated for `analytics_query_stream()`.

## Risks

1. **Late-error regression for transactions** → mitigated by never-stream-transactions plus the
   buffered-equivalence test.
2. **Prepared-statement retry-replay breaking under streaming** → forced-buffered policy plus
   the retry-exception unwinding test.
3. **Stream lifetime across retries / session teardown** → stream owned by the public handle;
   explicit ASan/TSan cancel-in-flight test.

## References

- Engine: `core/row_streamer.{hxx,cxx}`; lexer `core/utils/json_streaming_lexer.cxx`.
- Transport: `core/io/http_session.{hxx,cxx}` (`write_and_stream`, `read_some`, `do_read`,
  `stop`), `core/io/http_streaming_response.{hxx,cxx}`, `core/io/http_streaming_parser.cxx`,
  `core/io/http_message.hxx` (`streaming_settings`), `core/io/streams.cxx` (socket options, TLS).
- Buffered path to refactor: `core/operations/document_query.{hxx,cxx}` (`make_response`,
  error table), `core/impl/query.cxx` (`build_result`/`map_rows`).
- Public model: `couchbase/scan_result.hxx`, `couchbase/scan_result_item.hxx`,
  `core/impl/public_scan_result.cxx`, `core/range_scan_orchestrator.cxx`.
- Public types: `couchbase/query_result.hxx`, `couchbase/query_meta_data.hxx`,
  `couchbase/query_options.hxx`, `couchbase/analytics_result.hxx`, `couchbase/cluster.hxx`,
  `couchbase/scope.hxx`.
- Transactions: `core/transactions/attempt_context_impl.cxx` (`wrap_query`).
