#include "threads_pool.h"
#include "threads_arping.h"

CGR_CHAR *CgrStrNCpy
(
INOUT CGR_CHAR                 *s1,
IN    const CGR_CHAR           *s2,
IN    size_t                    n
)
{
    strncpy(s1, s2, n);
    s1[n-1] = '\0';

    return s1;
}

static void CgrDeviceConvertMacStringToHex
(
    CGR_CHAR       *pMac,
    CGR_UCHAR      *pHex
)
{
    CGR_CHAR       *p;
    CGR_UCHAR      *q;
    CGR_UCHAR       ucH;
    CGR_UCHAR       ucL;
    CGR_INT         i;

    for (p = pMac, q = pHex, i = 0; i < 6; i++)
    {
        /* Convert numeric MAC string to hex mac */
        ucH = *p++;
        ucL = *p++;

        if (ucH >= 'a')
        {
            ucH = ucH - 'a' + 0xa;
        }
        else if (ucH >= 'A')
        {
            ucH = ucH - 'A' + 0xa;
        }
        else
        {
            ucH -= '0';
        }

        if (ucL >= 'a')
        {
            ucL = ucL - 'a' + 0xa;
        }
        else if (ucL >= 'A')
        {
            ucL = ucL - 'A' + 0xa;
        }
        else
        {
            ucL -= '0';
        }

        *q++ = ((ucH << 4) & 0xf0) | (ucL & 0x0f);

        if (*p == '\0')
        {
            break;
        }
        
        p++; /* skip ':' */
    }
}
static CGR_INT CgrDeviceArping
(
    void       *parping_args
)
{
    ARPING_STRUCT    *arping_struct = (ARPING_STRUCT *)parping_args;
    char * pRemoteMac = arping_struct->remote_mac;
    CGR_UINT32 nRemoteIp = htonl(arping_struct->remote_ip);
    char *pLocalMac = arping_struct->pdevice->local_mac;
    CGR_UINT32 nLocalIp = htonl(arping_struct->pdevice->local_ip);
    char *pInterface = arping_struct->pdevice->interface;
    
    arping_struct->thread_status = 2;//thread working mark
    
    thread_debug("######################MAC:%s INFC:%s", pLocalMac, pInterface);
    thread_debug("######################RMOTE_IP:%u LOCAL_IP:%u", nRemoteIp, nLocalIp);
    CGR_INT                     optval = 1;
    CGR_INT                     s;          /* socket fd */
    CGR_INT                     rv = 1;     /* return value */
    CGR_INT                     i;
    CGR_UCHAR                   usRemoteMacHex[6];
    CGR_UCHAR                   usLocalMacHex[6];
    CGR_UINT32                  ui;
    
    struct sockaddr             addr;       /* for interface name */
    fd_set                      fdset;
    struct timeval              tm;   
    struct CGR_DEVICE_ARP_MSG   arp;

    if ((s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1)
    {
        thread_error("Could not open raw socket");
        return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1)
    {
        thread_error("Could not setsocketopt on raw socket");
        close(s);
        return -1;
    }

    /* Prepare ARP request */
    memset(&arp, 0, sizeof(arp));
    memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);                       /* Destination hardware address */
    CgrDeviceConvertMacStringToHex(pLocalMac, usLocalMacHex);
    memcpy(arp.ethhdr.h_source, usLocalMacHex, 6);                      /* MAC SA                       */
    arp.ethhdr.h_proto  = htons(ETH_P_ARP);                             /* Protocol type (Ethernet)     */
    arp.htype           = htons(ARPHRD_ETHER);                          /* Hardware type                */
    arp.ptype           = htons(ETH_P_IP);                              /* Protocol type (ARP message)  */
    arp.hlen            = 6;                                            /* Hardware address length      */
    arp.plen            = 4;                                            /* Protocol address length      */
    arp.operation       = htons(ARPOP_REQUEST);                         /* ARP op code                  */
    memcpy((void *)&arp.sInaddr, (void *)&nLocalIp, sizeof(u_int32_t)); /* Souce IP address             */
    memcpy(arp.sHaddr, usLocalMacHex, 6);                               /* Source hardware address      */
    memcpy((void *)&arp.tInaddr, (void *)&nRemoteIp, sizeof(u_int32_t));/* Destination IP address       */
    memset(&addr, 0, sizeof(addr));
    CgrStrNCpy(addr.sa_data, pInterface, 14);

    /* Send arp request */    
    if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
    {
        thread_error("Error on sending ARP request");
        rv = -1;
        goto CgrDeviceArpingExit;
    }

    tm.tv_usec = 300000;                                                 /* Wait 100msecs                */
    tm.tv_sec = 0;
    FD_ZERO(&fdset);
    FD_SET(s, &fdset);
    i = select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm);

    if (i < 0)
    {
        thread_error("Error on select");
        rv = -1;
        arping_struct->status = 0;
        goto CgrDeviceArpingExit;
    }
    else if (i == 0)
    {
        /* Time out */
        thread_debug("Not receive ARPING response in time");
        rv = -1;
        arping_struct->status = 0;
        goto CgrDeviceArpingExit;
    }
    else if (FD_ISSET(s, &fdset))
    {
        if (recv(s, &arp, sizeof(arp), 0) < 0 )
        {
            rv = -1;
            arping_struct->status = 0;
            goto CgrDeviceArpingExit;
        }

        /* arp.sInaddr might have aligment issue, so use a stack unsigned integer avariable 'ui' instead */
        memcpy(&ui, arp.sInaddr, sizeof(CGR_UINT32));

        CgrDeviceConvertMacStringToHex(pRemoteMac, usRemoteMacHex);

        if (arp.operation                       == htons(ARPOP_REPLY)   &&            
            ui                                  == nRemoteIp            &&  /* Compare remote IP  */
            bcmp(arp.tHaddr, usLocalMacHex, 6)  == 0                    &&  /* Compare local MAC  */
            bcmp(arp.sHaddr, usRemoteMacHex, 6) == 0)                       /* Compare remote MAC */
        {
            thread_debug("Valid arp reply receved for this address");
            rv = 0;
            arping_struct->status = 1;
        }
    }

