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

# Reads the output of a check and reports each finding at its source position: as GitHub workflow
# commands and a step summary under GitHub Actions, as a grouped list otherwise.
#
#   bin/findings.rb --from FORMAT [--tool NAME] INPUT...
#   bin/findings.rb --self-test
#
# FORMAT is one of:
#   gcc         path:line:col: level: message [rule] (clang, GCC, cppcheck, check-log-annotations)
#   msvc        path(line,col): error C1234: message [project], as MSBuild prints it
#   clang-tidy  the YAML file written by clang-tidy -export-fixes
#   diff        the unified diff printed by git-clang-format --diff
#   ctest       Testing/Temporary/LastTest.log: test framework failures and sanitizer reports
#               of failed tests
#
# GitHub annotates at most 10 errors and 10 warnings per step, so findings on lines the commit
# changed go first; the step summary lists the rest. Warnings outside the project are dropped;
# errors outside it are listed without a position. A missing input is logged, not fatal: a passing
# check may write nothing, and a step that never ran is already marked. The exit status is 0
# unless the arguments are wrong (2), also when the input cannot be read, so this never changes the
# outcome of a job.

require "optparse"
require "set"
require "tmpdir"
require "yaml"
require_relative "merge-sarif"

module Findings
  Finding = Struct.new(:tool, :level, :path, :line, :col, :end_line, :rule, :message, :detail,
                       keyword_init: true)

  LEVELS = %w[error warning notice].freeze
  PER_LEVEL = 10        # annotations GitHub keeps per level and step
  SUMMARY_ROWS = 200    # a step summary over 1 MiB is dropped
  MESSAGE_BYTES = 4000  # the runner cuts a message at 4096 characters
  DETAIL_LINES = 16
  SGR = /\e\[[0-9;]*[A-Za-z]/

  module_function

  # Lines of a log as valid UTF-8 without colour, CR or newline. Logs carry colour (the build forces
  # it), CRLF (Windows) and stray bytes (test output), none of which may stop the report.
  def each_line(path)
    return enum_for(:each_line, path) unless block_given?

    File.open(path, "rb") do |io|
      io.each_line do |raw|
        yield raw.force_encoding(Encoding::UTF_8).scrub("?").gsub(SGR, "").delete("\r").chomp
      end
    end
  end

  # Positions in the repository. A path outside ROOT, in a build tree or vendored code, or naming
  # no file, is not a project source and yields nil.
  class Repo
    attr_reader :root

    def initialize(root)
      @root = File.expand_path(root).tr("\\", "/").chomp("/")
      @cache = {}
    end

    def relative(path)
      return nil if path.nil? || path.empty?

      @cache.fetch(path) { @cache[path] = resolve(path) }
    end

    def strip_root(text)
      text.gsub(/#{Regexp.escape(@root)}\//i, "").gsub(/#{Regexp.escape(@root.tr('/', '\\'))}\\/i, "")
    end

    # { path => Set of line numbers } that HEAD changed against HEAD^. On a pull request HEAD is
    # the merge commit and HEAD^ the base. Empty without HEAD^ (a depth 1 checkout).
    def changed_lines
      @changed_lines ||= begin
        out = IO.popen(["git", "-C", @root, "diff", "--no-color", "-U0", "HEAD^", "HEAD"],
                       err: File::NULL, &:read)
        $?.success? ? parse_changed(out) : {}
      rescue SystemCallError
        {}
      end
    end

    def ranking?
      !changed_lines.empty?
    end

    private

    def resolve(path)
      p = path.tr("\\", "/")
      absolute = p.start_with?("/") || p.match?(%r{\A[A-Za-z]:/})
      p = File.expand_path(p).tr("\\", "/") unless absolute
      prefix = "#{@root}/"
      return nil unless p.downcase.start_with?(prefix.downcase)

      rel = p[prefix.length..]
      return nil if MergeSarif::SKIP.match?(rel) || !File.file?(File.join(@root, rel))

      rel
    end

    def parse_changed(diff)
      lines = Hash.new { |h, k| h[k] = Set.new }
      file = nil
      diff.each_line do |l|
        if (m = l.match(%r{\A b/(.+?)\t?$}))
          file = m[1]
        elsif file && (m = l.match(/\A@@ -\S+ \+(\d+)(?:,(\d+))? @@/))
          start = m[1].to_i
          count = (m[2] || "1").to_i
          count.zero? ? lines[file] << start : (start...(start + count)).each { |n| lines[file] << n }
        end
      end
      lines
    end
  end

  RULE_SUFFIX = /,-warnings-as-errors\z/

  def rule_name(raw)
    return nil if raw.nil?

    raw.sub(RULE_SUFFIX, "").sub(/\A-Werror,/, "").sub(/\A-Werror=/, "-W")
  end

  GCC = /\A(?<path>[^\s:][^:]*?):(?<line>\d+):(?:(?<col>\d+):)?\s+
         (?<level>fatal\ error|error|warning|style|performance|portability|information):\s+
         (?<message>.*?)(?:\s+\[(?<rule>[^\]\s]+)\])?\z/x
  GCC_LEVELS = { "fatal error" => "error", "error" => "error", "warning" => "warning",
                 "style" => "warning", "performance" => "warning",
                 "portability" => "warning" }.freeze

  # clang, GCC, cppcheck and anything else in their shape. Notes and include context are not
  # findings, and cppcheck's "information" lines report on the run, not on the code.
  def parse_gcc(path, tool, _repo)
    each_line(path).map do |l|
      m = l.match(GCC) or next
      level = GCC_LEVELS[m[:level]] or next
      col = m[:col].to_i
      line = m[:line].to_i
      Finding.new(tool: tool, level: level, path: m[:path], line: line.positive? ? line : nil,
                  col: col.positive? ? col : nil, rule: rule_name(m[:rule]), message: m[:message])
    end.compact
  end

  MSVC = /\A\s*(?:\d+>)?(?<path>(?:[A-Za-z]:)?[^(:]+)\((?<line>\d+)(?:,(?<col>\d+))?\)\s*:\s+
          (?<level>fatal\ error|error|warning)\s+(?<rule>[A-Z]+\d+)\s*:\s+(?<message>.*?)
          (?:\s+\[[^\]]+\])?\z/x

  # MSBuild prefixes the project number, appends the project file and repeats every diagnostic
  # in its closing summary; dedupe collapses the repeats.
  def parse_msvc(path, tool, _repo)
    each_line(path).map do |l|
      m = l.match(MSVC) or next
      Finding.new(tool: tool, level: m[:level] == "warning" ? "warning" : "error", path: m[:path].strip,
                  line: m[:line].to_i, col: m[:col]&.to_i, rule: m[:rule], message: m[:message])
    end.compact
  end

  # clang-tidy names the position as a byte offset; line and column are counted from the file.
  def parse_clang_tidy(path, tool, _repo)
    sources = Hash.new { |h, file| h[file] = File.file?(file) ? File.binread(file) : nil }
    position = lambda do |file, offset|
      text = sources[file]
      return [nil, nil] if text.nil? || offset.nil?

      head = text.byteslice(0, offset)
      [head.count("\n") + 1, offset - (head.rindex("\n") || -1)]
    end
    File.binread(path).force_encoding(Encoding::UTF_8).scrub("?").split(/^---\s*$/).flat_map do |doc|
      next [] if doc.strip.empty? || doc.strip == "..."

      data = YAML.safe_load(doc) || {}
      (data["Diagnostics"] || []).map do |d|
        msg = d["DiagnosticMessage"] || {}
        level = { "Error" => "error", "Warning" => "warning" }[d["Level"]] or next
        line, col = position.call(msg["FilePath"], msg["FileOffset"])
        notes = (d["Notes"] || []).map do |n|
          next if n["Message"] == msg["Message"]

          nline, = position.call(n["FilePath"], n["FileOffset"])
          "note: #{n['FilePath']}:#{nline}: #{n['Message']}"
        end.compact
        Finding.new(tool: tool, level: level, path: msg["FilePath"], line: line, col: col,
                    rule: rule_name(d["DiagnosticName"]), message: msg["Message"].to_s,
                    detail: notes.empty? ? nil : notes.join("\n"))
      end.compact
    end
  end

  # One finding per run of changed lines, positioned on the lines as they are now, with the
  # formatted text as the detail.
  def parse_diff(path, tool, _repo)
    findings = []
    file = nil
    old = 0
    run = nil
    flush = lambda do
      if run
        findings << Finding.new(tool: tool, level: "error", path: file, line: run[:first],
                                end_line: run[:last], rule: nil,
                                message: "clang-format would change #{run[:last] > run[:first] ? 'these lines' : 'this line'}",
                                detail: run[:added].empty? ? "(lines removed)" : run[:added].join("\n"))
      end
      run = nil
    end
    each_line(path).each do |l|
      if l.start_with?("diff --git ")
        flush.call
        file = nil
      elsif (m = l.match(%r{\A--- a/(.+?)\t?\z}))
        file = m[1]
      elsif l.start_with?("+++ ")
        next
      elsif (m = l.match(/\A@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@/))
        flush.call
        old = m[1].to_i
      elsif file.nil? || l.start_with?("\\") # "\ No newline at end of file"
        next
      elsif l.start_with?("-")
        run ||= { first: old, last: old, added: [] }
        run[:last] = old
        old += 1
      elsif l.start_with?("+")
        run ||= { first: [old, 1].max, last: [old, 1].max, added: [] }
        run[:added] << l[1..]
      else
        flush.call
        old += 1
      end
    end
    flush.call
    findings
  end

  FRAMEWORK_FAILURE = /\A"(?<case>.+)" FAILED: (?:(?<path>[^\s:][^:]*?):(?<line>\d+): )?(?<message>.*)\z/
  SANITIZER_START = /\A(?:==\d+==ERROR: |WARNING: )(?<kind>AddressSanitizer|LeakSanitizer|ThreadSanitizer|MemorySanitizer):/
  FRAME = /\A\s+#\d+ .*?(?<path>(?:[A-Za-z]:)?\/[^\s:]+):(?<line>\d+)(?::(?<col>\d+))?(?:\s|\z)/

  # The per-test blocks of LastTest.log. Only failed tests report: a passing test may print a
  # FAILED line on purpose (the framework's own self-test does). A failed test with no recognised
  # line reports its name. Output without test blocks is read as one failed test.
  def parse_ctest(path, tool, repo)
    findings = []
    pending = []
    test = nil
    report = nil
    in_output = false
    blocks = false
    each_line(path).each do |l|
      if (m = l.match(%r{\A\d+/\d+ Test: (.+)\z}))
        blocks = true
        test = m[1]
        pending = []
        report = nil # a report cut short, by a timeout kill, ends with its test
      elsif l == "<end of output>"
        in_output = false
      elsif blocks && !in_output && l == "Output:"
        in_output = :header
      elsif in_output == :header
        in_output = true
      elsif blocks && !in_output && l.start_with?("Test Failed", "Test Passed", "Test Not Run")
        if l.start_with?("Test Failed")
          pending << Finding.new(tool: tool, level: "error", rule: test, message: "#{test} failed") if pending.empty?
          findings.concat(pending)
        end
        pending = []
      elsif (m = l.match(FRAMEWORK_FAILURE))
        pending << Finding.new(tool: tool, level: "error", path: m[:path], line: m[:line]&.to_i,
                               rule: test, message: "#{m[:case]}: #{m[:message]}")
      elsif (m = l.match(SANITIZER_START))
        report = { lines: [l], frames: [] }
      elsif report
        if (f = l.match(FRAME))
          report[:frames] << [f[:path], f[:line].to_i, f[:col]&.to_i]
        end
        report[:lines] << l if report[:lines].size < DETAIL_LINES
        next unless (s = l.match(/\ASUMMARY: (.*)\z/))

        frame = report[:frames].find { |fpath, *| repo.relative(fpath) } || []
        summary = s[1].sub(/ (?:[A-Za-z]:)?\/\S+:\d+(?::\d+)?/, "")
        pending << Finding.new(tool: tool, level: "error", path: frame[0], line: frame[1], col: frame[2],
                               rule: test, message: summary, detail: report[:lines].join("\n"))
        report = nil
      end
    end
    findings.concat(pending) unless blocks
    findings
  end

  PARSERS = { "gcc" => :parse_gcc, "msvc" => :parse_msvc, "clang-tidy" => :parse_clang_tidy,
              "diff" => :parse_diff, "ctest" => :parse_ctest }.freeze

  def collect(format, tool, inputs, repo)
    missing = inputs.reject { |input| File.file?(input) }
    findings = (inputs - missing).flat_map { |input| send(PARSERS.fetch(format), input, tool, repo) }
    findings = findings.select do |f|
      outside = !f.path.nil? && repo.relative(f.path).nil?
      f.path = repo.relative(f.path)
      f.line = f.col = f.end_line = nil unless f.path
      f.detail = repo.strip_root(f.detail) if f.detail
      # A warning in vendored code fails nothing and cannot be changed here.
      !(outside && f.level == "warning")
    end
    [findings.uniq { |f| [f.rule, f.path, f.line, f.col, f.message] }, missing]
  end

  # On a changed line, in a changed file, elsewhere in the repository, without a position; errors
  # before warnings within each.
  def rank(findings, repo)
    changed = repo.changed_lines
    findings.sort_by do |f|
      lines = changed.fetch(f.path, nil)
      group = if f.path.nil? then 3
              elsif lines.nil? then 2
              elsif (f.line.to_i..(f.end_line || f.line.to_i)).any? { |n| lines.include?(n) } then 0
              else 1
              end
      [group, LEVELS.index(f.level), f.path.to_s, f.line.to_i, f.col.to_i]
    end
  end

  def escape_data(text)
    text.to_s.gsub("%", "%25").gsub("\r", "%0D").gsub("\n", "%0A")
  end

  def escape_property(text)
    escape_data(text).gsub(":", "%3A").gsub(",", "%2C")
  end

  def command(f)
    props = []
    if f.path
      props << "file=#{escape_property(f.path)}"
      props << "line=#{f.line}" if f.line
      props << "endLine=#{f.end_line}" if f.end_line && f.end_line != f.line
      props << "col=#{f.col}" if f.col && !f.end_line
    end
    props << "title=#{escape_property([f.tool, f.rule].compact.join(': '))}"
    text = f.detail ? "#{f.message}\n\n#{f.detail}" : f.message
    text = "#{text.byteslice(0, MESSAGE_BYTES).scrub('')}…" if text.bytesize > MESSAGE_BYTES
    "::#{f.level} #{props.join(',')}::#{escape_data(text)}"
  end

  def cell(text)
    text.to_s.gsub(/\s+/, " ").gsub("|", "\\|").gsub("<", "&lt;").gsub("`", "'")
  end

  def summary(findings, tool, annotated, repo)
    out = +"### #{cell(tool)}: #{findings.size} #{findings.size == 1 ? 'finding' : 'findings'}\n\n"
    nowhere = findings.count { |f| f.path.nil? }
    over = findings.size - nowhere - annotated
    out << "#{over} not annotated: GitHub keeps #{PER_LEVEL} per level in a step.\n\n" if over.positive?
    out << "#{nowhere} without a position in the repository.\n\n" if nowhere.positive?
    out << "Not ranked by the diff: the checkout has no parent commit.\n\n" unless repo.ranking?
    out << "| Location | Level | Rule | Message |\n|---|---|---|---|\n"
    findings.first(SUMMARY_ROWS).each do |f|
      where = f.path ? [f.path, f.line].compact.join(":") : ""
      out << "| #{cell(where)} | #{f.level} | #{cell(f.rule)} | #{cell(f.message)} |\n"
    end
    out << "\n#{findings.size - SUMMARY_ROWS} more not listed.\n" if findings.size > SUMMARY_ROWS
    out
  end

  def text(findings)
    findings.group_by(&:path).map do |path, in_file|
      body = in_file.map do |f|
        where = [f.line, f.col].compact.join(":")
        "  #{where.empty? ? '' : "#{where}: "}#{f.level}: #{f.message}#{f.rule ? " [#{f.rule}]" : ''}"
      end
      "#{path || '(no position)'}\n#{body.join("\n")}"
    end.join("\n")
  end

  # Returns the lines for stdout and, under GitHub Actions, the step summary.
  def report(findings, missing, tool, repo, github:)
    # Not an annotation: a check that passes may write nothing, and a step that never ran is
    # already marked.
    out = missing.map { |m| "#{tool}: #{m} not found, nothing to report" }
    return [out, nil] if findings.empty?

    ranked = rank(findings, repo)
    return [out << text(ranked), nil] unless github

    # A finding without a position (vendored code, a stack with no project frame) is listed in the
    # summary only: as an annotation it would not point at anything the pull request can change.
    shown = ranked.select(&:path).group_by(&:level).values.flat_map { |group| group.first(PER_LEVEL) }
    out.concat(shown.map { |f| command(f) })
    [out, summary(ranked, tool, shown.size, repo)]
  end

  def run(format, tool, inputs, root: File.expand_path("..", __dir__), github: ENV["GITHUB_ACTIONS"] == "true")
    repo = Repo.new(root)
    findings, missing = collect(format, tool, inputs, repo)
    lines, step_summary = report(findings, missing, tool, repo, github: github)
    puts lines
    if step_summary && ENV["GITHUB_STEP_SUMMARY"]
      File.open(ENV["GITHUB_STEP_SUMMARY"], "a") { |f| f.puts step_summary }
    end
    0
  rescue StandardError => e
    # Not an annotation: the check's own step carries its outcome.
    puts "#{tool}: cannot read #{inputs.join(' ')}: #{e.class}: #{e.message.lines.first.to_s.strip}"
    0
  end

  # Fixtures are tool output captured from clang 24, GCC 16, cppcheck 2.13, clang-tidy 24,
  # git-clang-format, ctest 4.4 and an MSBuild log from CI, cut down, with the repository path
  # replaced by ROOT.
  FIXTURES = {
    "clang.log" => <<~'LOG',
      In file included from core/a.cxx:1:
      \e[1mROOT/core/widget.hxx:2:31: \e[0m\e[0;1;31merror: \e[0m\e[1munused variable 'unused' [-Werror,-Wunused-variable]\e[0m
          2 | inline int twice(int x) { int unused; return x * 2; }
      \e[0m\e[1mROOT/core/a.cxx:3:71: \e[0m\e[0;1;31merror: \e[0m\e[1muse of undeclared identifier 'undeclared'\e[0m
      ROOT/core/a.cxx:3:10: note: declared here
      ROOT/cmake-build-tests/_deps/x/x.h:1:1: warning: vendored [-Wfoo]
      ROOT/cmake-build-tests/_deps/x/x.h:1:5: error: vendored error [-Wbar]
      2 errors generated.
    LOG
    "gcc.log" => <<~'LOG',
      In file included from \e[01m\e[Kcore/b.cxx:1\e[m\e[K:
      \e[01m\e[Kcore/widget.hxx:\e[m\e[K In function 'int twice(int)':
      \e[01m\e[Kcore/widget.hxx:2:31:\e[m\e[K \e[01;31m\e[Kerror: \e[m\e[Kunused variable '\e[01m\e[Kunused\e[m\e[K' [\e[01;31m\e[K-Werror=unused-variable\e[m\e[K]
      cc1plus: all warnings being treated as errors
    LOG
    "cppcheck.log" => <<~'LOG',
      ROOT/core/c.cxx:1:0: information: Include file: <cstdlib> not found. [missingIncludeSystem]
      ROOT/core/c.cxx:2:27: error: Array 'a[3]' accessed at index 3, which is out of bounds. [arrayIndexOutOfBounds]
      int h(int n) { int a[3]; a[3] = n; return a[0]; }
                                ^
      ROOT/core/c.cxx:3:12: style: Struct 's' has a constructor with 1 argument that is not explicit. [noExplicitConstructor]
      ROOT/core/c.cxx:2:0: style: The function 'h' is never used. [unusedFunction]
      ROOT/core/c.cxx:0:0: error: syntax error [syntaxError]
      nofile:0:0: information: Active checkers: 161/592 (use --checkers-report=<filename> to see details) [checkersReport]
    LOG
    "msvc.log" => <<~'LOG',
         208>WINROOT\core\a.cxx(3,62): error C2838: 'X': illegal qualified name in member declaration [WINROOT\cmake-build-tests\x.vcxproj]
         208>WINROOT\core\a.cxx(3,62): error C2065: 'X': undeclared identifier [WINROOT\cmake-build-tests\x.vcxproj]
               WINROOT\core\a.cxx(3,62): error C2838: 'X': illegal qualified name in member declaration [WINROOT\cmake-build-tests\x.vcxproj]
               WINROOT\core\a.cxx(3,62): error C2065: 'X': undeclared identifier [WINROOT\cmake-build-tests\y.vcxproj]
    LOG
    "tidy.yaml" => <<~'LOG',
      ---
      MainSourceFile:  'ROOT/core/a.cxx'
      Diagnostics:
        - DiagnosticName:  cppcoreguidelines-avoid-c-arrays
          DiagnosticMessage:
            Message:         'do not declare C-style arrays, use ''std::array'' instead'
            FilePath:        'ROOT/core/a.cxx'
            FileOffset:      64
            Replacements:    []
          Level:           Error
        - DiagnosticName:  clang-analyzer-security.insecureAPI.strcpy
          DiagnosticMessage:
            Message:         'Call to function ''strcpy'' is insecure'
            FilePath:        'ROOT/core/a.cxx'
            FileOffset:      77
            Replacements:    []
          Notes:
            - Message:         'Call to function ''strcpy'' is insecure'
              FilePath:        'ROOT/core/a.cxx'
              FileOffset:      77
            - Message:         'the buffer is declared here'
              FilePath:        'ROOT/core/a.cxx'
              FileOffset:      64
          Level:           Error
      ...
      ---
      MainSourceFile:  'ROOT/core/b.cxx'
      Diagnostics:
        - DiagnosticName:  cppcoreguidelines-avoid-c-arrays
          DiagnosticMessage:
            Message:         'do not declare C-style arrays, use ''std::array'' instead'
            FilePath:        'ROOT/core/a.cxx'
            FileOffset:      64
            Replacements:    []
          Level:           Error
      ...
    LOG
    "format.diff" => <<~'LOG',
      clang-format suggested the following changes:

      diff --git a/core/b.cxx b/core/b.cxx
      index e4946b2..440a50a 100644
      --- a/core/b.cxx
      +++ b/core/b.cxx
      @@ -1,6 +1,18 @@
       #include "widget.hxx"
       int g() { return twice(2); }
      -int  k( int a ){return a;}
      +int
      +k(int a)
      +{
      +  return a;
      +}

      -int ok() { return 0; }
      -int   m(){ return 1 ; }
      +int
      +ok()
      +{
      +  return 0;
      +}
      diff --git a/core/c.cxx b/core/c.cxx
      --- a/core/c.cxx	
      +++ b/core/c.cxx	
      @@ -3 +3,2 @@
      -struct s { s(int x) : v(x) {} int v; };
      \ No newline at end of file
      +struct s
      +{ s(int x) : v(x) {} int v; };
      \ No newline at end of file
    LOG
    "LastTest.log" => <<~'LOG',
      1/4 Testing: unit_asan
      1/4 Test: unit_asan
      Output:
      ----------------------------------------------------------
      =================================================================
      ==2167900==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x7be6f4de0020 at pc 0x00000055ce55
      READ of size 4 at 0x7be6f4de0020 thread T0
          #0 0x00000055ce54 in read_past(int*) ROOT/test/unit/asan.cxx:3:39
          #1 0x00000055cdf4 in main ROOT/test/unit/asan.cxx:4:125
          #2 0x7f7ce947f340 in __libc_start_call_main (/lib64/libc.so.6+0x7f340) (BuildId: fcb7)
      SUMMARY: AddressSanitizer: heap-buffer-overflow ROOT/test/unit/asan.cxx:3:39 in read_past(int*)
      ==2167900==ABORTING
      <end of output>
      Test time =   0.02 sec
      ----------------------------------------------------------
      Test Failed.
      2/4 Testing: unit_tsan
      2/4 Test: unit_tsan
      Output:
      ----------------------------------------------------------
      ==================
      WARNING: ThreadSanitizer: data race (pid=2167902)
        Write of size 4 at 0x0000018a37d8 by thread T2:
          #0 std::__invoke_impl /usr/include/c++/16/bits/invoke.h:63:14 (tsan+0x527bf9) (BuildId: f80d)
          #1 bump() ROOT/test/unit/tsan.cxx:3:22 (tsan+0x527b9c) (BuildId: f80d)
      SUMMARY: ThreadSanitizer: data race ROOT/test/unit/tsan.cxx:3:22 in bump()
      ==================
      <end of output>
      Test Failed.
      3/4 Testing: unit_lsan
      3/4 Test: unit_lsan
      Output:
      ----------------------------------------------------------
      ==2167320==ERROR: LeakSanitizer: detected memory leaks
      Direct leak of 64 byte(s) in 1 object(s) allocated from:
          #0 0x00000044f873 in malloc (ROOT/build/lsan+0x44f873) (BuildId: b147)
          #1 0x00000045300d in leak() ROOT/test/unit/lsan.cxx:3:48
      SUMMARY: LeakSanitizer: 64 byte(s) leaked in 1 allocation(s).
      <end of output>
      Test Failed.
      4/4 Testing: unit_framework_selftest
      4/4 Test: unit_framework_selftest
      Output:
      ----------------------------------------------------------
      "an expected failure" FAILED: ROOT/test/unit/fail.cxx:9: printed on purpose
      <end of output>
      Test Passed.
      5/5 Testing: unit_fail
      5/5 Test: unit_fail
      Output:
      ----------------------------------------------------------
      "an empty key is rejected" FAILED: ROOT/test/unit/fail.cxx:12: expected true, got false
      [trace] binary \xff\xfe bytes
      <end of output>
      Test Failed.
      6/8 Testing: unit_timeout
      6/8 Test: unit_timeout
      Output:
      ----------------------------------------------------------
      ==7==ERROR: AddressSanitizer: stack-overflow on address 0x7ffd
          #0 0x55 in f() ROOT/test/unit/asan.cxx:2:1
      <end of output>
      Test Failed.
      7/8 Testing: unit_after_timeout
      7/8 Test: unit_after_timeout
      Output:
      ----------------------------------------------------------
      SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior ROOT/test/unit/asan.cxx:1:1
      "a throwing case" FAILED: threw std::runtime_error: boom
      <end of output>
      Test Failed.
      8/8 Testing: unit_crash
      8/8 Test: unit_crash
      Output:
      ----------------------------------------------------------
      Segmentation fault
      <end of output>
      Test Failed.
    LOG
  }.freeze

  SOURCES = {
    "core/widget.hxx" => "#pragma once\ninline int twice(int x) { int unused; return x * 2; }\n",
    "core/a.cxx" => "#include \"widget.hxx\"\n#include <cstring>\n" \
                    "int f(const char* s) { char buf[4]; strcpy(buf, s); return twice(1); }\n",
    "core/b.cxx" => "#include \"widget.hxx\"\nint g() { return twice(2); }\nint  k( int a ){return a;}\n\n" \
                    "int ok() { return 0; }\nint   m(){ return 1 ; }\n",
    "core/c.cxx" => "#include <cstdlib>\nint h(int n) { int a[3]; a[3] = n; return a[0]; }\nstruct s { s(int x) : v(x) {} int v; };\n",
    "test/unit/asan.cxx" => "\n\n\n\n", "test/unit/tsan.cxx" => "\n\n\n",
    "test/unit/lsan.cxx" => "\n\n\n", "test/unit/fail.cxx" => "\n" * 12,
    "cmake-build-tests/_deps/x/x.h" => "\n"
  }.freeze

  def self_test
    failures = []
    check = ->(name, ok) { failures << name unless ok }
    Dir.mktmpdir do |dir|
      root = File.join(dir, "repo")
      SOURCES.each do |rel, body|
        FileUtils.mkdir_p(File.dirname(File.join(root, rel)))
        File.write(File.join(root, rel), body)
      end
      win = root.tr("/", "\\").upcase # MSBuild paths: backslashes, another case
      FIXTURES.each do |name, body|
        body = body.gsub("\\e", "\e").gsub("\\xff\\xfe", "\xff\xfe".b.force_encoding(Encoding::UTF_8))
        File.binwrite(File.join(dir, name), body.gsub("WINROOT", win).gsub("ROOT", root))
      end
      repo = Repo.new(root)
      found = lambda do |format, file|
        Dir.chdir(root) { collect(format, format, [File.join(dir, file)], repo).first }
      end
      at = ->(fs) { fs.map { |f| [f.path, f.line, f.col, f.level, f.rule] } }

      clang = found.call("gcc", "clang.log")
      check.call("clang: colour stripped, notes dropped, vendored error unpositioned", at.call(clang) == [
        ["core/widget.hxx", 2, 31, "error", "-Wunused-variable"], ["core/a.cxx", 3, 71, "error", nil],
        [nil, nil, nil, "error", "-Wbar"]
      ] && clang[1].message == "use of undeclared identifier 'undeclared'")
      check.call("gcc: relative path, -Werror= rule",
                 at.call(found.call("gcc", "gcc.log")) == [["core/widget.hxx", 2, 31, "error", "-Wunused-variable"]])
      check.call("cppcheck: levels, column 0, information dropped", at.call(found.call("gcc", "cppcheck.log")) == [
        ["core/c.cxx", 2, 27, "error", "arrayIndexOutOfBounds"], ["core/c.cxx", 3, 12, "warning", "noExplicitConstructor"],
        ["core/c.cxx", 2, nil, "warning", "unusedFunction"], ["core/c.cxx", nil, nil, "error", "syntaxError"]
      ])
      check.call("msvc: Windows path, project suffix, summary repeats",
                 at.call(found.call("msvc", "msvc.log")) == [["core/a.cxx", 3, 62, "error", "C2838"],
                                                             ["core/a.cxx", 3, 62, "error", "C2065"]])
      tidy = found.call("clang-tidy", "tidy.yaml")
      check.call("clang-tidy: offsets, documents merged, header deduped", at.call(tidy) == [
        ["core/a.cxx", 3, 24, "error", "cppcoreguidelines-avoid-c-arrays"],
        ["core/a.cxx", 3, 37, "error", "clang-analyzer-security.insecureAPI.strcpy"]
      ] && tidy[1].detail == "note: core/a.cxx:3: the buffer is declared here")
      diff = found.call("diff", "format.diff")
      check.call("diff: one finding per run, current line numbers",
                 diff.map { |f| [f.path, f.line, f.end_line] } == [["core/b.cxx", 3, 3], ["core/b.cxx", 5, 6], ["core/c.cxx", 3, 3]] &&
                 diff[0].detail == "int\nk(int a)\n{\n  return a;\n}")
      tests = found.call("ctest", "LastTest.log")
      check.call("ctest: project frames, failed tests only, bad bytes", at.call(tests) == [
        ["test/unit/asan.cxx", 3, 39, "error", "unit_asan"], ["test/unit/tsan.cxx", 3, 22, "error", "unit_tsan"],
        ["test/unit/lsan.cxx", 3, 48, "error", "unit_lsan"], ["test/unit/fail.cxx", 12, nil, "error", "unit_fail"],
        [nil, nil, nil, "error", "unit_timeout"], [nil, nil, nil, "error", "unit_after_timeout"],
        [nil, nil, nil, "error", "unit_crash"]
      ] && tests[0].message == "AddressSanitizer: heap-buffer-overflow in read_past(int*)" &&
                 tests[3].message == "an empty key is rejected: expected true, got false" &&
                 tests[4].message == "unit_timeout failed" &&
                 tests[5].message == "a throwing case: threw std::runtime_error: boom" &&
                 !tests[0].detail.include?(root))

      many = (1..15).map { |n| Finding.new(tool: "t", level: "error", path: "core/b.cxx", line: n % 6 + 1, message: "m#{n}") }
      changed = Repo.new(root)
      changed.instance_variable_set(:@changed_lines, { "core/b.cxx" => Set[3] })
      lines, step = report(many, ["gone.log"], "t", changed, github: true)
      commands = lines.grep(/\A::error /)
      check.call("budget: 10 errors, changed line first, all in the summary",
                 commands.size == PER_LEVEL && commands.first.include?("line=3,") &&
                 step.include?("15 findings") && step.include?("5 not annotated"))
      gone = File.join(dir, "gone.log")
      check.call("missing input is a log line", collect("gcc", "t", [gone], repo) == [[], [gone]] &&
                 lines.first == "t: gone.log not found, nothing to report")
      odd = Finding.new(tool: "t", level: "warning", path: "core/a,b:c.cxx", line: 1, rule: "r", message: "50% a\nb | <c>")
      check.call("escaping", command(odd) == "::warning file=core/a%2Cb%3Ac.cxx,line=1,title=t%3A r::50%25 a%0Ab | <c>" &&
                             summary([odd], "t", 1, changed).include?("50% a b \\| &lt;c>"))
      check.call("no findings, no output", report([], [], "t", changed, github: true) == [[], nil])
      broken = File.join(dir, "broken.yaml")
      File.binwrite(broken, "Diagnostics:\n  - DiagnosticName: [unclosed\n")
      said = StringIO.new
      $stdout = said
      status = run("clang-tidy", "t", [broken], root: root, github: true)
      $stdout = STDOUT
      check.call("unreadable input exits 0", status.zero? && said.string.start_with?("t: cannot read "))
      nofs = Finding.new(tool: "t", level: "error", path: "core/b.cxx", message: "no line")
      check.call("a file without a line", rank([nofs], changed) == [nofs] && command(nofs).include?("file=core/b.cxx,title="))
      nowhere = Finding.new(tool: "t", level: "warning", message: "vendored")
      lines, step = report([nowhere], [], "t", changed, github: true)
      check.call("unpositioned: summary only", lines.empty? && step.include?("vendored"))
      plain = Repo.new(dir) # not a git checkout: no ranking, still reported
      check.call("ranking off without HEAD^", !plain.ranking? &&
                 summary(many, "t", 10, plain).include?("Not ranked"))
    end
    failures.each { |f| warn "self-test FAILED: #{f}" }
    puts "ok" if failures.empty?
    failures.empty?
  end
end

if $PROGRAM_NAME == __FILE__
  require "fileutils"
  require "stringio"
  exit(Findings.self_test ? 0 : 1) if ARGV == ["--self-test"]
  options = {}
  parser = OptionParser.new do |opts|
    opts.banner = "usage: #{$PROGRAM_NAME} --from FORMAT [--tool NAME] INPUT... | --self-test"
    opts.on("--from FORMAT", Findings::PARSERS.keys) { |v| options[:from] = v }
    opts.on("--tool NAME") { |v| options[:tool] = v }
  end
  begin
    parser.parse!(ARGV)
  rescue OptionParser::ParseError => e
    warn e.message, parser.banner
    exit 2
  end
  if options[:from].nil? || ARGV.empty?
    warn parser.banner
    exit 2
  end
  exit Findings.run(options[:from], options[:tool] || options[:from], ARGV)
end
