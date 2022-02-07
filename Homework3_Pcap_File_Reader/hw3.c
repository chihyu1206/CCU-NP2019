#include <stdio.h>
#include <pcap.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUFSIZE 4096
#define SIZE_ETHERNET 14
#define ETHER_ADDR_LEN 6

/* Ethernet header */
struct sniff_ethernet {
    u_char ether_dhost[ETHER_ADDR_LEN];   // Destination host address
    u_char ether_shost[ETHER_ADDR_LEN];   // Source host address
    u_short ether_type;           // IP, ARP, RARP, etc
};

/* IP header */
struct sniff_ip {
    u_char ip_vhl;                /* version << 4 | header length >> 2 */
    u_char ip_tos;                /* type of service */
    u_short ip_len;               /* total length */
    u_short ip_id;                /* identification */
    u_short ip_off;               /* fragment offset field */
#define IP_RF 0x8000            /* reserved fragment flag */
#define IP_DF 0x4000            /* dont fragment flag */
#define IP_MF 0x2000            /* more fragments flag */
#define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    u_char ip_ttl;                /* time to live */
    u_char ip_p;                  /* protocol */
    u_short ip_sum;               /* checksum */
    struct in_addr ip_src, ip_dst;        /* source and dest address */
};
#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip) (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct sniff_tcp {
    u_short th_sport;             /* source port */
    u_short th_dport;             /* destination port */
    tcp_seq th_seq;               /* sequence number */
    tcp_seq th_ack;               /* acknowledgement number */
    u_char th_offx2;              /* data offset, rsvd */
#define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
    u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    u_short th_win;               /* window */
    u_short th_sum;               /* checksum */
    u_short th_urp;               /* urgent pointer */
};

pcap_t *handle;
bool isSend = false;

void packet_printer(const u_char * packet, struct pcap_pkthdr header) {
    const struct sniff_ethernet *ethernet;        /* The ethernet header */
    const struct sniff_ip *ip;    /* The IP header */
    const struct sniff_tcp *tcp;  /* The TCP header */
    char *payload;                /* Packet payload */
    u_int size_ip;
    u_int size_tcp;

    ethernet = (struct sniff_ethernet *) (packet);
    ip = (struct sniff_ip *) (packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip) * 4;
    if (size_ip < 20) {
        printf("*Invalid IP header length: %u bytes*\n", size_ip);
        return;
    }
    tcp = (struct sniff_tcp *) (packet + SIZE_ETHERNET + size_ip);
    size_tcp = TH_OFF(tcp) * 4;
    if (size_tcp < 20) {
        printf("*Invalid TCP header length: %u bytes*\n", size_tcp);
        return;
    }
    payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);

    //prevent inet_ntoa rewrite buffer
    char src[BUFSIZE], dst[BUFSIZE];
    strcpy(src, inet_ntoa(ip->ip_src));
    strcpy(dst, inet_ntoa(ip->ip_dst));

    //transfer timestamp to printable type
    time_t time;
    struct tm *tm;
    char timebuf[BUFSIZE];
    time = header.ts.tv_sec;
    tm = localtime(&time);
    /* size_t strftime(char *s, size_t max, const char *format,
       const struct tm *tm); 
    */
    strftime(timebuf, BUFSIZE, "%Y-%m-%d %H:%M:%S", tm);  //yyyy-mm-dd hh-mm-ss

    //print information
    printf("Src IP:%s\nDst IP:%s\nSrc Port:%hu\nDst Port:%hu\nPacket Size:%hu bytes\nPacket Time:%s\n\n",
       src, dst, tcp->th_sport, tcp->th_dport, header.len, timebuf);
}

void packet_handler(u_char * args, const struct pcap_pkthdr *header, const u_char * packet) {
    packet_printer(packet, *header);
}

int main(int argc, char *argv[]) {
    char *device = NULL;
    char errorbuf[BUFSIZE];
    int timeout = 1000;
    struct bpf_program filter;
    char filter_exp[BUFSIZE];
    bpf_u_int32 subnet_mask, ip;

    //-p:read the pcap file -f:set the filter
    char chr;
    while ((chr = getopt(argc, argv, "p:f:s")) != -1) {
        switch (chr) {
            case 'p': {
                printf("file=%s\n", optarg);
                handle = pcap_open_offline(optarg, errorbuf);
                break;
            }  
            case 'f': {
                optind--;
                for (; optind < argc; optind++) {
                    if (argv[optind][0] == '-')
                        break;
                    strcat(filter_exp, argv[optind]);
                    strcat(filter_exp, " ");
                }
                printf("Filter=%s\n", filter_exp);
                break;
            }
            default: {
                printf("Warning:WRONG COMMAND!!\n");
                return 0;
            }
        }
  }
    //set filter
    if (pcap_compile(handle, &filter, filter_exp, 0, ip) == -1) {
        printf("Bad filter - %s\n", pcap_geterr(handle));
        exit(-1);
    }
    if (pcap_setfilter(handle, &filter) == -1) {
        printf("Error setting filter - %s\n", pcap_geterr(handle));
        exit(-1);
    }

    printf("---Start sniffing---\n");
    //get packet
    pcap_loop(handle, 0, packet_handler, NULL);
    printf("---End Sniffing---\n");
    pcap_close(handle);

    return 0;
}
