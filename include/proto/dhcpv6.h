/*
 * Copyright (c) 2026 Rozhuk Ivan <rozhuk.im@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#ifndef __DHCP6_MESSAGE_H__
#define __DHCP6_MESSAGE_H__


#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#ifndef nitems
#	define nitems(__X)	(sizeof(__X) / sizeof(__X[0]))
#endif


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
// https://www.iana.org/assignments/dhcpv6-parameters/dhcpv6-parameters.xhtml
/* RFC 3319 DHCPv6 Options for SIP Servers */
/* RFC 3646 DNS Configuration Options for DHCPv6 */
/* RFC 3898 NIS Configuration Options for DHCPv6 */
/* RFC 4075 SNTP Configuration Option for DHCPv6 */
/* RFC 4280 DHCP Options for BMCS */
/* RFC 4580 Relay Agent Subscriber-ID */
/* RFC 4649 Relay Agent Remote-ID */
/* RFC 4704 The DHCPv6 Client FQDN Option */
/* RFC 4776 DHCP Civic */
/* RFC 4833 Timezone Options for DHCP */
/* RFC 4994 Relay Agent ERO */
/* RFC 5007 DHCPv6 Leasequery */
/* RFC 5192 PAA DHCP Options */
/* RFC 5223 DHCP-Based LoST Discovery */
/* RFC 5417 CAPWAP AC DHCP Option */
/* RFC 5460 DHCPv6 Bulk Leasequery */
/* RFC 5678 Mobility Services for DCHP Options */
/* RFC 5908 NTP Server Option for DHCPv6 */
/* RFC 5970 DHCPv6 Options for Network Boot */
/* RFC 5986 LIS Discovery */
/* RFC 6011 SIP UA Configuration */
/* RFC 6153 ANDSF DHCP Options */
/* RFC 6225 DHCP Options for Coordinate LCI */
/* RFC 6334 DS-Lite DHCPv6 Option */
/* RFC 6422 Relay-Supplied DHCP Options */
/* RFC 6440 The ERP Local Domain Name DHCPv6 Option */
/* RFC 6603 PD Exclude Option */
/* RFC 6607 Virtual Subnet Selection Options */
/* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
/* RFC 6731 RDNSS Selection for MIF Nodes */
/* RFC 6784 Kerberos Options for DHCPv6 */
/* RFC 6939 DHCPv6 Client Link-Layer Address Option */
/* RFC 6977 DHCPv6 Relay-Triggered Reconfiguration */
/* RFC 7037 DHCPv6 RADIUS Option */
/* RFC 7078 DHCPv6 Address Selection Policy Opt */
/* RFC 7291 PCP DHCP Options */
/* RFC 7341 DHCPv4 over DHCPv6 */
/* RFC 7598 DHCPv6 for Softwire 46 CEs */
/* RFC 7600 Stateless IPv4 Residual Deployment (4rd) */
/* RFC 7653 DHCPv6 Active Leasequery */
/* RFC 7774 MPL Configuration for DHCPv6 */
/* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
/* RFC 8026 OPTION_S46_PRIORITY DHCPv6 Option */
/* RFC 8115 IPv4/IPv6 Multicast Prefixes Option */
/* RFC 8156 DHCPv6 Failover Protocol */
/* RFC 8357 DHCP Relay Source Port */
/* RFC 8520 Manufacturer Usage Descriptions */
/* RFC 8539 Softwire Provisioning with DHCP 4o6 */
/* RFC 8572 Secure Zero Touch Provisioning (SZTP) */
/* RFC 8910 Captive-Portal Identification in DHCP and Router Advertisements (RAs) */
/* RFC 8947 Link-Layer Address Assignment Mechanism for DHCPv6 */
/* RFC 8948 Structured Local Address Plan (SLAP) Quadrant Selection Option for DHCPv6 */
/* RFC 8973 DDoS Open Threat Signaling (DOTS) Agent Discovery */
/* RFC 9463 DHCP and Router Advertisement Options for the Discovery of Network-designated Resolvers (DNR) */
/* RFC 9527 DHCPv6 Options for the Homenet Naming Authority */
/* RFC 9686 Registering Self-Generated IPv6 Addresses Using DHCPv6 */
/* RFC 9915 Dynamic Host Configuration Protocol for IPv6 (DHCPv6) */
//
// http://www.iana.org/numbers.htm
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////


#define DHCP6_SERVERS		"ff05::1:3"
#define DHCP6_RLY_AGNTS_SERVERS	"ff02::1:2"

#define DHCP6_SRV_PORT		547
#define DHCP6_CLI_PORT		546
#define DHCP6_MIN_PACKET_LENGTH	300 /* RFC 1542 2.1. */


typedef struct dhcp6_header_s {
	uint8_t		msg_type;	/* Identifies the DHCP message type. */
	uint8_t		xid[3];		/* The transaction ID for this message exchange. */
	/* Optional parameters field. */
} __attribute__((__packed__)) dhcp6_hdr_t, *dhcp6_hdr_p;

/* Message  type. */
#define DHCP6_HDR_MT_SOLICIT		1
#define DHCP6_HDR_MT_ADVERTISE		2
#define DHCP6_HDR_MT_REQUEST		3
#define DHCP6_HDR_MT_CONFIRM		4
#define DHCP6_HDR_MT_RENEW		5
#define DHCP6_HDR_MT_REBIND		6
#define DHCP6_HDR_MT_REPLY		7
#define DHCP6_HDR_MT_RELEASE		8
#define DHCP6_HDR_MT_DECLINE		9
#define DHCP6_HDR_MT_RECONFIGURE	10
#define DHCP6_HDR_MT_INFO_REQ		11
#define DHCP6_HDR_MT_RELAY_FORW		12
#define DHCP6_HDR_MT_RELAY_REPL		13
#define DHCP6_HDR_MT_LEASEQUERY		14
#define DHCP6_HDR_MT_LEASEQUERY_REPLY	15
#define DHCP6_HDR_MT_LEASEQUERY_DONE	16
#define DHCP6_HDR_MT_LEASEQUERY_DATA	17
#define DHCP6_HDR_MT_RECONFIGURE_REQUEST 18
#define DHCP6_HDR_MT_RECONFIGURE_REPLY	19
#define DHCP6_HDR_MT_DHCPV4_QUERY	20
#define DHCP6_HDR_MT_DHCPV4_RESPONSE	21
#define DHCP6_HDR_MT_ACTIVELEASEQUERY	22
#define DHCP6_HDR_MT_STARTTLS		23
#define DHCP6_HDR_MT_BNDUPD		24
#define DHCP6_HDR_MT_BNDREPLY		25
#define DHCP6_HDR_MT_POOLREQ		26
#define DHCP6_HDR_MT_POOLRESP		27
#define DHCP6_HDR_MT_UPDREQ		28
#define DHCP6_HDR_MT_UPDREQALL		29
#define DHCP6_HDR_MT_UPDDONE		30
#define DHCP6_HDR_MT_CONNECT		31
#define DHCP6_HDR_MT_CONNECTREPLY	32
#define DHCP6_HDR_MT_DISCONNECT		33
#define DHCP6_HDR_MT_STATE		34
#define DHCP6_HDR_MT_CONTACT		35
#define DHCP6_HDR_MT_ADDR_REG_INFORM	36
#define DHCP6_HDR_MT_ADDR_REG_REPLY	37

static const char *dhcp6_header_msg_type[] = {
/*   0 */	NULL,
/*   1 */	"SOLICIT",
/*   2 */	"ADVERTISE",
/*   3 */	"REQUEST",
/*   4 */	"CONFIRM",
/*   5 */	"RENEW",
/*   6 */	"REBIND",
/*   7 */	"REPLY",
/*   8 */	"RELEASE",
/*   9 */	"DECLINE",
/*  10 */	"RECONFIGURE",
/*  11 */	"INFORMATION-REQUEST",
/*  12 */	"RELAY-FORW",
/*  13 */	"RELAY-REPL",
/*  14 */	"LEASEQUERY",
/*  15 */	"LEASEQUERY-REPLY",
/*  16 */	"LEASEQUERY-DONE",
/*  17 */	"LEASEQUERY-DATA",
/*  18 */	"RECONFIGURE-REQUEST",
/*  19 */	"RECONFIGURE-REPLY",
/*  20 */	"DHCPV4-QUERY",
/*  21 */	"DHCPV4-RESPONSE",
/*  22 */	"ACTIVELEASEQUERY",
/*  23 */	"STARTTLS",
/*  24 */	"BNDUPD",
/*  25 */	"BNDREPLY",
/*  26 */	"POOLREQ",
/*  27 */	"POOLRESP",
/*  28 */	"UPDREQ",
/*  29 */	"UPDREQALL",
/*  30 */	"UPDDONE",
/*  31 */	"CONNECT",
/*  32 */	"CONNECTREPLY",
/*  33 */	"DISCONNECT",
/*  34 */	"STATE",
/*  35 */	"CONTACT",
/*  36 */	"ADDR-REG-INFORM",
/*  37 */	"ADDR-REG-REPLY",
};


typedef struct dhcp6_relay_agent_message_header_s {
	uint8_t		msg_type;		/* Identifies the DHCP message type. */
	uint8_t		hop_count;		/* Number of relay agents that have already relayed this message. */
	uint8_t		link_address[16];	/* An address that may be used by the server to identify the link on which the client is located. */
	uint8_t		peer_address[16];	/* The address of the client or relay agent from which the message to be relayed was received. */
	/* Optional parameters field. */
} __attribute__((__packed__)) dhcp6_relay_msg_t, *dhcp6_relay_msg_p;


