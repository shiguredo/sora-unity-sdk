#include <sora/sora_signaling.h>
#include <sora/sora_video_codec.h>

#include "sora_conf.json.h"
#include "sora_conf_internal.json.h"

namespace sora_unity_sdk {

sora::SoraSignalingConfig::ForwardingFilter ConvertToForwardingFilter(
    const sora_conf::internal::ForwardingFilter& filter);
sora::VideoCodecCapabilityConfig ConvertToVideoCodecCapabilityConfig(
    const sora_conf::internal::VideoCodecCapabilityConfig& config);
sora::VideoCodecCapabilityConfig ConvertToVideoCodecCapabilityConfigWithSession(
    const sora_conf::internal::VideoCodecCapabilityConfig& config);
sora_conf::internal::VideoCodecCapability ConvertToInternalVideoCodecCapability(
    const sora::VideoCodecCapability& capability);
sora::VideoCodecPreference ConvertToVideoCodecPreference(
    const sora_conf::internal::VideoCodecPreference& preference);

}  // namespace sora_unity_sdk