1.3.2

New Features
------------

* CXXCBC-820: **Node Identification API** -- Introduced `couchbase::node_id`,
  a value type that uniquely identifies a cluster node. On Server 8.0.1+ the
  identifier is the server-assigned `nodeUUID`; on older releases a stable
  CRC32-based hash is derived from the node's hostname and KV port. The type
  is equality-comparable, ordered, and hashable (a `std::hash` specialization
  is provided), so it can be used directly as a key in application-side maps.

* CXXCBC-821: **Node Attribution for KV Operations** -- `node_id` is now
  propagated through the entire key-value operation path. `couchbase::result`
  gains a `node_id()` accessor and `couchbase::error` gains a `node_id()`
  accessor, so callers can attribute every result and error to the serving
  cluster node. Replica reads expose a per-entry `node_id` on the results of
  `get_all_replicas()` and `lookup_in_all_replicas()`. A new client-side
  helper, `collection::node_id_for(document_id, options)`, resolves a key to
  its owning node from the vBucket map with no network round-trip.

* CXXCBC-826: **Enumerate KV-Serving Nodes** -- Added
  `collection::node_ids(options, handler)` (with a `std::future`-returning
  overload) to enumerate the cluster nodes that currently serve key-value
  traffic for a collection's bucket, drawn from the client-side topology
  snapshot with no network round-trip. Each entry is the same `node_id` the
  SDK reports on KV results and errors, so the result is directly comparable
  to the keys of an application's `unordered_map<node_id, ...>` — exactly what
  a state machine needs to retire tracking entries when a node leaves the
  cluster topology. Nodes that do not expose a KV port for the configured
  transport are filtered out.

* CXXCBC-822: **External Circuit Breaker Example** -- Added a copy-pasteable
  circuit breaker example built on the `node_id` API and modelled on Netflix
  Hystrix / Resilience4j. It demonstrates a per-node registry keyed on
  `couchbase::node_id`, a sliding-window state machine
  (closed/open/half_open/forced_open/disabled), failure-rate and slow-call-rate
  tripping, per-node isolation, and a topology-sweep that prunes tracker state
  using `collection::node_ids()`.

Bug Fixes
---------

* CXXCBC-785: **Projected Get with > 16 Paths** -- A projected `get` requesting
  more than 16 paths no longer fails when one of the requested paths does not
  exist in the document. Missing paths are now skipped rather than aborting the
  whole projection.

* CXXCBC-823: **HTTP Session Manager Cleanup** -- Fixed `check_in()` not
  removing sessions from the pending/idle session maps when a session is not
  connected, and updated each session's `on_stop()` to remove itself from all
  session maps. Destruction of removed sessions is deferred until after the
  manager mutex is released, avoiding holding the lock across session teardown.
  A code comment and `cppcheck-suppress` directive document this deliberate
  deferred-destruction idiom.

Improvements
------------

* CXXCBC-828: **Reduced std::regex Usage in Error Handling** -- Replaced inline
  `std::regex` construction with `std::string::find()` when building errors
  from server response text in collection and scope management operations.
  Where case-insensitive matching is still required, the `std::regex` objects
  are now `static const` so they are compiled once.

* CXXCBC-829: **contains_string Utility** -- Added
  `utils::contains_string(input, substr, ignore_case = false)`, a substring
  match with optional ASCII-only case folding and no `std::locale` dependency,
  and replaced the remaining `std::regex_search` calls with it.
