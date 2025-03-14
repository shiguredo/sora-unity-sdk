#include "converter.h"

// Boost
#include <boost/json.hpp>

// Sora
#include <sora/java_context.h>
#include <sora/vpl_session.h>

namespace sora_unity_sdk {

sora::SoraSignalingConfig::ForwardingFilter ConvertToForwardingFilter(
    const sora_conf::internal::ForwardingFilter& filter) {
  sora::SoraSignalingConfig::ForwardingFilter ff;

  if (filter.has_action()) {
    ff.action = filter.action;
  }
  if (filter.has_name()) {
    ff.name = filter.name;
  }
  if (filter.has_priority()) {
    ff.priority = filter.priority;
  }
  for (const auto& rs : filter.rules) {
    std::vector<sora::SoraSignalingConfig::ForwardingFilter::Rule> ffrs;
    for (const auto& r : rs.rules) {
      sora::SoraSignalingConfig::ForwardingFilter::Rule ffr;
      ffr.field = r.field;
      ffr.op = r.op;
      ffr.values = r.values;
      ffrs.push_back(ffr);
    }
    ff.rules.push_back(ffrs);
  }

  if (filter.has_version()) {
    ff.version = filter.version;
  }
  if (filter.has_metadata()) {
    boost::system::error_code ec;
    auto ffmd = boost::json::parse(filter.metadata, ec);
    if (ec) {
      RTC_LOG(LS_WARNING) << "Invalid JSON: forwarding_filter metadata="
                          << filter.metadata;
    } else {
      ff.metadata = ffmd;
    }
  }

  return ff;
}

sora::VideoCodecCapabilityConfig ConvertToVideoCodecCapabilityConfig(
    const sora_conf::internal::VideoCodecCapabilityConfig& config) {
  sora::VideoCodecCapabilityConfig vccc;
  if (config.has_openh264_path()) {
    vccc.openh264_path = config.openh264_path;
  }
  return vccc;
}
sora::VideoCodecCapabilityConfig ConvertToVideoCodecCapabilityConfigWithSession(
    const sora_conf::internal::VideoCodecCapabilityConfig& config) {
  sora::VideoCodecCapabilityConfig vccc =
      ConvertToVideoCodecCapabilityConfig(config);
  if (sora::CudaContext::CanCreate()) {
    vccc.cuda_context = sora::CudaContext::Create();
  }
  vccc.jni_env = sora::GetJNIEnv();
  return vccc;
}

sora::VideoCodecCapability ConvertToVideoCodecCapability(
    const sora_conf::internal::VideoCodecCapability& capability) {
  sora::VideoCodecCapability vcc;
  for (const auto& engine : capability.engines) {
    sora::VideoCodecCapability::Engine e(
        boost::json::value_to<sora::VideoCodecImplementation>(
            boost::json::value(engine.name)));
    for (const auto& codec : engine.codecs) {
      sora::VideoCodecCapability::Codec c(
          boost::json::value_to<webrtc::VideoCodecType>(
              boost::json::value(codec.type)),
          codec.encoder, codec.decoder);
      c.parameters =
          boost::json::value_to<sora::VideoCodecCapability::Parameters>(
              boost::json::parse(codec.parameters));
      e.codecs.push_back(c);
    }
    e.parameters =
        boost::json::value_to<sora::VideoCodecCapability::Parameters>(
            boost::json::parse(engine.parameters));
    vcc.engines.push_back(e);
  }
  return vcc;
}
sora_conf::internal::VideoCodecCapability ConvertToInternalVideoCodecCapability(
    const sora::VideoCodecCapability& capability) {
  sora_conf::internal::VideoCodecCapability vcc;
  for (const auto& engine : capability.engines) {
    sora_conf::internal::VideoCodecCapability::Engine e;
    e.name = boost::json::value_from(engine.name).as_string().c_str();
    e.parameters =
        boost::json::serialize(boost::json::value_from(engine.parameters));
    for (const auto& codec : engine.codecs) {
      sora_conf::internal::VideoCodecCapability::Codec c;
      c.type = boost::json::value_from(codec.type).as_string().c_str();
      c.encoder = codec.encoder;
      c.decoder = codec.decoder;
      c.parameters =
          boost::json::serialize(boost::json::value_from(codec.parameters));
      e.codecs.push_back(c);
    }
    vcc.engines.push_back(e);
  }
  return vcc;
}

sora::VideoCodecPreference ConvertToVideoCodecPreference(
    const sora_conf::internal::VideoCodecPreference& preference) {
  sora::VideoCodecPreference vcp;
  for (const auto& codec : preference.codecs) {
    sora::VideoCodecPreference::Codec c;
    c.type = boost::json::value_to<webrtc::VideoCodecType>(
        boost::json::value(codec.type));
    if (codec.has_encoder()) {
      c.encoder = boost::json::value_to<sora::VideoCodecImplementation>(
          boost::json::value(codec.encoder));
    }
    if (codec.has_decoder()) {
      c.decoder = boost::json::value_to<sora::VideoCodecImplementation>(
          boost::json::value(codec.decoder));
    }
    c.parameters =
        boost::json::value_to<sora::VideoCodecPreference::Parameters>(
            boost::json::parse(codec.parameters));
    vcp.codecs.push_back(c);
  }
  return vcp;
}

sora_conf::internal::VideoCodecPreference ConvertToInternalVideoCodecPreference(
    const sora::VideoCodecPreference& preference) {
  sora_conf::internal::VideoCodecPreference vcp;
  for (const auto& codec : preference.codecs) {
    sora_conf::internal::VideoCodecPreference::Codec c;
    c.type = boost::json::value_from(codec.type).as_string().c_str();
    if (codec.encoder.has_value()) {
      c.set_encoder(
          boost::json::value_from(codec.encoder.value()).as_string().c_str());
    }
    if (codec.decoder.has_value()) {
      c.set_decoder(
          boost::json::value_from(codec.decoder.value()).as_string().c_str());
    }
    c.parameters =
        boost::json::serialize(boost::json::value_from(codec.parameters));
    vcp.codecs.push_back(c);
  }
  return vcp;
}

}  // namespace sora_unity_sdk
