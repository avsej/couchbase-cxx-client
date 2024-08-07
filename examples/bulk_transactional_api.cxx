/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2021-Present Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "couchbase/logger.hxx"
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>
#include <couchbase/transactions.hxx>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <tao/json/to_string.hpp>

#include <fstream>
#include <ios>
#include <iostream>
#include <math.h>
#include <string>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <future>
#include <random>
#include <set>
#include <system_error>
#include <time.h>

using namespace std;

#ifdef __linux__
void
mem_usage(double& vm_usage, double& resident_set)
{
  vm_usage = 0.0;
  resident_set = 0.0;
  ifstream stat_stream("/proc/self/stat", ios_base::in); // get info from proc directory
  // create some variables to get info
  string pid, comm, state, ppid, pgrp, session, tty_nr;
  string tpgid, flags, minflt, cminflt, majflt, cmajflt;
  string utime, stime, cutime, cstime, priority, nice;
  string O, itrealvalue, starttime;
  unsigned long vsize;
  long rss;
  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags >>
    minflt >> cminflt >> majflt >> cmajflt >> utime >> stime >> cutime >> cstime >> priority >>
    nice >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
  stat_stream.close();
  long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
  vm_usage = vsize / 1024.0;
  resident_set = rss * page_size_kb;
}
#else
#include <mach/mach.h>

void
mem_usage(double& vm_usage, double& resident_set)
{
  vm_usage = 0.0;
  resident_set = 0.0;

  mach_task_basic_info info{};
  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

  if (task_info(
        mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &infoCount) ==
      KERN_SUCCESS) {
    vm_usage = static_cast<double>(info.virtual_size) / 1024.0 / 1024.0;
    resident_set = static_cast<double>(info.resident_size) / 1024.0;
  }
}
#endif

std::string
GenerateString()
{
  static const std::vector<char> characters{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                                             'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                                             'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };
  static int numOfChars = characters.size();
  std::string output;
  for (int i = 0; i < numOfChars * 4; ++i) {
    output += characters[rand() % numOfChars];
  }

  return output;
}