typedef struct dhcp6_option_header_s {
	uint16_t	code;	/* DHCP6_OPT_* (Assigned by IANA.). */
	uint16_t	len;	/* Size (in octets) of OPTION-DATA. */
	/* data: varies per OPTION-CODE. */
} __attribute__((__packed__)) dhcp6_opt_hdr_t, *dhcp6_opt_hdr_p;


/* DHCP Standard Options. */
// RESERVED				0
#define DHCP6_OPT_CLIENTID		1
#define DHCP6_OPT_SERVERID		2
	#define DHCP6_DUID_TYPE_LLT		1
	#define DHCP6_DUID_TYPE_EN		2
	#define DHCP6_DUID_TYPE_LL		3
	#define DHCP6_DUID_TYPE_UUID		4
#define DHCP6_OPT_IA_NA			3
#define DHCP6_OPT_IA_TA			4
#define DHCP6_OPT_IAADDR		5
#define DHCP6_OPT_ORO			6
#define DHCP6_OPT_PREFERENCE		7
#define DHCP6_OPT_ELAPSED_TIME		8
#define DHCP6_OPT_RELAY_MSG		9
// UNASSIGNED				10
#define DHCP6_OPT_AUTH			11
#define DHCP6_OPT_UNICAST		12
#define DHCP6_OPT_STATUS_CODE		13
#define DHCP6_OPT_RAPID_COMMIT		14
#define DHCP6_OPT_USER_CLASS		15
#define DHCP6_OPT_VENDOR_CLASS		16
#define DHCP6_OPT_VENDOR_OPTS		17
#define DHCP6_OPT_INTERFACE_ID		18
#define DHCP6_OPT_RECONF_MSG		19
#define DHCP6_OPT_RECONF_ACCEP		20
#define DHCP6_OPT_SIP_SERVER_D		21
#define DHCP6_OPT_SIP_SERVER_A		22
#define DHCP6_OPT_DNS_SERVERS		23
#define DHCP6_OPT_DOMAIN_LIST		24
#define DHCP6_OPT_IA_PD			25
#define DHCP6_OPT_IAPREFIX		26
#define DHCP6_OPT_NIS_SERVERS		27
#define DHCP6_OPT_NISP_SERVERS		28
#define DHCP6_OPT_NIS_DOMAIN_NAME	29
#define DHCP6_OPT_NISP_DOMAIN_NAME	30
#define DHCP6_OPT_SNTP_SERVERS		31
#define DHCP6_OPT_INFORMATION_REFRESH_TIME 32
#define DHCP6_OPT_BCMCS_SERVER_D	33
#define DHCP6_OPT_BCMCS_SERVER_A	34
// UNASSIGNED				35
#define DHCP6_OPT_GEOCONF_CIVIC		36
#define DHCP6_OPT_REMOTE_ID		37
#define DHCP6_OPT_SUBSCRIBER_ID		38
#define DHCP6_OPT_CLIENT_FQDN		39
	#define DHCP6_OPT_CLIENT_FQDN_F_S	(((uint8_t)1) << 0)
	#define DHCP6_OPT_CLIENT_FQDN_F_O	(((uint8_t)1) << 1)
	#define DHCP6_OPT_CLIENT_FQDN_F_N	(((uint8_t)1) << 2)
#define DHCP6_OPT_PANA_AGENT		40
#define DHCP6_OPT_NEW_POSIX_TIMEZONE	41
#define DHCP6_OPT_NEW_TZDB_TIMEZONE	42
#define DHCP6_OPT_ERO			43
#define DHCP6_OPT_LQ_QUERY		44
#define DHCP6_OPT_CLIENT_DATA		45
#define DHCP6_OPT_CLT_TIME		46
#define DHCP6_OPT_LQ_RELAY_DATA		47
#define DHCP6_OPT_LQ_CLIENT_LINK	48
#define DHCP6_OPT_MIP6_HNIDF		49
#define DHCP6_OPT_MIP6_VDINF		50
#define DHCP6_OPT_V6_LOST		51
#define DHCP6_OPT_CAPWAP_AC_V6		52
#define DHCP6_OPT_RELAY_ID		53
#define DHCP6_OPT_IPV6_ADDRESS_MOS	54
#define DHCP6_OPT_IPV6_FQDN_MOS		55
#define DHCP6_OPT_NTP_SERVER		56
#define DHCP6_OPT_V6_ACCESS_DOMAIN	57
#define DHCP6_OPT_SIP_UA_CS_LIST	58
#define DHCP6_OPT_BOOTFILE_URL		59
#define DHCP6_OPT_BOOTFILE_PARAM	60
#define DHCP6_OPT_CLIENT_ARCH_TYPE	61
#define DHCP6_OPT_NII			62
#define DHCP6_OPT_GEOLOCATION		63
#define DHCP6_OPT_AFTR_NAME		64
#define DHCP6_OPT_ERP_LOCAL_DOMAIN_NAME	65
#define DHCP6_OPT_RSOO			66
#define DHCP6_OPT_PD_EXCLUDE		67
#define DHCP6_OPT_VSS			68
#define DHCP6_OPT_MIP6_IDINF		69
#define DHCP6_OPT_MIP6_UDINF		70
#define DHCP6_OPT_MIP6_HNP		71
#define DHCP6_OPT_MIP6_HAA		72
#define DHCP6_OPT_MIP6_HAF		73
#define DHCP6_OPT_RDNSS_SELECTION	74
#define DHCP6_OPT_KRB_PRINCIPAL_NAME	75
#define DHCP6_OPT_KRB_REALM_NAME	76
#define DHCP6_OPT_KRB_DEFAULT_REALM_NAME 77
#define DHCP6_OPT_KRB_KDC		78
#define DHCP6_OPT_CLIENT_LINKLAYER_ADDR	79
#define DHCP6_OPT_LINK_ADDRESS		80
#define DHCP6_OPT_RADIUS		81
#define DHCP6_OPT_SOL_MAX_RT		82
#define DHCP6_OPT_INF_MAX_RT		83
#define DHCP6_OPT_ADDRSEL		84
#define DHCP6_OPT_ADDRSEL_TABLE		85
#define DHCP6_OPT_V6_PCP_SERVER		86
#define DHCP6_OPT_DHCPV4_MSG		87
#define DHCP6_OPT_DHCP4_O_DHCP6_SERVER	88
#define DHCP6_OPT_S46_RULE		89
#define DHCP6_OPT_S46_BR		90
#define DHCP6_OPT_S46_DMR		91
#define DHCP6_OPT_S46_V4V6BIND		92
#define DHCP6_OPT_S46_PORTPARAMS	93
#define DHCP6_OPT_S46_CONT_MAPE		94
#define DHCP6_OPT_S46_CONT_MAPT		95
#define DHCP6_OPT_S46_CONT_LW		96
#define DHCP6_OPT_4RD			97
#define DHCP6_OPT_4RD_MAP_RULE		98
#define DHCP6_OPT_4RD_NON_MAP_RULE	99
#define DHCP6_OPT_LQ_BASE_TIME		100
#define DHCP6_OPT_LQ_START_TIME		101
#define DHCP6_OPT_LQ_END_TIME		102
#define DHCP6_OPT_CAPTIVE_PORTAL	103
#define DHCP6_OPT_MPL_PARAMETERS	104
#define DHCP6_OPT_ANI_ATT		105
#define DHCP6_OPT_ANI_NETWORK_NAME	106
#define DHCP6_OPT_ANI_AP_NAME		107
#define DHCP6_OPT_ANI_AP_BSSID		108
#define DHCP6_OPT_ANI_OPERATOR_ID	109
#define DHCP6_OPT_ANI_OPERATOR_REALM	110
#define DHCP6_OPT_S46_PRIORITY		111
#define DHCP6_OPT_MUD_URL_V6		112
#define DHCP6_OPT_V6_PREFIX64		113
#define DHCP6_OPT_F_BINDING_STATUS	114
#define DHCP6_OPT_F_CONNECT_FLAGS	115
#define DHCP6_OPT_F_DNS_REMOVAL_INFO	116
#define DHCP6_OPT_F_DNS_HOST_NAME	117
#define DHCP6_OPT_F_DNS_ZONE_NAME	118
#define DHCP6_OPT_F_DNS_FLAGS		119
#define DHCP6_OPT_F_EXPIRATION_TIME	120
#define DHCP6_OPT_F_MAX_UNACKED_BNDUPD	121
#define DHCP6_OPT_F_MCLT		122
#define DHCP6_OPT_F_PARTNER_LIFETIME	123
#define DHCP6_OPT_F_PARTNER_LIFETIME_SENT 124
#define DHCP6_OPT_F_PARTNER_DOWN_TIME	125
#define DHCP6_OPT_F_PARTNER_RAW_CLT_TIME 126
#define DHCP6_OPT_F_PROTOCOL_VERSION	127
#define DHCP6_OPT_F_KEEPALIVE_TIME	128
#define DHCP6_OPT_F_RECONFIGURE_DATA	129
#define DHCP6_OPT_F_RELATIONSHIP_NAME	130
#define DHCP6_OPT_F_SERVER_FLAGS	131
#define DHCP6_OPT_F_SERVER_STATE	132
#define DHCP6_OPT_F_START_TIME_OF_STATE	133
#define DHCP6_OPT_F_STATE_EXPIRATION_TIME 134
#define DHCP6_OPT_RELAY_PORT		135
#define DHCP6_OPT_V6_SZTP_REDIRECT	136
#define DHCP6_OPT_S46_BIND_IPV6_PREFIX	137
#define DHCP6_OPT_IA_LL			138
#define DHCP6_OPT_LLADDR		139
#define DHCP6_OPT_SLAP_QUAD		140
#define DHCP6_OPT_V6_DOTS_RI		141
#define DHCP6_OPT_V6_DOTS_ADDRESS	142
#define DHCP6_OPT_IPV6_ADDRESS_ANDSF	143
#define DHCP6_OPT_V6_DNR		144
#define DHCP6_OPT_REGISTERED_DOMAIN	145
#define DHCP6_OPT_FORWARD_DIST_MANAGER	146
#define DHCP6_OPT_REVERSE_DIST_MANAGER	147
#define DHCP6_OPT_ADDR_REG_ENABLE	148
#define DHCP6_OPT_IA_SRV6_LOCATOR	149
#define DHCP6_OPT_IALOCATOR		150


