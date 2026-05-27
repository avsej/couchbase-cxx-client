I am pleased to announce the availability of Couchbase C++ SDK 1.3.2 (released May 26, 2026).

This release supports SDK API 3.9: https://docs.couchbase.com/cxx-sdk/current/project-docs/compatibility.html#api-version

## What’s New in 1.3.2

* Node identification API — A new `couchbase::node_id` value type uniquely identifies a cluster node (the server-assigned UUID on Server 8.0.1+, a stable hostname/port hash on older releases). It is comparable and hashable, so it can be used directly as a map key.
* Per-node attribution for KV operations — Every `result` and `error` now reports the `node_id` of the serving node, replica reads expose a per-entry node id, and a new `collection::node_id_for()` helper resolves a key to its owning node locally with no network round-trip.
* Enumerate KV-serving nodes — `collection::node_ids()` returns the cluster nodes currently serving key-value traffic for a bucket, drawn from the client-side topology with no network round-trip. The returned ids match those reported on results and errors, making it straightforward to keep application-side, per-node state machines in sync with cluster topology.
* External circuit breaker example — A copy-pasteable, per-node circuit breaker (modelled on Hystrix / Resilience4j) demonstrating the new node id surface end to end.
* Bug fixes — Projected `get` with more than 16 paths no longer fails when a path is missing, and the HTTP session manager now correctly removes disconnected and stopped sessions from its session maps.
* Internal improvements — Reduced `std::regex` usage in error-handling paths in favour of plain string matching for lower overhead.

## Links

**Getting Started**: https://docs.couchbase.com/cxx-sdk/current/hello-world/overview.html

### Version 1.3.2 (May 26, 2026) — Recommended

Release Notes: https://docs.couchbase.com/cxx-sdk/current/project-docs/sdk-release-notes.html#version-1-3-2-26-may-2026
API Reference: https://docs.couchbase.com/sdk-api/couchbase-cxx-client-1.3.2
Source Code: https://github.com/couchbase/couchbase-cxx-client/releases/tag/1.3.2
--

Sergey Auseyau

Sr. Software Engineer, SDK

https://www.couchbase.com