struct program_arguments {
  std::string connection_string{ "couchbase://now.its.hidden.com" };
  std::string username{ "Administrator" };
  std::string password{ "password" };
  std::string bucket_name{ "default" };
  std::string scope_name{ couchbase::scope::default_name };
  std::string collection_name{ couchbase::collection::default_name };
  std::size_t number_of_operations{ 1'000 };
  std::size_t document_body_size{ 1'024 };
  std::chrono::seconds transaction_timeout{ 120 };

  static auto load_from_environment() -> program_arguments
  {
    program_arguments arguments;
    if (const auto* val = getenv("CB_CONNECTION_STRING"); val != nullptr && val[0] != '\0') {
      arguments.connection_string = val;
    }
    if (const auto* val = getenv("CB_USERNAME"); val != nullptr && val[0] != '\0') {
      arguments.username = val;
    }
    if (const auto* val = getenv("CB_PASSWORD"); val != nullptr && val[0] != '\0') {
      arguments.password = val;
    }
    if (const auto* val = getenv("CB_BUCKET_NAME"); val != nullptr && val[0] != '\0') {
      arguments.bucket_name = val;
    }
    if (const auto* val = getenv("CB_SCOPE_NAME"); val != nullptr && val[0] != '\0') {
      arguments.scope_name = val;
    }
    if (const auto* val = getenv("CB_COLLECTION_NAME"); val != nullptr && val[0] != '\0') {
      arguments.collection_name = val;
    }
    if (const auto* val = getenv("CB_NUMBER_OF_OPERATIONS"); val != nullptr && val[0] != '\0') {
      char* end = nullptr;
      auto int_val = std::strtoul(val, &end, 10);
      if (end != val) {
        arguments.number_of_operations = int_val;
      }
    }
    if (const auto* val = getenv("CB_DOCUMENT_BODY_SIZE"); val != nullptr && val[0] != '\0') {
      char* end = nullptr;
      auto int_val = std::strtoul(val, &end, 10);
      if (end != val) {
        arguments.document_body_size = int_val;
      }
    }
    if (const auto* val = getenv("CB_TRANSACTION_TIMEOUT"); val != nullptr && val[0] != '\0') {
      char* end = nullptr;
      auto int_val = std::strtoul(val, &end, 10);
      if (end != val) {
        arguments.transaction_timeout = std::chrono::seconds{ int_val };
      }
    }
    return arguments;
  }
};

std::string
random_text(std::size_t length)
{
  std::string alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  static thread_local std::mt19937_64 gen{ std::random_device()() };
  std::uniform_int_distribution<std::size_t> dis(0, alphabet.size() - 1);
  std::string text(length, '-');
  for (std::size_t i = 0; i < length; ++i) {
    text[i] = alphabet[dis(gen)];
  }
  return text;
}

auto
generate_document(std::size_t document_body_size) -> tao::json::value
{
  return {
    { "size", document_body_size },
    { "text", random_text(document_body_size) },
  };
}

void
run_workload_sequential(const std::shared_ptr<couchbase::transactions::transactions>& transactions,
                        const couchbase::collection& collection,
                        const program_arguments& arguments)
{
  if (arguments.number_of_operations <= 0) {
    return;
  }

  const std::string document_id_prefix{ "tx_sequential" };
  std::vector<std::string> document_ids;
  document_ids.reserve(arguments.number_of_operations);
  for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
    document_ids.emplace_back(fmt::format("{}_{:06d}", document_id_prefix, i));
  }

  {
    using remove_result = std::future<std::pair<couchbase::error, couchbase::mutation_result>>;

    std::map<std::string, std::size_t> errors;
    std::vector<remove_result> results;
    results.reserve(arguments.number_of_operations);

    auto cleanup_start = std::chrono::system_clock::now();
    for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
      results.emplace_back(collection.remove(document_ids[i], {}));
    }
    for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
      auto [err, result] = results[i].get();
      if (err.ec()) {
        errors[err.ec().message()]++;
      }
    }
    auto cleanup_end = std::chrono::system_clock::now();

    if (!errors.empty()) {
      fmt::print("\tSome operations completed with errors:\n");
      for (auto [error, hits] : errors) {
        fmt::print("\t{}: {}\n", error, hits);
      }
    }
  }

  const auto document = generate_document(arguments.document_body_size);

  auto start = std::chrono::system_clock::now();

  {
    std::map<std::string, std::size_t> errors;

    auto exec_start = std::chrono::system_clock::now();
    auto [err, result] = transactions->run(
      [&collection, &document_ids, &document, &arguments, &errors](
        std::shared_ptr<couchbase::transactions::attempt_context> attempt) -> couchbase::error {
        for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
          auto [err, res] = attempt->insert(collection, document_ids[i], document);
          if (err.ec()) {
            errors[err.ec().message()]++;
          }
          fmt::print("\rexecute insert: {}", i);
          fflush(stdout);
        }
        return {};
      });
    auto exec_end = std::chrono::system_clock::now();

    if (err.ec()) {
      fmt::print("\tTransaction completed with error {}, cause={}\n",
                 err.ec().message(),
                 err.cause().has_value() ? err.cause().value().ec().message() : "");
      if (err.ec() == couchbase::errc::transaction::expired) {
        fmt::print("\tINFO: Try to increase CB_TRANSACTION_TIMEOUT, current value is {} seconds\n",
                   arguments.transaction_timeout);
      }
    }
    if (!errors.empty()) {
      fmt::print("\tSome operations completed with errors:\n");
      for (auto [error, hits] : errors) {
        fmt::print("\t{}: {}\n", error, hits);
      }
    }
  }

  {
    std::map<std::string, std::size_t> errors;

    auto exec_start = std::chrono::system_clock::now();
    auto [err, result] = transactions->run(
      [&collection, &document_ids, &arguments, &errors](
        std::shared_ptr<couchbase::transactions::attempt_context> attempt) -> couchbase::error {
        for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
          auto [err, res] = attempt->get(collection, document_ids[i]);
          if (err.ec()) {
            errors[err.ec().message()]++;
          }
          fmt::print("\rexecute get: {}", i);
          fflush(stdout);
        }
        return {};
      });
    auto exec_end = std::chrono::system_clock::now();

    if (err.ec()) {
      fmt::print("\tTransaction completed with error {}, cause={}\n",
                 err.ec().message(),
                 err.cause().has_value() ? err.cause().value().ec().message() : "");
      if (err.ec() == couchbase::errc::transaction::expired) {
        fmt::print("\tINFO: Try to increase CB_TRANSACTION_TIMEOUT, current value is {} seconds\n",
                   arguments.transaction_timeout);
      }
    }

    if (!errors.empty()) {
      fmt::print("\tSome operations completed with errors:\n");
      for (auto [error, hits] : errors) {
        fmt::print("\t{}: {}\n", error, hits);
      }
    }
  }

  auto end = std::chrono::system_clock::now();
}