/* Struct describes options for app internal use. */
typedef struct dhcp6_option_params_s {
	const char	*disp_name;	/* User friendly display name. */
	const uint16_t	len;		/* Len. */
	const uint8_t	type;		/* Data type - parser hint. */
	const uint8_t	flags;		/* Flags with additional info. */
	const void	*data_vals;	/* Extra data for parsing. */
	const size_t	data_vals_cnt;	/* Extra data items count. */
} dhcp6_opt_params_t, *dhcp6_opt_params_p;

/* Type. */
#define DHCP6_OPTP_T_NONE	0
#define DHCP6_OPTP_T_SUBOPTS	1 /* data_vals points to dhcp6_opt_params_t array. */
#define DHCP6_OPTP_T_1BYTE	2 /* uint8_t */
#define DHCP6_OPTP_T_2BYTE	3 /* uint16_t */
#define DHCP6_OPTP_T_4BYTE	4 /* uint32_t */
#define DHCP6_OPTP_T_4TIME	5 /* uint32_t */
#define DHCP6_OPTP_T_IPADDR	6 /* uint8_t[16] */
#define DHCP6_OPTP_T_STR_DNS	7 /* DNS string format: https://www.rfc-editor.org/rfc/rfc1035#section-3.1 */
#define DHCP6_OPTP_T_STR_UTF8	8 /* char array. */
#define DHCP6_OPTP_T_STR	9 /* char array. */
#define DHCP6_OPTP_T_ADV	10 /* Option have specific format. This is TODO marker. */
#define DHCP6_OPTP_T_BYTES	11 /* uint8_t array. */
#define DHCP6_OPTP_T__LAST__	DHCP6_OPTP_T_BYTES

static const uint16_t dhcp6_opt_type2size_fixed[] = {
	[DHCP6_OPTP_T_NONE] =		0,
	[DHCP6_OPTP_T_SUBOPTS] =	UINT16_MAX,
	[DHCP6_OPTP_T_1BYTE] =		1,
	[DHCP6_OPTP_T_2BYTE] =		2,
	[DHCP6_OPTP_T_4BYTE] =		4,
	[DHCP6_OPTP_T_4TIME] =		4,
	[DHCP6_OPTP_T_IPADDR] =		16,
	[DHCP6_OPTP_T_STR_DNS] =	UINT16_MAX,
	[DHCP6_OPTP_T_STR_UTF8] =	UINT16_MAX,
	[DHCP6_OPTP_T_STR] =		UINT16_MAX,
	[DHCP6_OPTP_T_ADV] =		UINT16_MAX,
	[DHCP6_OPTP_T_BYTES] =		UINT16_MAX,
	[(DHCP6_OPTP_T__LAST__ + 1)] =	UINT16_MAX,
};
static const uint16_t dhcp6_opt_type2size_min[] = {
	[DHCP6_OPTP_T_NONE] =		UINT16_MAX,
	[DHCP6_OPTP_T_SUBOPTS] =	sizeof(dhcp6_opt_hdr_t),
	[DHCP6_OPTP_T_1BYTE] =		UINT16_MAX,
	[DHCP6_OPTP_T_2BYTE] =		UINT16_MAX,
	[DHCP6_OPTP_T_4BYTE] =		UINT16_MAX,
	[DHCP6_OPTP_T_4TIME] =		UINT16_MAX,
	[DHCP6_OPTP_T_IPADDR] =		UINT16_MAX,
	[DHCP6_OPTP_T_STR_DNS] =	1,
	[DHCP6_OPTP_T_STR_UTF8] =	1,
	[DHCP6_OPTP_T_STR] =		1,
	[DHCP6_OPTP_T_ADV] =		UINT16_MAX,
	[DHCP6_OPTP_T_BYTES] =		1,
	[(DHCP6_OPTP_T__LAST__ + 1)] =	UINT16_MAX,
};


/* Flags. */
#define DHCP6_OPTP_F_FIXEDLEN	(((uint8_t)1) << 0) /* Option len have fixed value. Have less prion than type size. */
#define DHCP6_OPTP_F_MINLEN	(((uint8_t)1) << 1) /* Minimum option len is known. Have more prion than type size. */
#define DHCP6_OPTP_F_ARRAY	(((uint8_t)1) << 2) /* In case (FIXEDLEN + ARRAY), Len = sizeof 1 element.
						     * Aslo indicate that option may have multiple values of same type. */
#define DHCP6_OPTP_F_MULTI	(((uint8_t)1) << 3) /* Allowed option to appear multiple times / opposite to singleton. */


#define DHCP6_OPT_PARAMS_UNKNOWN {					\
	.disp_name = "Unknown",						\
	.len = 0,							\
	.type = DHCP6_OPTP_T_BYTES,					\
	.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),		\
	.data_vals = NULL,						\
	.data_vals_cnt = 0,						\
}
static const dhcp6_opt_params_t dhcp6_opt_params_unknown = DHCP6_OPT_PARAMS_UNKNOWN;


/* RFC 9915 Dynamic Host Configuration Protocol for IPv6 (DHCPv6) */
static const char *dhcp6_opt13[] = {
/*   0 */	"Success",
/*   1 */	"UnspecFail",
/*   2 */	"NoAddrsAvail",
/*   3 */	"NoBinding",
/*   4 */	"NotOnLink",
/*   5 */	"UseMulticast",
/*   6 */	"NoPrefixAvail",
/*   7 */	"UnknownQueryType",
/*   8 */	"MalformedQuery",
/*   9 */	"NotConfigured",
/*  10 */	"NotAllowed",
/*  11 */	"QueryTerminated",
/*  12 */	"DataMissing",
/*  13 */	"CatchUpComplete",
/*  14 */	"NotSupported",
/*  15 */	"TLSConnectionRefused",
/*  16 */	"AddressInUse",
/*  17 */	"ConfigurationConflict",
/*  18 */	"MissingBindingInformation",
/*  19 */	"OutdatedBindingInformation",
/*  20 */	"ServerShuttingDown",
/*  21 */	"DNSUpdateNotSupported",
/*  22 */	"ExcessiveTimeSkew",
/*  23 */	"NoSRv6LocatorAvail",
};

