#ifndef __APINGT_TEST__
#define __APINGT_TEST__

#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <linux/if_arp.h>

typedef char                        CGR_CHAR;
typedef unsigned char               CGR_UCHAR;
typedef int32_t                     CGR_INT;
typedef uint32_t                    CGR_UINT;
typedef long                        CGR_LONG;
typedef unsigned long               CGR_ULONG;
typedef int64_t                     CGR_LONG_LONG;
typedef uint64_t                    CGR_ULON_GLONG;
typedef int8_t                      CGR_INT8;
typedef uint8_t                     CGR_UINT8;
typedef int16_t                     CGR_INT16;
typedef uint16_t                    CGR_UINT16;
typedef int32_t                     CGR_INT32;
typedef uint32_t                    CGR_UINT32;
typedef int64_t                     CGR_INT64;
typedef uint64_t                    CGR_UINT64;
typedef void                        CGR_VOID;
typedef float                       CGR_FLOAT;
typedef double                      CGR_DOUBLE;

typedef CGR_CHAR                   *PCGR_CHAR;
typedef CGR_UCHAR                  *PCGR_UCHAR;
typedef CGR_INT                    *PCGR_INT;
typedef CGR_UINT                   *PCGR_UINT;
typedef CGR_LONG                   *PCGR_LONG;
typedef CGR_ULONG                  *PCGR_ULONG;
typedef CGR_LONG_LONG              *PCGR_LONG_LONG;
typedef CGR_ULON_GLONG             *PCGR_ULON_GLONG;
typedef CGR_INT8                   *PCGR_INT8;
typedef CGR_UINT8                  *PCGR_UINT8;
typedef CGR_INT16                  *PCGR_INT16;
typedef CGR_INT32                  *PCGR_INT32;
typedef CGR_UINT32                 *PCGR_UINT32;
typedef CGR_UINT64                 *PCGR_UINT64;
typedef CGR_VOID                   *PCGR_VOID;
typedef CGR_FLOAT                  *PCGR_FLOAT;
typedef CGR_DOUBLE                 *PCGR_DOUBLE;
#define DEBUGERROR_DEVICE(message, args...) printf("Line %4d in %s: --"message"\n", __LINE__, __FILE__, ##args);
//#define DEBUGINFO_DEVICE(message, args...)  printf("Line %4d in %s: --"message"\n", __LINE__, __FILE__, ##args);
//#define DEBUGERROR_DEVICE(message, args...)
#define DEBUGINFO_DEVICE(message, args...)
#define INOUT
#define IN
#define OUT
#define MAC_BCAST_ADDR         (unsigned char *) "\xff\xff\xff\xff\xff\xff"

typedef struct _DEVICE_STRUCT
{
    char interface[32];
    CGR_UINT32 local_ip;
    char local_mac[32];
} DEVICE_STRUCT;
typedef struct _ARPING_STRUCT
{
    CGR_UINT32 remote_ip;
    char *remote_mac;
    int status;
    int thread_status;
    DEVICE_STRUCT *pdevice;
} ARPING_STRUCT;
typedef struct _IP_MAC
{
    char s_ip[32];
    char s_mac[32];
    int  status;
} IP_MAC;
/* ARP message */
struct CGR_DEVICE_ARP_MSG
{
    struct ethhdr   ethhdr;         /* Ethernet header                          */
    CGR_UINT16      htype;          /* hardware type (must be ARPHRD_ETHER)     */
    CGR_UINT16      ptype;          /* protocol type (must be ETH_P_IP)         */
    CGR_UCHAR       hlen;           /* hardware address length (must be 6)      */
    CGR_UCHAR       plen;           /* protocol address length (must be 4)      */
    CGR_UINT16      operation;      /* ARP opcode                               */
    CGR_UCHAR       sHaddr[6];      /* sender's hardware address                */
    CGR_UCHAR       sInaddr[4];     /* sender's IP address                      */
    CGR_UCHAR       tHaddr[6];      /* target's hardware address                */
    CGR_UCHAR       tInaddr[4];     /* target's IP address                      */
    CGR_UCHAR       pad[18];        /* pad for min. Ethernet payload (60 bytes) */
};
#endif