void
run_workload_bulk(const std::shared_ptr<couchbase::transactions::transactions>& transactions,
                  const couchbase::collection& collection,
                  const program_arguments& arguments)
{
  if (arguments.number_of_operations <= 0) {
    return;
  }

  const std::string document_id_prefix{ "tx_bulk" };
  std::vector<std::string> document_ids;
  document_ids.reserve(arguments.number_of_operations);
  for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
    document_ids.emplace_back(fmt::format("{}_{:06d}", document_id_prefix, i));
  }

  {
    using remove_result = std::future<std::pair<couchbase::error, couchbase::mutation_result>>;

    std::map<std::string, std::size_t> errors;
    std::vector<remove_result> results;
    results.reserve(arguments.number_of_operations);

    auto cleanup_start = std::chrono::system_clock::now();
    for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
      results.emplace_back(collection.remove(document_ids[i], {}));
    }
    for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
      auto [err, result] = results[i].get();
      if (err.ec()) {
        errors[err.ec().message()]++;
      }
    }
    auto cleanup_end = std::chrono::system_clock::now();

    if (!errors.empty()) {
      fmt::print("\tSome operations completed with errors:\n");
      for (auto [error, hits] : errors) {
        fmt::print("\t{}: {}\n", error, hits);
      }
    }
  }

  const auto document = generate_document(arguments.document_body_size);

  auto start = std::chrono::system_clock::now();

  {
    std::map<std::string, std::size_t> errors;

    auto tx_promise = std::make_shared<
      std::promise<std::pair<couchbase::error, couchbase::transactions::transaction_result>>>();
    auto tx_future = tx_promise->get_future();

    auto schedule_start = std::chrono::system_clock::now();
    transactions->run(
      [&collection, &document_ids, &document, &arguments, &errors](
        std::shared_ptr<couchbase::transactions::async_attempt_context> attempt)
        -> couchbase::error {
        for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
          attempt->insert(collection, document_ids[i], document, [&errors](auto ctx, auto) {
            if (ctx.ec()) {
              errors[ctx.ec().message()]++;
            }
          });
        }
        return {};
      },
      [tx_promise](auto err, auto result) {
        tx_promise->set_value({ err, result });
      });

    auto schedule_end = std::chrono::system_clock::now();

    auto exec_start = std::chrono::system_clock::now();
    auto [err, result] = tx_future.get();
    auto exec_end = std::chrono::system_clock::now();

    if (err.ec()) {
      fmt::print("\tTransaction completed with error {}, cause={}\n",
                 err.ec().message(),
                 err.cause().has_value() ? err.cause().value().ec().message() : "");
      if (err.ec() == couchbase::errc::transaction::expired) {
        fmt::print("\tINFO: Try to increase CB_TRANSACTION_TIMEOUT, current value is {} seconds\n",
                   arguments.transaction_timeout);
      }
    }

    if (!errors.empty()) {
      fmt::print("\tSome operations completed with errors:\n");
      for (auto [error, hits] : errors) {
        fmt::print("\t{}: {}\n", error, hits);
      }
    }
  }

  {
    std::map<std::string, std::size_t> errors;

    auto tx_promise = std::make_shared<
      std::promise<std::pair<couchbase::error, couchbase::transactions::transaction_result>>>();
    auto tx_future = tx_promise->get_future();

    auto schedule_start = std::chrono::system_clock::now();
    transactions->run(
      [&collection, &document_ids, &arguments, &errors](
        const std::shared_ptr<couchbase::transactions::async_attempt_context>& attempt)
        -> couchbase::error {
        for (std::size_t i = 0; i < arguments.number_of_operations; ++i) {
          attempt->get(collection, document_ids[i], [&errors](auto ctx, auto) {
            if (ctx.ec()) {
              errors[ctx.ec().message()]++;
            }
          });
        }
        return {};
      },
      [tx_promise](auto err, auto result) {
        tx_promise->set_value({ err, result });
      });

    auto schedule_end = std::chrono::system_clock::now();

    auto exec_start = std::chrono::system_clock::now();
    auto [err, result] = tx_future.get();
    auto exec_end = std::chrono::system_clock::now();

    if (err.ec()) {
      fmt::print("\tTransaction completed with error {}, cause={}\n",
                 err.ec().message(),
                 err.cause().has_value() ? err.cause().value().ec().message() : "");
      if (err.ec() == couchbase::errc::transaction::expired) {
        fmt::print("\tINFO: Try to increase CB_TRANSACTION_TIMEOUT, current value is {} seconds\n",
                   arguments.transaction_timeout);
      }
    }

    if (err.ec() == couchbase::errc::transaction::expired) {
      fmt::print("\tINFO: Try to increase CB_TRANSACTION_TIMEOUT, current value is {} seconds\n",
                 arguments.transaction_timeout);
    }
    if (!errors.empty()) {
      fmt::print("\tSome operations completed with errors:\n");
      for (auto [error, hits] : errors) {
        fmt::print("\t{}: {}\n", error, hits);
      }
    }
  }
  auto end = std::chrono::system_clock::now();
}