CgrDeviceArpingExit:
    close(s);
    arping_struct->thread_status = 1;
    return rv;
}
static GetRemoteIpMac(IP_MAC *s_ip_mac)
{
    FILE *fp;
    int fsize;
    int count = 0;
    char *arp_data;
    char *pfp;
    char sHwType[16];
    char sFlagsMask[32];
    char sInterface[16];
    system("/sbin/arp -n > ./data.arp");
    fp = fopen("./data.arp", "r");
    fseek (fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    arp_data = (char *)alloca(fsize + 1);
    fread(arp_data, 1, fsize, fp);
    arp_data[fsize] = '\0';
    pfp = arp_data;
    for(; *pfp != '\0';)
    {
        if(*pfp == '\n' || *pfp == '\r')
        {
            *pfp = '\0';
        }
        pfp++;
    }
    pfp = arp_data;
    for(; *pfp != '\0';)
    {
        pfp++;
    }
    while(1)
    {
        for(; *pfp != '\0';)
        {
            pfp++;
        }
        pfp++;
        if(*pfp == '\0')
        {
            break;
        }
        sscanf(pfp, "%s %s %s %s %s", s_ip_mac[count].s_ip, sHwType, s_ip_mac[count].s_mac, sFlagsMask, sInterface);
		thread_debug("get remote IP:%s MAC:%s", s_ip_mac[count].s_ip, s_ip_mac[count].s_mac);
        s_ip_mac[count].status = 1;
        count++;
    }
}
int GetLocalMacAndIp(char *iface_name, char *local_mac, char *local_ip)
{
	FILE *pfp = NULL;
	char *ptmp = NULL;
	char syscmd[128];
	char tmpbuffer[128], *tmp;

	memset(syscmd, '\0', sizeof(syscmd));
	memset(tmpbuffer, '\0', sizeof(tmpbuffer));
	sprintf(syscmd, "ifconfig %s", iface_name);
	if((pfp = popen(syscmd, "r")) == NULL)
	{
		thread_debug("syscmd:%s failed", syscmd);
		return -1;
	}
	fgets(tmpbuffer, sizeof(tmpbuffer), pfp);
	sscanf(tmpbuffer, "%*s%*s%*s%*s%s", local_mac);
	thread_debug("local mac: %s", local_mac);
	//strcpy(local_mac, local_ip);
	fgets(tmpbuffer, sizeof(tmpbuffer), pfp);
	sscanf(tmpbuffer, "%*s%s", local_ip);
	strcpy(tmpbuffer, local_ip);
	tmp = strchr(tmpbuffer, ':');
	++tmp;
	strcpy(local_ip, tmp);
	thread_debug("local ip:%s local mac: %s", local_ip, local_mac);
	pclose(pfp);
	return 0;


}
void main(int argc, char *argv[])
{
	char local_mac[32], local_ip[32];
    struct timeval begin_time[10000];
    struct timeval end_time[10000];
    int test_count = 10;

	memset(local_mac, 0, sizeof(local_mac));
	memset(local_ip, 0, sizeof(local_ip));
    memset(begin_time, 0, sizeof(begin_time));
    memset(end_time, 0, sizeof(end_time));
    
    IP_MAC s_ip_mac[100];
    ARPING_STRUCT arping_ifo[100];
    DEVICE_STRUCT local_info;
    struct in_addr      oInAddr;
   thread_debug("argc=%d", argc); 
	if(argc != 3 || strcmp(argv[1], "-i"))
	{
		printf("usage: -i interface name\n");
		exit(-1);
	}
	if(GetLocalMacAndIp(argv[2], local_mac, local_ip))
	{
		printf("interface name error!!!");
		exit(-1);
	}

    THREAD_POOL *pool = creat_thread_pool(3, 50);
    init_thread_pool(pool);
    void (*job)(void *);
    job = CgrDeviceArping;
    
    memset(s_ip_mac, 0,sizeof(s_ip_mac));
    memset(arping_ifo, 0,sizeof(arping_ifo));
    memset(&local_info, 0,sizeof(local_info));
    GetRemoteIpMac(s_ip_mac);
    inet_aton(local_ip, &oInAddr);
    sprintf(local_info.interface, argv[2]);
    local_info.local_ip = ntohl(oInAddr.s_addr);
    //local_info.local_ip = htonl(oInAddr.s_addr);
    sprintf(local_info.local_mac, local_mac);
while(test_count--)
{
    printf("test count=%d\n", test_count);   
    gettimeofday(&begin_time[test_count], NULL);
    int count;
    for(count = 0; s_ip_mac[count].status == 1; count++)
    {
        if(!inet_aton(s_ip_mac[count].s_ip, &oInAddr))
        {
            //printf("the ip: %s error!!!\n", s_ip_mac[count].s_ip);
            return;
        }
        arping_ifo[count].remote_mac = s_ip_mac[count].s_mac;
        arping_ifo[count].remote_ip  = ntohl(oInAddr.s_addr);
        //arping_ifo[count].remote_ip  = htonl(oInAddr.s_addr);
        arping_ifo[count].pdevice = &local_info;
        //thread_debug("Add____job_____MAC:%s\n",  arping_ifo[count].remote_mac);
        //add_job(pool, job, (void *)(&arping_ifo[count]));

    }
    //printf("_____________add job end\n");
    for(count = 0; s_ip_mac[count].status == 1; count++)
    {
		
        add_job(pool, job, (void *)(&arping_ifo[count]));
        //CgrDeviceArping((void *)(&arping_ifo[count]));
		thread_debug("Add____job_____MAC:%s\n",  arping_ifo[count].remote_mac);
    }
    while(1)
    {
		printf("Goto while\n\n\n");
        for(count = 0;s_ip_mac[count].status == 1; count++)
        {

            printf("The arping MAC:%s thread status%d\n", arping_ifo[count].remote_mac, arping_ifo[count].thread_status);
        }
		for(count = 0;s_ip_mac[count].status == 1; count++)
        {

            if(arping_ifo[count].thread_status != 1)
            {
                goto THREAD_CHECK;
            }
        }
        for(count = 0;s_ip_mac[count].status == 1; count++)
        {
            if(arping_ifo[count].status == 1)
            {
                printf("the %s computer is online\n", arping_ifo[count].remote_mac);
            }
            else
            {
                printf("the %s computer is offline\n", arping_ifo[count].remote_mac);
            }
        }
        gettimeofday(&end_time[test_count], NULL);
        break;
        THREAD_CHECK:
		//printf("Goto Sleep\n");
        usleep(10000);
        continue;
        
    }
    //close_thread_pool(pool);
	sleep(1);
}
for(test_count = 10; test_count > 0; test_count--)
{
    unsigned int seconds, mro_seconds;
    printf("test count=%d\n", test_count);
    if(end_time[test_count - 1].tv_usec - begin_time[test_count - 1].tv_usec < 0)
    {
        mro_seconds = 1000 * 1000 - begin_time[test_count - 1].tv_usec + end_time[test_count - 1].tv_usec;
        seconds = end_time[test_count - 1].tv_sec - begin_time[test_count - 1].tv_sec - 1;
    }
    else
    {
        mro_seconds = end_time[test_count - 1].tv_usec - begin_time[test_count - 1].tv_usec;
        seconds = end_time[test_count - 1].tv_sec - begin_time[test_count - 1].tv_sec;
    }
    printf("Test %d wast time %u:%u\n", 10 - test_count, seconds, mro_seconds);
}
printf("test count=%d\n", test_count);

}
