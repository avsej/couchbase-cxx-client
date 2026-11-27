#!/usr/bin/env ruby
# frozen_string_literal: true

#  Copyright 2026-Present Couchbase, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# Merges the SARIF files scan-build writes, one per translation unit, into one run for code
# scanning, which rejects several runs of one tool and category in one analysis.
#
# * Every URI under ROOT becomes relative to it.
# * A result is kept only when its primary location is a project source: under ROOT and outside
#   SKIP (build trees, where CPM unpacks dependencies on CI, vendored code, CMake's probes).
# * A run whose results are all outside ROOT is refused: ROOT is wrong, not the code clean.
# * A result reported from several translation units, as a header's is, is kept once.
# * Artifact and rule indices refer to arrays of the file they came from, so they are dropped;
#   the URI and the rule id carry the same information.
#
#   bin/merge-sarif.rb REPORT_DIR ROOT OUTPUT
#   bin/merge-sarif.rb --self-test

require "json"
require "tmpdir"

module MergeSarif
  SKIP = %r{\A(?:cmake-build-[^/]*|build[^/]*|\.cache|third_party|_deps)/}

  module_function

  def relativize!(node, prefix)
    case node
    when Hash
      node.delete("index")
      node.delete("ruleIndex")
      node["uri"] = node["uri"].delete_prefix(prefix) if node["uri"].is_a?(String)
      node.each_value { |v| relativize!(v, prefix) }
    when Array
      node.each { |v| relativize!(v, prefix) }
    end
  end

  def primary_uri(result)
    result.dig("locations", 0, "physicalLocation", "artifactLocation", "uri").to_s
  end

  def project_source?(result)
    uri = primary_uri(result)
    !uri.empty? && !uri.include?(":") && !uri.start_with?("/") && !SKIP.match?(uri)
  end

  def merge(files, root)
    prefix = "file://#{File.expand_path(root)}/"
    tool = nil
    rules = {}
    results = {}
    seen = 0
    under_root = 0
    files.sort.each do |file|
      JSON.parse(File.read(file)).fetch("runs").each do |run|
        relativize!(run, prefix)
        driver = run.dig("tool", "driver")
        tool ||= driver.reject { |k, _| k == "rules" }
        (driver["rules"] || []).each { |r| rules[r["id"]] ||= r }
        all = run["results"] || []
        seen += all.size
        under_root += all.count { |r| !primary_uri(r).include?(":") }
        all.select { |r| project_source?(r) }.each do |r|
          loc = r.dig("locations", 0, "physicalLocation")
          key = [r["ruleId"], loc.dig("artifactLocation", "uri"), loc["region"], r.dig("message", "text")]
          results[key] ||= r
        end
      end
    end
    # CMake's probes and, on CI, CPM's sources sit under ROOT, so a run with results always has
    # some there. None means ROOT does not match the paths clang wrote, and uploading the empty
    # run that follows would close every alert.
    raise ArgumentError, "none of #{seen} results is under #{root}" if seen.positive? && under_root.zero?

    used = results.each_value.map { |r| r["ruleId"] }.uniq
    driver = (tool || { "name" => "clang" }).merge("rules" => rules.values_at(*used).compact)
    {
      "$schema" => "https://json.schemastore.org/sarif-2.1.0.json",
      "version" => "2.1.0",
      "runs" => [{ "tool" => { "driver" => driver }, "results" => results.values }]
    }
  end

  def self_test
    Dir.mktmpdir do |dir|
      root = File.join(dir, "repo")
      result = lambda do |path, line|
        uri = path.start_with?("/") ? "file://#{path}" : "file://#{root}/#{path}"
        { "ruleId" => "r", "ruleIndex" => 0, "message" => { "text" => "m" },
          "locations" => [{ "physicalLocation" => { "artifactLocation" => { "index" => 0, "uri" => uri },
                                                    "region" => { "startLine" => line } } }] }
      end
      run = lambda do |*res|
        { "runs" => [{ "tool" => { "driver" => { "name" => "clang", "rules" => [{ "id" => "r" }] } },
                       "artifacts" => [], "results" => res }] }
      end
      File.write(File.join(dir, "a.sarif"), JSON.dump(run.call(result.call("core/a.hxx", 1),
                                                               result.call("cmake-build-x/_deps/d.cc", 2),
                                                               result.call("third_party/j.c", 3))))
      File.write(File.join(dir, "b.sarif"), JSON.dump(run.call(result.call("core/a.hxx", 1),
                                                               result.call("/usr/include/x.h", 4),
                                                               result.call("core/b.cxx", 5))))
      out = merge(Dir[File.join(dir, "*.sarif")], root)
      got = out["runs"][0]["results"].map { |r| r.dig("locations", 0, "physicalLocation", "artifactLocation") }
      want = [{ "uri" => "core/a.hxx" }, { "uri" => "core/b.cxx" }]
      mismatch = begin
        merge(Dir[File.join(dir, "*.sarif")], File.join(dir, "elsewhere"))
        false
      rescue ArgumentError
        true
      end
      empty = merge([], root)["runs"][0]["results"] == []
      ok = mismatch && empty && out["runs"].size == 1 && got == want && out["runs"][0]["tool"]["driver"]["rules"] == [{ "id" => "r" }] &&
           !JSON.dump(out).match?(/"(?:rule)?[iI]ndex"/)
      puts(ok ? "ok" : "FAILED: #{got.inspect}")
      ok
    end
  end
end

if $PROGRAM_NAME == __FILE__
  exit(MergeSarif.self_test ? 0 : 1) if ARGV == ["--self-test"]
  unless ARGV.size == 3
    warn "usage: #{$PROGRAM_NAME} REPORT_DIR ROOT OUTPUT | --self-test"
    exit 2
  end
  report, root, output = ARGV
  # scan-build creates REPORT_DIR/<timestamp>/ before the build; without it, it never ran. An
  # empty one is a clean analysis, and its empty run is what closes the remaining alerts.
  abort("no scan-build output under #{report}") if Dir[File.join(report, "*", "")].empty?
  files = Dir[File.join(report, "**", "*.sarif")]
  begin
    sarif = MergeSarif.merge(files, root)
  rescue ArgumentError => e
    abort(e.message)
  end
  File.write(output, JSON.pretty_generate(sarif))
  puts "#{files.size} files, #{sarif['runs'][0]['results'].size} results -> #{output}"
end