/* RFC 5678 Mobility Services for DCHP Options */
static const dhcp6_opt_params_t dhcp6_opt54[] = {
/*   0 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*   1 */	{
			.disp_name = "IS",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*   2 */	{
			.disp_name = "CS",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*   3 */	{
			.disp_name = "ES",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
};

/* RFC 5678 Mobility Services for DCHP Options */
static const dhcp6_opt_params_t dhcp6_opt55[] = {
/*   0 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*   1 */	{
			.disp_name = "IS",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*   2 */	{
			.disp_name = "CS",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*   3 */	{
			.disp_name = "ES",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
};

/* RFC 5908 NTP Server Option for DHCPv6 */
static const dhcp6_opt_params_t dhcp6_opt56[] = {
/*   0 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*   1 */	{
			.disp_name = "NTP Server Address",
			.type = DHCP6_OPTP_T_IPADDR,
		},
/*   2 */	{
			.disp_name = "NTP Multicast Address",
			.type = DHCP6_OPTP_T_IPADDR,
		},
/*   3 */	{
			.disp_name = "NTP Server FQDN",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
};

/* RFC 8156 DHCPv6 Failover Protocol */
static const char *dhcp6_opt114[] = {
/*   0 */	NULL,
/*   1 */	"ACTIVE",
/*   2 */	"EXPIRED",
/*   3 */	"RELEASED",
/*   4 */	"PENDING-FREE",
/*   5 */	"FREE",
/*   6 */	"FREE-BACKUP",
/*   7 */	"ABANDONED",
/*   8 */	"RESET",
};

/* RFC 8156 DHCPv6 Failover Protocol */
static const char *dhcp6_opt132[] = {
/*   0 */	NULL,
/*   1 */	"STARTUP: Startup state (1)",
/*   2 */	"NORMAL: Normal state",
/*   3 */	"COMMUNICATIONS-INTERRUPTED: Communications interrupted",
/*   4 */	"PARTNER-DOWN: Partner down",
/*   5 */	"POTENTIAL-CONFLICT: Synchronizing",
/*   6 */	"RECOVER: Recovering bindings from partner",
/*   7 */	"RECOVER-WAIT: Waiting out MCLT after RECOVER",
/*   8 */	"RECOVER-DONE: Interlock state prior to NORMAL",
/*   9 */	"RESOLUTION-INTERRUPTED: Comm. failed during resolution",
/*  10 */	"CONFLICT-DONE: Primary resolved its conflicts",
};


static const dhcp6_opt_params_t dhcp6_options[] = {
/*   0 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*   1 */	{
			.disp_name = "Client Identifier",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*   2 */	{
			.disp_name = "Server Identifier",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*   3 */	{
			.disp_name = "Identity Association for Non-Temporary Addresses",
			.len = 12,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/*   4 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*   5 */	{
			.disp_name = "IA Address",
			.len = 24,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/*   6 */	{
			.disp_name = "Option Request",
			.type = DHCP6_OPTP_T_2BYTE,
			.flags = DHCP6_OPTP_F_ARRAY,
			/* filter for request opts codes. */
		},
/*   7 */	{
			.disp_name = "Preference",
			.type = DHCP6_OPTP_T_1BYTE,
		},
/*   8 */	{
			.disp_name = "Elapsed Time",
			.type = DHCP6_OPTP_T_2BYTE,
		},
/*   9 */	{
			.disp_name = "Relay Message",
			.type = DHCP6_OPTP_T_STR_UTF8,
		},
/*  10 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*  11 */	{
			.disp_name = "Authentication",
			.len = 11,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  12 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*  13 */	{
			.disp_name = "Status Code",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
			.data_vals = (const void*)dhcp6_opt13,
			.data_vals_cnt = nitems(dhcp6_opt13),
		},
/*  14 */	{
			.disp_name = "Rapid Commit",
			.type = DHCP6_OPTP_T_NONE,
		},
/*  15 */	{
			.disp_name = "User Class",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  16 */	{
			.disp_name = "Vendor Class",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/*  17 */	{
			.disp_name = "Vendor-Specific Information",
			.type = DHCP6_OPTP_T_SUBOPTS,
			.flags = DHCP6_OPTP_F_MULTI,
		},
/*  18 */	{
			.disp_name = "Interface-Id",
			.type = DHCP6_OPTP_T_BYTES,
		},
/*  19 */	{
			.disp_name = "Reconfigure Message",
			.type = DHCP6_OPTP_T_1BYTE,
		},
/*  20 */	{
			.disp_name = "Reconfigure Accept",
			.type = DHCP6_OPTP_T_NONE,
		},
/*  21 */	{ /* RFC 3319 DHCPv6 Options for SIP Servers */
			.disp_name = "SIP Servers Domain Name List",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  22 */	{ /* RFC 3319 DHCPv6 Options for SIP Servers */
			.disp_name = "SIP Servers IPv6 Address List",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  23 */	{ /* RFC 3646 DNS Configuration Options for DHCPv6 */
			.disp_name = "DNS Recursive Name Server",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  24 */	{ /* RFC 3646 DNS Configuration Options for DHCPv6 */
			.disp_name = "Domain Search List",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  25 */	{
			.disp_name = "Identity Association for Prefix Delegation",
			.len = 12,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/*  26 */	{
			.disp_name = "IA Prefix",
			.len = 25,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/*  27 */	{ /* RFC 3898 NIS Configuration Options for DHCPv6 */
			.disp_name = "NIS Servers",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  28 */	{ /* RFC 3898 NIS Configuration Options for DHCPv6 */
			.disp_name = "NIS+ Servers",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  29 */	{ /* RFC 3898 NIS Configuration Options for DHCPv6 */
			.disp_name = "NIS Domain Name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  30 */	{ /* RFC 3898 NIS Configuration Options for DHCPv6 */
			.disp_name = "NIS+ Domain Name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  31 */	{ /* RFC 4075 SNTP Configuration Option for DHCPv6 */
			.disp_name = "SNTP Servers",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  32 */	{
			.disp_name = "Information Refresh Time",
			.type = DHCP6_OPTP_T_4BYTE,
		},
/*  33 */	{ /* RFC 4280 DHCP Options for BMCS */
			.disp_name = "BMCS Domain Name List",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  34 */	{ /* RFC 4280 DHCP Options for BMCS */
			.disp_name = "BMCS Address",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  35 */	DHCP6_OPT_PARAMS_UNKNOWN,
/*  36 */	{ /* RFC 4776 DHCP Civic */
			.disp_name = "Civic Location",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  37 */	{ /* RFC 4649 Relay Agent Remote-ID */
			.disp_name = "Relay Agent Remote-ID",
			.len = 5,
			.type = DHCP6_OPTP_T_BYTES,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  38 */	{ /* RFC 4580 Relay Agent Subscriber-ID */
			.disp_name = "Relay Agent Subscriber-ID",
			.type = DHCP6_OPTP_T_BYTES,
		},
/*  39 */	{ /* RFC 4704 The DHCPv6 Client FQDN Option */
			.disp_name = "Client FQDN",
			.len = 1,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  40 */	{ /* RFC 5192 PAA DHCP Options */
			.disp_name = "PANA Authentication Agent",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  41 */	{ /* RFC 4833 Timezone Options for DHCP */
			.disp_name = "TZ-POSIX",
			.type = DHCP6_OPTP_T_STR,
		},
/*  42 */	{ /* RFC 4833 Timezone Options for DHCP */
			.disp_name = "TZ Name",
			.type = DHCP6_OPTP_T_STR,
		},
/*  43 */	{ /* RFC 4994 Relay Agent ERO */
			.disp_name = "Relay Agent Echo Request",
			.type = DHCP6_OPTP_T_2BYTE,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  44 */	{ /* RFC 5007 DHCPv6 Leasequery */
			.disp_name = "Lease query",
			.len = 17,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  45 */	{ /* RFC 5007 DHCPv6 Leasequery */
			.disp_name = "Client Data",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  46 */	{ /* RFC 5007 DHCPv6 Leasequery */
			.disp_name = "Client Last Transaction Time",
			.type = DHCP6_OPTP_T_4TIME,
		},
/*  47 */	{ /* RFC 5007 DHCPv6 Leasequery */
			.disp_name = "Relay Data",
			.len = 16,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  48 */	{ /* RFC 5007 DHCPv6 Leasequery */
			.disp_name = "Client Link",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  49 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Home Network ID FQDN",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  50 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Visited Home Network Information",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  51 */	{ /* RFC 5223 DHCP-Based LoST Discovery */
			.disp_name = "LoST Server",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  52 */	{ /* RFC 5417 CAPWAP AC DHCP Option */
			.disp_name = "CAPWAP AC",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  53 */	{ /* RFC 5460 DHCPv6 Bulk Leasequery */
			.disp_name = "Relay-ID",
			.len = 2,
			.type = DHCP6_OPTP_T_BYTES,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  54 */	{ /* RFC 5678 Mobility Services for DCHP Options */
			.disp_name = "MoS IPv6 Address",
			.type = DHCP6_OPTP_T_SUBOPTS,
			.data_vals = (const void*)dhcp6_opt54,
			.data_vals_cnt = nitems(dhcp6_opt54),
		},
/*  55 */	{ /* RFC 5678 Mobility Services for DCHP Options */
			.disp_name = "MoS Domain Name List Option",
			.type = DHCP6_OPTP_T_SUBOPTS,
			.data_vals = (const void*)dhcp6_opt55,
			.data_vals_cnt = nitems(dhcp6_opt55),
		},
/*  56 */	{ /* RFC 5908 NTP Server Option for DHCPv6 */
			.disp_name = "NTP Server",
			.type = DHCP6_OPTP_T_SUBOPTS,
			.data_vals = (const void*)dhcp6_opt56,
			.data_vals_cnt = nitems(dhcp6_opt56),
		},
/*  57 */	{ /* RFC 5986 LIS Discovery */
			.disp_name = "Access Network Domain Name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  58 */	{ /* RFC 6011 SIP UA Configuration */
			.disp_name = "SIP User Agent Configuration Service Domains",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  59 */	{ /* RFC 5970 DHCPv6 Options for Network Boot */
			.disp_name = "Boot File URL",
			.type = DHCP6_OPTP_T_STR,
		},
/*  60 */	{ /* RFC 5970 DHCPv6 Options for Network Boot */
			.disp_name = "Boot File Parameters",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  61 */	{ /* RFC 5970 DHCPv6 Options for Network Boot */
			.disp_name = "Client System Architecture Type",
			.type = DHCP6_OPTP_T_2BYTE,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  62 */	{ /* RFC 5970 DHCPv6 Options for Network Boot */
			.disp_name = "Client Network Interface Identifier",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/*  63 */	{ /* RFC 6225 DHCP Options for Coordinate LCI */
			.disp_name = "GeoLoc",
			.len = 16,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/*  64 */	{ /* RFC 6334 DS-Lite DHCPv6 Option */
			.disp_name = "AFTR-Name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  65 */	{ /* RFC 6440 The ERP Local Domain Name DHCPv6 Option */
			.disp_name = "ERP Local Domain Name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  66 */	{ /* RFC 6422 Relay-Supplied DHCP Options */
			.disp_name = "Relay-Supplied DHCP Options",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  67 */	{ /* RFC 6603 PD Exclude Option */
			.disp_name = "Prefix Exclude",
			.len = 2, /* 2-17 */
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  68 */	{ /* RFC 6607 Virtual Subnet Selection Options */
			.disp_name = "Virtual Subnet Selection",
			.len = 1,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  69 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Identified Home Network Information",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  70 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Unrestricted Home Network Information",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  71 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Home Network Prefix",
			.len = 1,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  72 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Home Agent Address",
			.type = DHCP6_OPTP_T_IPADDR,
		},
/*  73 */	{ /* RFC 6610 DHCPv6 for Home Info Discovery in MIPv6 */
			.disp_name = "MIPv6 Home Agent FQDN",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/*  74 */	{ /* RFC 6731 RDNSS Selection for MIF Nodes */
			.disp_name = "RDNSS Selection",
			.len = 17,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  75 */	{ /* RFC 6784 Kerberos Options for DHCPv6 */
			.disp_name = "Kerberos Principal Name",
			.len = 5,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  76 */	{ /* RFC 6784 Kerberos Options for DHCPv6 */
			.disp_name = "Kerberos Realm Name",
			.len = 5,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  77 */	{ /* RFC 6784 Kerberos Options for DHCPv6 */
			.disp_name = "Kerberos Default Realm Name",
			.len = 5,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  78 */	{ /* RFC 6784 Kerberos Options for DHCPv6 */
			.disp_name = "Kerberos KDC",
			.len = 23,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  79 */	{ /* RFC 6939 DHCPv6 Client Link-Layer Address Option */
			.disp_name = "Client Link-Layer Address",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  80 */	{ /* RFC 6977 DHCPv6 Relay-Triggered Reconfiguration */
			.disp_name = "Link Address",
			.type = DHCP6_OPTP_T_IPADDR,
		},
/*  81 */	{ /* RFC 7037 DHCPv6 RADIUS Option */
			.disp_name = "RADIUS",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  82 */	{
			.disp_name = "SOL_MAX_RT",
			.type = DHCP6_OPTP_T_4BYTE,
		},
/*  83 */	{
			.disp_name = "INF_MAX_RT",
			.type = DHCP6_OPTP_T_4BYTE,
		},
/*  84 */	{ /* RFC 7078 DHCPv6 Address Selection Policy Opt */
			.disp_name = "Address Selection",
			.len = 1,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  85 */	{ /* RFC 7078 DHCPv6 Address Selection Policy Opt */
			.disp_name = "Address Selection Policy Table",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  86 */	{ /* RFC 7291 PCP DHCP Options */
			.disp_name = "PCP Server",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  87 */	{ /* RFC 7341 DHCPv4 over DHCPv6 */
			.disp_name = "DHCPv4 Message",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  88 */	{ /* RFC 7341 DHCPv4 over DHCPv6 */
			.disp_name = "DHCP 4o6 Server Address",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/*  89 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 Rule",
			.len = 9,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/*  90 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 BR",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_MULTI,
		},
/*  91 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 DMR",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  92 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 IPv4/IPv6 Address Binding",
			.len = 5,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/*  93 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 Port Parameters",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/*  94 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 MAP-E Container",
			.type = DHCP6_OPTP_T_SUBOPTS,
			.flags = DHCP6_OPTP_F_MULTI,
		},
/*  95 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 MAP-T Container",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  96 */	{ /* RFC 7598 DHCPv6 for Softwire 46 CEs */
			.disp_name = "S46 Lightweight 4over6 Container",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  97 */	{ /* RFC 7600 Stateless IPv4 Residual Deployment (4rd) */
			.disp_name = "4rd",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/*  98 */	{ /* RFC 7600 Stateless IPv4 Residual Deployment (4rd) */
			.disp_name = "4rd Mapping-Rule",
			.len = 20,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/*  99 */	{ /* RFC 7600 Stateless IPv4 Residual Deployment (4rd) */
			.disp_name = "4rd Non-Mapping-Rule",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/* 100 */	{ /* RFC 7653 DHCPv6 Active Leasequery */
			.disp_name = "Leasequery Base Time",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 101 */	{ /* RFC 7653 DHCPv6 Active Leasequery */
			.disp_name = "Leasequery Star Time",
			.type = DHCP6_OPTP_T_4TIME,
		},
/* 102 */	{ /* RFC 7653 DHCPv6 Active Leasequery */
			.disp_name = "Leasequery End Time",
			.type = DHCP6_OPTP_T_4TIME,
		},
/* 103 */	{ /* RFC 8910 Captive-Portal Identification in DHCP and Router Advertisements (RAs) */
			.disp_name = "Captive-Portal URL",
			.type = DHCP6_OPTP_T_STR,
		},
/* 104 */	{ /* RFC 7774 MPL Configuration for DHCPv6 */
			.disp_name = "MPL Parameter Configuration",
			.len = 16,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/* 105 */	{ /* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
			.disp_name = "ANI Access Technology Type",
			.len = 2,
			.type = DHCP6_OPTP_T_BYTES,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/* 106 */	{ /* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
			.disp_name = "ANI Network Name",
			.type = DHCP6_OPTP_T_STR_UTF8,
		},
/* 107 */	{ /* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
			.disp_name = "ANI Access Point Name",
			.type = DHCP6_OPTP_T_STR_UTF8,
		},
/* 108 */	{ /* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
			.disp_name = "ANI Access Point BSSID",
			.len = 6,
			.type = DHCP6_OPTP_T_BYTES,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/* 109 */	{ /* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
			.disp_name = "ANI Operator Identifier",
			.type = DHCP6_OPTP_T_4BYTE,
		},
/* 110 */	{ /* RFC 7839 ANI Options for DHCPv4 and DHCPv6 */
			.disp_name = "ANI Operator Realm",
			.type = DHCP6_OPTP_T_BYTES,
		},
/* 111 */	{ /* RFC 8026 OPTION_S46_PRIORITY DHCPv6 Option */
			.disp_name = "S46 Priority",
			.len = 2,
			.type = DHCP6_OPTP_T_BYTES,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/* 112 */	{ /* RFC 8520 Manufacturer Usage Descriptions */
			.disp_name = "MUD URL",
			.type = DHCP6_OPTP_T_STR,
		},
/* 113 */	{ /* RFC 8115 IPv4/IPv6 Multicast Prefixes Option */
			.disp_name = "IPv6 prefix 64",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/* 114 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover binding status",
			.type = DHCP6_OPTP_T_1BYTE,
			.data_vals = (const void*)dhcp6_opt114,
			.data_vals_cnt = nitems(dhcp6_opt114),
		},
/* 115 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover connect flags",
			.type = DHCP6_OPTP_T_2BYTE,
		},
/* 116 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover DNS removal info",
			.type = DHCP6_OPTP_T_SUBOPTS,
		},
/* 117 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover DNS host name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/* 118 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover DNS zone name",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/* 119 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover DNS flags",
			.type = DHCP6_OPTP_T_2BYTE,
		},
/* 120 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover expiration time",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 121 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover max unacked BNDUPD",
			.type = DHCP6_OPTP_T_4BYTE,
		},
/* 122 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Maximum Client Lead Time (MCLT)",
			.type = DHCP6_OPTP_T_4TIME,
		},
/* 123 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover partner lifetime",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 124 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover partner lifetime sent",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 125 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover partner down time",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 126 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover partner raw clt time",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 127 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover protocol version",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_FIXEDLEN,
		},
/* 128 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover keepalive time",
			.type = DHCP6_OPTP_T_4TIME,
		},
/* 129 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover reconfigure data",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/* 130 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover relationship name",
			.type = DHCP6_OPTP_T_STR_UTF8,
		},
/* 131 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover server flags",
			.type = DHCP6_OPTP_T_1BYTE,
		},
/* 132 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover server state",
			.type = DHCP6_OPTP_T_1BYTE,
			.data_vals = (const void*)dhcp6_opt132,
			.data_vals_cnt = nitems(dhcp6_opt132),
		},
/* 133 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover start time of state",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 134 */	{ /* RFC 8156 DHCPv6 Failover Protocol */
			.disp_name = "Failover state expiration time",
			.type = DHCP6_OPTP_T_4TIME, /* seconds since midnight January 1, 2000 UTC */
		},
/* 135 */	{ /* RFC 8357 DHCP Relay Source Port */
			.disp_name = "Relay Source Port",
			.type = DHCP6_OPTP_T_2BYTE,
		},
/* 136 */	{ /* RFC 8572 Secure Zero Touch Provisioning (SZTP) */
			.disp_name = "SZTP Redirect",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/* 137 */	{ /* RFC 8539 Softwire Provisioning with DHCP 4o6 */
			.disp_name = "Softwire Source Binding Prefix Hint",
			.len = 1,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/* 138 */	{ /* RFC 8947 Link-Layer Address Assignment Mechanism for DHCPv6 */
			.disp_name = "Identity Association for Link-Layer Addresses",
			.len = 12,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/* 139 */	{ /* RFC 8947 Link-Layer Address Assignment Mechanism for DHCPv6 */
			.disp_name = "Link-Layer Addresses",
			.len = 12,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/* 140 */	{ /* RFC 8948 Structured Local Address Plan (SLAP) Quadrant Selection Option for DHCPv6 */
			.disp_name = "QUAD",
			.len = 2,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_FIXEDLEN | DHCP6_OPTP_F_ARRAY),
		},
/* 141 */	{ /* RFC 8973 DDoS Open Threat Signaling (DOTS) Agent Discovery */
			.disp_name = "DOTS Reference Identifier",
			.type = DHCP6_OPTP_T_STR_DNS,
		},
/* 142 */	{ /* RFC 8973 DDoS Open Threat Signaling (DOTS) Agent Discovery */
			.disp_name = "DOTS Address",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/* 143 */	{ /* RFC 6153 ANDSF DHCP Options */
			.disp_name = "ANDSF IPv6 Address",
			.type = DHCP6_OPTP_T_IPADDR,
			.flags = DHCP6_OPTP_F_ARRAY,
		},
/* 144 */	{ /* RFC 9463 DHCP and Router Advertisement Options for the Discovery of Network-designated Resolvers (DNR) */
			.disp_name = "DHCPv6 Encrypted DNS",
			.len = 4,
			.type = DHCP6_OPTP_T_ADV,
			.flags = (DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_MULTI),
		},
/* 145 */	{ /* RFC 9527 DHCPv6 Options for the Homenet Naming Authority */
			.disp_name = "Registered Homenet Domain",
			.type = DHCP6_OPTP_T_STR_DNS,
			.flags = DHCP6_OPTP_F_MULTI,
		},
/* 146 */	{ /* RFC 9527 DHCPv6 Options for the Homenet Naming Authority */
			.disp_name = "Forward Distribution Manager",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/* 147 */	{ /* RFC 9527 DHCPv6 Options for the Homenet Naming Authority */
			.disp_name = "Reverse Distribution Manager Server",
			.len = 3,
			.type = DHCP6_OPTP_T_ADV,
			.flags = DHCP6_OPTP_F_MINLEN,
		},
/* 148 */	{ /* RFC 9686 Registering Self-Generated IPv6 Addresses Using DHCPv6 */
			.disp_name = "Reverse Distribution Manager Server",
			.type = DHCP6_OPTP_T_NONE,
		},
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/* Runtime options validator. */
static int
dhcp6_validate_opt_params(dhcp6_opt_params_p opts, const size_t opts_count) {
	int error;
	dhcp6_opt_params_p opt;

	if (0 == opts_count)
		return (0);
	if (NULL == opts)
		return (EINVAL);

	/* Validate type2size tables. */
	if (nitems(dhcp6_opt_type2size_fixed) != nitems(dhcp6_opt_type2size_min))
		return (EINVAL);
	for (size_t i = 0; i < nitems(dhcp6_opt_type2size_fixed); i ++) {
		if (UINT16_MAX != dhcp6_opt_type2size_fixed[i] &&
		    UINT16_MAX != dhcp6_opt_type2size_min[i])
			return (EINVAL);
	}

	/* Runtime validation of main options. */
	for (size_t i = 0; i < opts_count; i ++) {
		opt = &opts[i];

		/* Allow len only with flags. */
		if (0 != opt->len &&
		    0 == ((DHCP6_OPTP_F_MINLEN | DHCP6_OPTP_F_FIXEDLEN) & opt->flags))
			return (EINVAL);
		/* Check that MINLEN or FIXEDLEN set, not together. */
		if (0 != (DHCP6_OPTP_F_MINLEN & opt->flags) &&
		    0 != ((DHCP6_OPTP_F_FIXEDLEN) & opt->flags))
			return (EINVAL);
		/* MINLEN and ARRAY flages check. */
		if (0 != (DHCP6_OPTP_F_MINLEN & opt->flags) &&
		    0 != (DHCP6_OPTP_F_ARRAY & opt->flags) &&
		    DHCP6_OPTP_T_STR_DNS != opt->type && DHCP6_OPTP_T_ADV != opt->type) /* Allow arrays for this typees. */
			return (EINVAL);
		/* FIXEDLEN and ARRAY can be used together. */
		/* Check that ARRAY properly used with types. */
		if (0 != (DHCP6_OPTP_F_ARRAY & opt->flags) &&
		    0 == (DHCP6_OPTP_F_FIXEDLEN & opt->flags) &&
		    UINT16_MAX == dhcp6_opt_type2size_fixed[opt->type] &&
		    DHCP6_OPTP_T_STR_DNS != opt->type && DHCP6_OPTP_T_ADV != opt->type) /* Allow arrays for this type. */
			return (EINVAL);
		/* Check NONE type. */
		if (DHCP6_OPTP_T_NONE == opt->type &&
		    (0 != opt->len || 0 != opt->flags))
			return (EINVAL);
		/* Check SUBOPTS. */
		if (DHCP6_OPTP_T_SUBOPTS == opt->type) {
			error = dhcp6_validate_opt_params(
			    (dhcp6_opt_params_p)opt->data_vals, opt->data_vals_cnt);
			if (0 != error)
				return (error);
		}
	}

	return (0); /* OK. */
}


#if BYTE_ORDER == BIG_ENDIAN

static inline uint16_t
dhcp6_ntohs(const uint8_t *p) {
	return (((uint16_t)(((uint16_t)p[0]) << 0)) |
		((uint16_t)(((uint16_t)p[1]) << 8)));
}
static inline void
dhcp6_htons(const uint16_t v, uint8_t *p) {
	p[0] = (uint8_t)(v >> 0);
	p[1] = (uint8_t)(v >> 8);
}

static inline uint32_t
dhcp6_ntohl(const uint8_t *p) {
	return (((uint32_t)(((uint32_t)p[0]) <<  0)) |
	        ((uint32_t)(((uint32_t)p[1]) <<  8)) |
	        ((uint32_t)(((uint32_t)p[2]) << 16)) |
		((uint32_t)(((uint32_t)p[3]) << 24)));
}
static inline void
dhcp6_htonl(const uint32_t v, uint8_t *p) {
	p[0] = (uint8_t)(v >>  0);
	p[1] = (uint8_t)(v >>  8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

#else

static inline uint16_t
dhcp6_ntohs(const uint8_t *p) {
	return (((uint16_t)(((uint16_t)p[0]) << 8)) |
		((uint16_t)(((uint16_t)p[1]) << 0)));
}
static inline void
dhcp6_htons(const uint16_t v, uint8_t *p) {
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)(v >> 0);
}

static inline uint32_t
dhcp6_ntohl(const uint8_t *p) {
	return (((uint32_t)(((uint32_t)p[0]) << 24)) |
	        ((uint32_t)(((uint32_t)p[1]) << 16)) |
		((uint32_t)(((uint32_t)p[2]) <<  8)) |
		((uint32_t)(((uint32_t)p[3]) <<  0)));
}
static inline void
dhcp6_htonl(const uint32_t v, uint8_t *p) {
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >>  8);
	p[3] = (uint8_t)(v >>  0);
}

#endif


#define DHCP6_DNS_SEQ_LABEL_DATA_MASK		((uint8_t)0x3f)	/* --XXXXXX */
#define DHCP6_DNS_SEQ_LABEL_CTRL_MASK		((uint8_t)0xc0)	/* XX------ */

static inline int
dhcp6_dns_decode_calc_size(const uint8_t *buf, const size_t buf_size, size_t *off,
    size_t *name_len_ret) {
	const uint8_t *rpos = buf, *rpos_max = (buf + buf_size);
	size_t name_len = 0;
	uint8_t label;

	if (NULL == buf || 0 == buf_size)
		return (EINVAL);

	if (NULL != off) {
		if ((*off) >= buf_size)
			return (EINVAL);
		rpos += (*off);
	}

	for (; rpos < rpos_max;) {
		label = rpos[0];
		rpos ++; /* Move to data / next name. */
		if (0 == label) /* Zero label = end of name. */
			break;
		/* Is label have flags? */
		if (0 != (label & DHCP6_DNS_SEQ_LABEL_CTRL_MASK))
			return (EBADMSG);
		rpos += label; /* Move to next label. */
		/* Is label len valid? */
		if (rpos > rpos_max)
			return (EBADMSG); /* Out of buf range. */
		name_len += (label + 1); /* +1 for dot. */
	}

	if (NULL != off) {
		(*off) = (size_t)(rpos - buf);
	}
	if (0 != name_len) { /* Clear last dot. */
		name_len --;
	}
	if (NULL != name_len_ret) {
		(*name_len_ret) = name_len;
	}

	return (0);
}

static inline int
dhcp6_dns_decode(const uint8_t *buf, const size_t buf_size, size_t *off,
    uint8_t *name, const size_t name_buf_size, size_t *name_len_ret) {
	const uint8_t *rpos = buf, *rpos_max = (buf + buf_size);
	uint8_t *wpos = name, *wpos_max = (name + name_buf_size);
	uint8_t label;

	if (NULL == buf || 0 == buf_size ||
	    NULL == name || 0 == name_buf_size)
		return (EINVAL);

	if (NULL != off) {
		if ((*off) >= buf_size)
			return (EINVAL);
		rpos += (*off);
	}

	for (; rpos < rpos_max;) {
		label = rpos[0];
		rpos ++; /* Move to data / next name. */
		if (0 == label) /* Zero label = end of name. */
			break;
		/* Is label have flags? */
		if (0 != (label & DHCP6_DNS_SEQ_LABEL_CTRL_MASK))
			return (EBADMSG);
		/* Is label len valid? */
		if ((rpos + label) > rpos_max)
			return (EBADMSG); /* Out of buf range. */
		/* Does buf space enough? */
		if ((wpos + label + 1) > wpos_max)
			return (ENOBUFS); /* Out of buf range. */
		memcpy(wpos, rpos, label);
		wpos[label] = '.';
		wpos += (label + 1);
		rpos += label; /* Move to next label. */
	}

	if (NULL != off) {
		(*off) = (size_t)(rpos - buf);
	}
	if (wpos != name) { /* Clear last dot. */
		wpos --;
	}
	wpos[0] = 0; /* Set zero at the end. */
	if (NULL != name_len_ret) {
		(*name_len_ret) = (size_t)(wpos - name);
	}

	return (0);
}

static inline int
dhcp6_dns_encode(const uint8_t *name, const size_t name_size,
    uint8_t *buf, const size_t buf_size, size_t *buf_size_ret) {
	size_t size_ret, lbl_idx, i, len;

	if ((NULL == name && 0 != name_size) ||
	    (NULL == buf && 0 != buf_size))
		return (EINVAL);

	/* Calculate result size for name. */
	size_ret = (0 == name_size) ? 1 : (name_size + 2);
	if (NULL != buf_size_ret) {
		(*buf_size_ret) = size_ret;
	}
	if (size_ret > buf_size) /* Is buf space enough? */
		return (ENOBUFS);
	if (0 == name_size) {
		buf[0] = 0; /* Write null label = end marker. */
		return (0);
	}

	/* Copy name to result buf. */
	memmove((buf + 1), name, name_size);
	buf[(name_size + 1)] = 0; /* Write null label = end marker. */

	/* Replace dots by labels with len. */
	for (lbl_idx = 0, i = 1; i < size_ret; i ++) {
		if ('.' != buf[i] && i < (size_ret - 1)) /* Dot or zero label. */
			continue;
		len = ((i - 1) - lbl_idx); /* -1: dont count dot. */
		if (0 == len || DHCP6_DNS_SEQ_LABEL_DATA_MASK < len) /* Label max size is 63 bytes. */
			return (EINVAL);
		buf[lbl_idx] = (uint8_t)len;
		lbl_idx = i;
	}

	return (0);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/* This is 65536 bit map. */
typedef union dhcp6_options_buf_bitmap_s {
	uint8_t		u8[8192];  /* (65536 / 8) */
	uint16_t	u16[4096]; /* (65536 / 16) */
	uint32_t	u32[2048]; /* (65536 / 32) */
	uint64_t	u64[1024]; /* (65536 / 64) */
} dhcp6_o_buf_map_t, *dhcp6_o_buf_map_p;


static int
dhcp6_o_buf_map_is_set(dhcp6_o_buf_map_p ob_map, const uint16_t idx) {

	if (NULL == ob_map)
		return (0);
	return (0 != (ob_map->u64[(idx / 64)] & (((uint64_t)1) << (idx % 64))));
}

static void
dhcp6_o_buf_map_set(dhcp6_o_buf_map_p ob_map, const uint16_t idx, const int val) {

	if (NULL == ob_map)
		return;
	if (0 == val) {  /* Clear bit. */
		ob_map->u64[(idx / 64)] &= ~(((uint64_t)1) << (idx % 64));
	} else {
		ob_map->u64[(idx / 64)] |= (((uint64_t)1) << (idx % 64));
	}
}

static int
dhcp6_o_buf_map_buf_import(dhcp6_o_buf_map_p ob_map, const int val,
    const uint8_t *buf, const size_t buf_size) {
	uint16_t u16;

	if (NULL == ob_map ||
	    (NULL == buf && 0 != buf_size))
		return (EINVAL);

	for (size_t i = 0; i < buf_size; i += sizeof(uint16_t)) {
		memcpy(&u16, &buf[i], sizeof(uint16_t));
		dhcp6_o_buf_map_set(ob_map, u16, val);
	}

	return (0);
}

static void
dhcp6_o_buf_map_invert(dhcp6_o_buf_map_p ob_map) {

	if (NULL == ob_map)
		return;
	for (size_t i = 0; i < nitems(ob_map->u64); i ++) {
		ob_map->u64[i] = ~ob_map->u64[i];
	}
}

static void
dhcp6_o_buf_map_and(dhcp6_o_buf_map_p dst, dhcp6_o_buf_map_p src1, dhcp6_o_buf_map_p src2) {

	if (NULL == dst || NULL == src1 || NULL == src2)
		return;
	for (size_t i = 0; i < nitems(dst->u64); i ++) {
		dst->u64[i] = (src1->u64[i] & src2->u64[i]);
	}
}

static void
dhcp6_o_buf_map_or(dhcp6_o_buf_map_p dst, dhcp6_o_buf_map_p src1, dhcp6_o_buf_map_p src2) {

	if (NULL == dst || NULL == src1 || NULL == src2)
		return;
	for (size_t i = 0; i < nitems(dst->u64); i ++) {
		dst->u64[i] = (src1->u64[i] | src2->u64[i]);
	}
}

static void
dhcp6_o_buf_map_xor(dhcp6_o_buf_map_p dst, dhcp6_o_buf_map_p src1, dhcp6_o_buf_map_p src2) {

	if (NULL == dst || NULL == src1 || NULL == src2)
		return;
	for (size_t i = 0; i < nitems(dst->u64); i ++) {
		dst->u64[i] = (src1->u64[i] ^ src2->u64[i]);
	}
}

static void
dhcp6_o_buf_map_and_inv(dhcp6_o_buf_map_p dst, dhcp6_o_buf_map_p src) {

	if (NULL == dst || NULL == src)
		return;
	for (size_t i = 0; i < nitems(dst->u64); i ++) {
		dst->u64[i] &= ~src->u64[i];
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


typedef struct dhcp6_options_buf_option_data_s {
	uint8_t *	val;
	uint16_t	len;
	uint8_t		flags;
} dhcp6_o_buf_opt_data_t, *dhcp6_o_buf_opt_data_p;


typedef struct dhcp6_options_buf_option_s {
	size_t		data_cnt;
	dhcp6_o_buf_opt_data_t data[1]; /* May be more than 1. */
} dhcp6_o_buf_opt_t, *dhcp6_o_buf_opt_p;

typedef struct dhcp6_options_buf_s {
	size_t			opts_cnt;
	dhcp6_o_buf_opt_p *	opts;
	dhcp6_o_buf_map_t	map;
} dhcp6_o_buf_t, *dhcp6_o_buf_p;

#define DHCP6_AO_BUF_O_F_IS_PTR		(((uint8_t)1) << 0) /* val is pointer to DHCP packet buffer part. */
#define DHCP6_AO_BUF_O_F_ALLOCATED	(((uint8_t)1) << 1) /* val is allocated and free() must be called. */


static void
dhcp6_o_buf_opt_clean(dhcp6_o_buf_p obuf, const uint16_t code) {
	dhcp6_o_buf_opt_p opt;

	if (NULL == obuf)
		return;
	if (code >= obuf->opts_cnt)
		return;
	opt = obuf->opts[code];
	if (NULL == opt)
		return;
	for (size_t j = 0; j < opt->data_cnt; j ++) {
		if (0 != (DHCP6_AO_BUF_O_F_ALLOCATED & opt->data[j].flags)) {
			free(opt->data[j].val);
		}
	}
	free(opt);
	obuf->opts[code] = NULL;
	dhcp6_o_buf_map_set(&obuf->map, code, 0);
}

static void
dhcp6_o_buf_clean(dhcp6_o_buf_p obuf) {
	dhcp6_o_buf_opt_p opt;

	if (NULL == obuf)
		return;
	for (size_t i = 0; i < obuf->opts_cnt; i ++) {
		opt = obuf->opts[i];
		if (NULL == opt)
			continue;
		for (size_t j = 0; j < opt->data_cnt; j ++) {
			if (0 != (DHCP6_AO_BUF_O_F_ALLOCATED & opt->data[j].flags)) {
				free(opt->data[j].val);
			}
		}
		free(opt);
	}
	free(obuf->opts);
	memset(obuf, 0x00, sizeof(dhcp6_o_buf_t));
}

static uint8_t *
dhcp6_o_buf_data_get_ptr(dhcp6_o_buf_p obuf, const uint16_t code, const size_t idx) {
	dhcp6_o_buf_opt_p opt;

	if (NULL == obuf)
		return (NULL);
	if (code >= obuf->opts_cnt)
		return (NULL);
	if (0 == dhcp6_o_buf_map_is_set(&obuf->map, code))
		return (NULL);
	opt = obuf->opts[code];
	if (idx >= opt->data_cnt)
		return (NULL);
	if (0 != ((DHCP6_AO_BUF_O_F_IS_PTR | DHCP6_AO_BUF_O_F_ALLOCATED) & opt->data[idx].flags))
		return (opt->data[idx].val);
	return ((uint8_t*)&opt->data[idx].val);
}

#define DHCP6_AO_BUF_O_DATA_ADD_F_EXT_PTR	(((uint32_t)1) << 0) /* Point to external mem. */
#define DHCP6_AO_BUF_O_DATA_ADD_F_IMPORT_MEM	(((uint32_t)1) << 1) /* Import ptr. */
#define DHCP6_AO_BUF_O_DATA_ADD_F_ALL		(DHCP6_AO_BUF_O_DATA_ADD_F_EXT_PTR | DHCP6_AO_BUF_O_DATA_ADD_F_IMPORT_MEM)
static int
dhcp6_o_buf_data_add(dhcp6_o_buf_p obuf, const uint32_t flags, const uint16_t code,
    const uint8_t *val, const size_t size) {
	void *ptr;
	dhcp6_o_buf_opt_p opt;

	if (NULL == obuf ||
	    DHCP6_AO_BUF_O_DATA_ADD_F_ALL == (DHCP6_AO_BUF_O_DATA_ADD_F_ALL & flags))
		return (EINVAL);

	if (code >= obuf->opts_cnt) {
		ptr = reallocarray(obuf->opts, (((size_t)code) + 1), sizeof(dhcp6_o_buf_opt_p));
		if (NULL == ptr)
			return (ENOMEM);
		memset(((uint8_t*)ptr) + (sizeof(dhcp6_o_buf_opt_p) * obuf->opts_cnt),
		    0x00, (sizeof(dhcp6_o_buf_opt_p) * ((((size_t)code) + 1) - obuf->opts_cnt)));
		obuf->opts = ptr;
		obuf->opts_cnt = (((size_t)code) + 1);
	}

	/* Option data storage management. */
	if (NULL == obuf->opts[code]) {
		opt = calloc(1, sizeof(dhcp6_o_buf_opt_t));
	} else {
		opt = realloc(obuf->opts[code], (sizeof(dhcp6_o_buf_opt_t) +
		    (sizeof(dhcp6_o_buf_opt_data_t) * obuf->opts[code]->data_cnt)));
	}
	if (NULL == opt)
		return (ENOMEM);
	obuf->opts[code] = opt;

	if (sizeof(opt->data[opt->data_cnt].val) >= size) { /* Can we store value locally without allocation? */
		memcpy(&opt->data[opt->data_cnt].val, val, size);
		opt->data[opt->data_cnt].flags = 0;
		if (0 != (DHCP6_AO_BUF_O_DATA_ADD_F_IMPORT_MEM & flags)) {
			free((uint8_t*)val);
		}
	} else if (0 != (DHCP6_AO_BUF_O_DATA_ADD_F_IMPORT_MEM & flags)) {
		opt->data[opt->data_cnt].val = (uint8_t*)val;
		opt->data[opt->data_cnt].flags = DHCP6_AO_BUF_O_F_ALLOCATED;
	} else if (0 != (DHCP6_AO_BUF_O_DATA_ADD_F_EXT_PTR & flags)) { /* Point to external mem. */
		opt->data[opt->data_cnt].val = (uint8_t*)val;
		opt->data[opt->data_cnt].flags = DHCP6_AO_BUF_O_F_IS_PTR;
	} else { /* Make a copy. */
		opt->data[opt->data_cnt].val = malloc((size + sizeof(void*)));
		if (NULL == opt->data[opt->data_cnt].val)
			return (ENOMEM);
		memcpy(opt->data[opt->data_cnt].val, val, size);
		opt->data[opt->data_cnt].flags = DHCP6_AO_BUF_O_F_ALLOCATED;
	}
	opt->data[opt->data_cnt].len = (uint16_t)size;
	opt->data_cnt ++;
	dhcp6_o_buf_map_set(&obuf->map, code, 1);

	return (0);
}


static size_t
dhcp6_o_buf_calc_size(dhcp6_o_buf_p obuf, const dhcp6_o_buf_map_p allow_filter) {
	size_t opt_seq_size = 0;
	dhcp6_o_buf_opt_p opt;
	dhcp6_o_buf_map_t flt;

	if (NULL == obuf)
		return (opt_seq_size);

	/* Apply allow filter or use only set map. */
	if (NULL != allow_filter) {
		dhcp6_o_buf_map_and(&flt, &obuf->map, allow_filter);
	} else {
		memcpy(&flt, &obuf->map, sizeof(flt));
	}
	/* Calc size. */
	for (size_t code = 0; code < obuf->opts_cnt; code ++) {
		if (0 == (code % 8) && 0 == flt.u8[(code / 8)]) { /* Fast skip. */
			code += 7; /* +1 in for() = 8. */
			continue;
		}
		if (0 == dhcp6_o_buf_map_is_set(&flt, (uint16_t)code))
			continue;
		opt = obuf->opts[code];
		for (size_t j = 0; j < opt->data_cnt; j ++) {
			opt_seq_size += sizeof(dhcp6_opt_hdr_t) + opt->data[j].len; /* Option header + data as is. */
		}
	}

	return (opt_seq_size);
}

static int
dhcp6_o_buf_serialize(dhcp6_o_buf_p obuf, const dhcp6_o_buf_map_p allow_filter,
    uint8_t *buf, const size_t buf_size, size_t *buf_written) {
	uint8_t *buf_max = (buf + buf_size), *ptr = buf;
	size_t opt_size, opt_len;
	dhcp6_o_buf_opt_p opt;
	dhcp6_o_buf_map_t flt;

	if (NULL == obuf ||
	    (NULL == buf && 0 != buf_size) ||
	    NULL == buf_written)
		return (EINVAL);

	/* Apply allow filter or use only set map. */
	if (NULL != allow_filter) {
		dhcp6_o_buf_map_and(&flt, &obuf->map, allow_filter);
	} else {
		memcpy(&flt, &obuf->map, sizeof(flt));
	}
	/* Write options. */
	for (size_t code = 0; code < obuf->opts_cnt; code ++) {
		if (0 == (code % 8) && 0 == flt.u8[(code / 8)]) { /* Fast skip. */
			code += 7; /* +1 in for() = 8. */
			continue;
		}
		if (0 == dhcp6_o_buf_map_is_set(&flt, (uint16_t)code))
			continue;
		opt = obuf->opts[code];
		if (NULL == opt) { /* Empty opt. */
			if ((ptr + sizeof(dhcp6_opt_hdr_t)) <= buf_max) {
				/* Write option header and data. */
				dhcp6_htons((uint16_t)code, &ptr[0]);
				dhcp6_htons((uint16_t)0, &ptr[2]);
			}
			ptr += sizeof(dhcp6_opt_hdr_t);
			continue;
		}
		for (size_t j = 0; j < opt->data_cnt; j ++) {
			opt_len = opt->data[j].len;
			opt_size = sizeof(dhcp6_opt_hdr_t) + opt_len; /* Option header + data as is. */
			if ((ptr + opt_size) <= buf_max) {
				/* Write option header and data. */
				dhcp6_htons((uint16_t)code, &ptr[0]);
				dhcp6_htons((uint16_t)opt_len, &ptr[2]);
				memcpy((ptr + sizeof(dhcp6_opt_hdr_t)),
				    dhcp6_o_buf_data_get_ptr(obuf, (uint16_t)code, j), opt_len);
			}
			ptr += opt_size;
		}
	}

	(*buf_written) = (size_t)(ptr - buf);
	if (ptr > buf_max)
		return (ENOBUFS);
	return (0);
}

static int
dhcp6_o_buf_process(const void *buf, const size_t buf_size,
    dhcp6_opt_params_p opts, const size_t opts_count,
    const int use_ptr, dhcp6_o_buf_p obuf) {
	int error;
	const uint8_t *pos = buf, *pos_max = (((const uint8_t*)buf) + buf_size), *opt_data;
	dhcp6_opt_params_p optp;
	uint16_t opt_code, opt_len, olen;

	if (NULL == buf || NULL == obuf)
		return (EINVAL);

	while (pos < pos_max) {
		/* Option header: code + len. */
		opt_code = dhcp6_ntohs(&pos[0]);
		opt_len = dhcp6_ntohs(&pos[2]);

		/* Get best dhcp6_opt_params for current option code. */
		if (opt_code < opts_count) { /* Known/defined option. */
			optp = &opts[opt_code];
		} else { /* Option is unknown. */
			optp = (dhcp6_opt_params_p)&dhcp6_opt_params_unknown;
		}

		/* Process option lenght. */
		opt_data = (pos + sizeof(dhcp6_opt_hdr_t));
		/* Is lenght in buf range? */
		if ((opt_data + opt_len) > pos_max)
			return (EBADMSG);
		/* Move pointer to next option. */
		pos += (sizeof(dhcp6_opt_hdr_t) + opt_len);

		/* Extra lenght check depend on option type knowlege. */
		/* Fixed size. */
		olen = dhcp6_opt_type2size_fixed[optp->type];
		if (UINT16_MAX == olen &&
		    0 != (DHCP6_OPTP_F_FIXEDLEN & optp->flags)) { /* Not from type. */
			olen = optp->len;
		}
		if (UINT16_MAX != olen) {
			if (0 != (DHCP6_OPTP_F_ARRAY & optp->flags)) { /* Lenght must be multiple to specified fixedlen of 1 element. */
				if (opt_len < olen ||
				    0 != (opt_len % olen))
					return (EBADMSG);
			} else { /* Lenght must be equal to specified fixedlen. */
				if (opt_len != olen)
					return (EBADMSG);
			}
		} else { /* Min size. */
			if (0 != (DHCP6_OPTP_F_MINLEN & optp->flags)) { /* Use provided val. */
				olen = optp->len;
			} else {
				olen = dhcp6_opt_type2size_min[optp->type];
			}
			if (UINT16_MAX != olen && opt_len < olen)
				return (EBADMSG);
		}

		/* Check for singleton. */
		if (0 != dhcp6_o_buf_map_is_set(&obuf->map, opt_code) &&
		    0 == (DHCP6_OPTP_F_MULTI & optp->flags))
			return (EBADMSG);

		/* Store option data to opts buf. */
		error = dhcp6_o_buf_data_add(obuf,
		    ((0 != use_ptr) ? DHCP6_AO_BUF_O_DATA_ADD_F_EXT_PTR : 0),
		    opt_code, opt_data, opt_len);
		if (0 != error)
			return (error);
	}

	return (0);
}


/* Check DHCP packet header and do options aggregate.
 * buf - pointer to mem with DHCP packet.
 * buf_size - DHCP packet size.
 * obuf - aggregate options buf. Do not forget to call dhcp6_o_buf_clean() after use.
 * Return 0 if no err.
 */
static int
dhcp6_pkt_chk_opts_aggregate(const void *buf, const size_t buf_size,
    const int use_ptr, dhcp6_o_buf_p obuf) {
	int error;
	const dhcp6_hdr_p hdr = (const dhcp6_hdr_p)buf;

	if (NULL == buf || NULL == obuf)
		return (EINVAL);

	/* Parse main options. */
	dhcp6_o_buf_clean(obuf);
	error = dhcp6_o_buf_process(
	    (const void*)(hdr + 1), (buf_size - sizeof(dhcp6_hdr_t)),
	    (dhcp6_opt_params_p)dhcp6_options, nitems(dhcp6_options),
	    use_ptr, obuf);
	if (0 != error)
		goto err_out;

	return (0);

err_out:
	dhcp6_o_buf_clean(obuf);
	return (error);
}


#endif /* __DHCP6_MESSAGE_H__ */
