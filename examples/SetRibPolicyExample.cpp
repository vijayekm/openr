/**
 * This is an example code for setting RibPolicy in Open/R.
 *
 */

#include <gflags/gflags.h>

#include <folly/io/async/EventBase.h>
#include <openr/common/OpenrClient.h>
#include <openr/common/Util.h>
#include <openr/if/gen-cpp2/OpenrCtrl_types.h>

DEFINE_string(host, "::1", "Host to talk to");

DEFINE_int32(ttl_secs, 300, "Number of seconds the policy is alive for");
DEFINE_int32(default_weight, 1, "Weight for nexthops with no area");
DEFINE_int32(area0_weight, 2, "Area0 (default area in Open/R) weight");
DEFINE_string(prefixes, "", "Comma separated list of prefixes to apply policy");

using namespace openr;

int
main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  // Create list of prefixes for match
  std::vector<std::string> prefixStrs;
  folly::split(",", FLAGS_prefixes, prefixStrs, true);
  std::vector<thrift::IpPrefix> prefixes;
  for (auto const& prefixStr : prefixStrs) {
    LOG(INFO) << "Prefix - " << prefixStr;
    prefixes.emplace_back(toIpPrefix(prefixStr));
  }

  // Action weight
  thrift::RibRouteActionWeight actionWeight;
  actionWeight.default_weight = FLAGS_default_weight;
  actionWeight.area_to_weight.emplace(
      thrift::KvStore_constants::kDefaultArea(), FLAGS_area0_weight);

  // Create PolicyStatement
  thrift::RibPolicyStatement policyStatement;
  policyStatement.matcher.prefixes_ref() = prefixes;
  policyStatement.action.set_weight_ref() = actionWeight;

  // Create RibPolicy
  thrift::RibPolicy policy;
  policy.statements.emplace_back(policyStatement);
  policy.ttl_secs = FLAGS_ttl_secs;

  // Create OpenrClient and set policy
  LOG(INFO) << "Creating connection to host " << FLAGS_host;
  folly::EventBase evb;
  auto client = getOpenrCtrlPlainTextClient(evb, folly::IPAddress(FLAGS_host));
  client->sync_setRibPolicy(policy);
  LOG(INFO) << "Done setting policy";

  // Done setting policy (any exception will be thrown or printed)
  return 0;
}
