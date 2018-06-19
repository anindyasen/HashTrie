// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_IP_HPP_
#define USERPLANE_IP_HPP_
/**
 * IPv4/IPv6 Hdr related information
 *
 */


#include <cstdint>
#include <array>

#include "common.hpp"
#include "cache_handler.hpp"
#include "logger.hpp"

#include "netinet/in.h"

using std::uint8_t;
using std::uint32_t;
using std::uint16_t;

namespace ip {
const uint32_t kIPv6AddrLen8Bit = 16;
const uint32_t kIPv6AddrLen32Bit = 4;
const uint32_t kIPv6AddrLen64Bit = 2;
const uint32_t kIPv6AddrPrefixLen = 8;
typedef uint32_t IPv4Addr_t;
typedef std::array<uint8_t, kIPv6AddrLen8Bit> IPv6Addr8Bit_t;
typedef std::array<uint8_t, kIPv6AddrLen32Bit> IPv6Addr32Bit_t;
typedef std::array<uint8_t, kIPv6AddrLen64Bit> IPv6Addr64Bit_t;
typedef std::array<uint8_t, kIPv6AddrPrefixLen> IPv6Prefix;

union IPv6Addr_t {
    IPv6Addr8Bit_t  IPv6Addr8Bit;
    IPv6Addr32Bit_t IPv6Addr32Bit;
    IPv6Addr64Bit_t IPv6Addr64Bit;
};

union Addr {
    IPv4Addr_t   ip4;
    IPv6Addr_t   ip6;
};

struct IPV4AddrMask {
    ip::IPv4Addr_t addr;
    uint8_t        mask;
    bool operator==(const IPV4AddrMask& t) {
        return (addr == t.addr) && (mask == t.mask);
    }
};

struct IPV6AddrMask {
    ip::IPv6Addr_t addr;
    uint8_t        mask;
    bool operator==(const IPV6AddrMask& t) {
        return std::equal(addr.IPv6Addr8Bit.begin(),
                          addr.IPv6Addr8Bit.end(),
                          t.addr.IPv6Addr8Bit.begin()) && (mask == t.mask);
    }
};

enum class AddrType : uint8_t {
    kIpv4_t  = 4,
    kIpv6_t  = 6,
};

struct IPAddr {
    AddrType addr_type;
    Addr     addr;
};

template<typename T>
always_inline
bool _CheckForMulticast(const T &Addr);

template<>
always_inline
bool _CheckForMulticast(const IPv4Addr_t &Addr) {
    return(Addr & htonl(0xf0000000)) == htonl(0xe0000000);
}
template<>
always_inline
bool _CheckForMulticast(const IPv6Addr_t &Addr) {
    uint8_t Prefix8 = Addr.IPv6Addr8Bit.at(0);
    if (Prefix8 == 0xff) {
        return true;
    } else {
        return false;
    }
}

const uint8_t  kIPv4IHLMask   = 0x0f;
const uint8_t  kIPv4VerMask   = ~kIPv4IHLMask;
const uint16_t kIPv4FragOfsMask = 0x1fff;
const uint16_t kIPv4FlagsMask = ~(kIPv4FragOfsMask);
const uint16_t kIPv4MFFlag    = (1<<13);
const uint16_t kIPv4DFFlag    = (1<<14);
const uint8_t  kIPv4NoOptsHdrSize = 20;
const uint8_t  kIPv4MaxHdrSize = 60;
const uint8_t  kIPv4FragOfsAlignByte = 8;
const uint8_t  kIPv4FragOfsByte = 8;

struct IPv4Hdr {
 public:
    uint8_t  get_ip_version() { return ((ver_ihl & kIPv4VerMask) >> 4); }
    void set_ip_version(uint8_t ver) { ver_ihl = (ver_ihl & ~kIPv4VerMask) | (ver << 4); }

    uint16_t  get_hlen_bytes() { return ((ver_ihl & (kIPv4IHLMask)) << 2); }
    void set_hlen_bytes(uint16_t bytes) { ver_ihl = ((ver_ihl & ~kIPv4IHLMask) | (bytes >> 2)); }

    uint8_t  get_tos() { return tos;}
    void set_tos(uint8_t t) { tos = t; }

    uint16_t get_packet_len() { return ntohs(len); }
    void set_packet_len(uint16_t l) { len = htons(l); }

    uint16_t get_id() { return ntohs(id); }
    void set_id(uint16_t identity) { id = htons(identity); }

    uint16_t get_flags() { return (ntohs(frag_ofs) & kIPv4FlagsMask); }
    void set_flags(uint16_t flag) { frag_ofs = htons(ntohs(frag_ofs) | (flag & kIPv4FlagsMask)); }

    bool is_mf_set() { return !!(ntohs(frag_ofs) & kIPv4MFFlag); }

    bool is_df_set() { return !!(ntohs(frag_ofs) & kIPv4DFFlag); }

    always_inline
    uint16_t get_frag_ofs_bytes() {
        return ((ntohs(frag_ofs) & kIPv4FragOfsMask) * kIPv4FragOfsByte );
    }

    always_inline
    void set_frag_ofs_bytes(uint16_t bytes) {
        frag_ofs = htons((ntohs(frag_ofs) & ~kIPv4FragOfsMask) |  (bytes / kIPv4FragOfsByte));
    }

    uint8_t  get_ttl() { return ttl; }
    void set_ttl(uint8_t time_to_live) { ttl = time_to_live; }

    uint8_t  get_transport_proto() { return next_proto_id; }
    void set_transport_proto(uint8_t proto) { next_proto_id = proto; }

    uint16_t get_hdr_csum() { return ntohs(hdr_csum); }
    void set_hdr_csum(uint16_t sum) { hdr_csum = htons(sum); }

    uint32_t get_src_addr() { return ntohl(src_addr); }
    void set_src_addr(uint32_t addr) { src_addr = htonl(addr); }

    uint32_t get_dst_addr() { return ntohl(dst_addr); }
    void set_dst_addr(uint32_t addr) { dst_addr = htonl(addr); }

 private:
    uint8_t  ver_ihl;           // version and header length
    uint8_t  tos;               // type of service
    uint16_t len;               // length of packet
    uint16_t id;                // packet ID
    uint16_t frag_ofs;          // fragmentation offset & flags
    uint8_t  ttl;               // time to live
    uint8_t  next_proto_id;     // protocol ID
    uint16_t hdr_csum;          // header checksum
    uint32_t src_addr;          // source address
    uint32_t dst_addr;          // destination address
}__attribute__((__packed__));
}  //  namespace ip

// define IPV4 header and related flags and related functions
// define IPV6 header and related flags and related functions
#endif  // USERPLANE_IP_HPP_
