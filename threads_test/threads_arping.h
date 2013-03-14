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

typedef char                        CHAR;
typedef unsigned char               UCHAR;
typedef int32_t                     INT;
typedef uint32_t                    UINT;
typedef long                        LONG;
typedef unsigned long               ULONG;
typedef int64_t                     LONG_LONG;
typedef uint64_t                    ULON_GLONG;
typedef int8_t                      INT8;
typedef uint8_t                     UINT8;
typedef int16_t                     INT16;
typedef uint16_t                    UINT16;
typedef int32_t                     INT32;
typedef uint32_t                    UINT32;
typedef int64_t                     INT64;
typedef uint64_t                    UINT64;
typedef void                        VOID;
typedef float                       FLOAT;
typedef double                      DOUBLE;

#define DEBUGERROR_ARPING(message, args...) printf("Line %4d in %s: --"message"\n", __LINE__, __FILE__, ##args);
#define DEBUGINFO_ARPING(message, args...)  printf("Line %4d in %s: --"message"\n", __LINE__, __FILE__, ##args);
//#define DEBUGINFO_ARPING(message, args...)
#define INOUT
#define IN
#define OUT
#define MAC_BCAST_ADDR         (unsigned char *) "\xff\xff\xff\xff\xff\xff"

typedef struct _DEVICE_STRUCT
{
    char interface[32];
    UINT32 local_ip;
    char local_mac[32];
} DEVICE_STRUCT;
enum ARPING_STATUS {ARPING_ONLINE=0, ARPING_OFFLINE, ARPING_ERR};
typedef struct _ARPING_STRUCT
{
    UINT32 remote_ip;
    char *remote_mac;
    enum ARPING_STATUS status;
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
struct DEVICE_ARP_MSG
{
    struct ethhdr   ethhdr;         /* Ethernet header                          */
    UINT16      htype;          /* hardware type (must be ARPHRD_ETHER)     */
    UINT16      ptype;          /* protocol type (must be ETH_P_IP)         */
    UCHAR       hlen;           /* hardware address length (must be 6)      */
    UCHAR       plen;           /* protocol address length (must be 4)      */
    UINT16      operation;      /* ARP opcode                               */
    UCHAR       sHaddr[6];      /* sender's hardware address                */
    UCHAR       sInaddr[4];     /* sender's IP address                      */
    UCHAR       tHaddr[6];      /* target's hardware address                */
    UCHAR       tInaddr[4];     /* target's IP address                      */
    UCHAR       pad[18];        /* pad for min. Ethernet payload (60 bytes) */
};
#endif
