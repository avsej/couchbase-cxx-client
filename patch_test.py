with open("test/unit/management/search.cxx", "r") as f:
    content = f.read()

new_case = """void
score_fusion_with_sort_specs_is_rejected_before_sending([[maybe_unused]] context& ctx)
{
  couchbase::core::operations::search_request request{};
  request.scoring = couchbase::core::search_scoring_reciprocal_rank_fusion{};
  request.sort_specs.emplace_back("-\\_score");
  
  couchbase::core::operations::search_request::encoded_request_type encoded{};
  couchbase::core::operations::http_context http_ctx{};
  auto ec = request.encode_to(encoded, http_ctx);
  assert_error(ec, couchbase::errc::common::invalid_argument, "sort options with score fusion are rejected");
}

"""

if "score_fusion_with_sort_specs_is_rejected_before_sending" not in content:
    content = content.replace("void\nthe_scoring_parameters_survive_build", new_case + "void\nthe_scoring_parameters_survive_build")
    content = content.replace("{ CASE(only_the_fusion_modes_are_gated_by_the_capability) },", "{ CASE(only_the_fusion_modes_are_gated_by_the_capability) },\n      { CASE(score_fusion_with_sort_specs_is_rejected_before_sending) },")
    with open("test/unit/management/search.cxx", "w") as f:
        f.write(content)