void
bulk_transactional_api_example()
{
  couchbase::logger::initialize_console_logger();
  couchbase::logger::set_level(couchbase::logger::log_level::error);

  auto arguments = program_arguments::load_from_environment();

  auto options = couchbase::cluster_options(arguments.username, arguments.password);
  options.apply_profile("wan_development");
  options.transactions().timeout(arguments.transaction_timeout);
  auto [connect_err, cluster] =
    couchbase::cluster::connect(arguments.connection_string, options).get();
  if (connect_err) {
    fmt::print("Unable to connect to cluster at \"{}\", error: {}\n",
               arguments.connection_string,
               connect_err);
  } else {
    auto transactions = cluster.transactions();
    auto collection = cluster.bucket(arguments.bucket_name)
                        .scope(arguments.scope_name)
                        .collection(arguments.collection_name);

    // run_workload_sequential(transactions, collection, arguments);
    /* for (int i = 1; i < 10; ++i) { */
    run_workload_bulk(transactions, collection, arguments);
    /* if (i % 10 == 0) { */
    double vm{};
    double rss{};
    mem_usage(vm, rss);
    cout << "bulk_transactional_api_example iter: "
         /* << i  */
         << " Virtual Memory: " << vm << " Resident set size: " << rss << endl;
    /* } */
    /* } */
  }
  cluster.close().get();
}

auto
main() -> int
{
  bulk_transactional_api_example();
}
