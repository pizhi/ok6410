/*
		ʵ�鲽��
//��ʼ��dm900
//��ݰ���
//��ݰ����	

*/

#include "dm9000.h"
#include "arp.h"

#define DM_ADD (*((volatile unsigned short *)0x18000000))
#define DM_DAT (*((volatile unsigned short *)0x18000004))

/*Register*/
#define MEM_SYS_CFG	(*(volatile unsigned *)0x7E00F120)
#define SROM_BW		(*(volatile unsigned *)0x70000000)
#define SROM_BC1	(*(volatile unsigned *)0x70000008)
#define GPNCON		(*(volatile unsigned *)0x7F008830)
#define EINT0CON0	(*(volatile unsigned *)0x7F008900)
#define EINT0MASK	(*(volatile unsigned *)0x7F008920)
#define EINT0PEND	(*(volatile unsigned *)0x7F008924)
#define VIC0INTENABLE	(*(volatile unsigned *)0x71200010)
#define EINT7_VECTADDR	(*(volatile unsigned *)0x71200104)
#define VIC0ADDRESS	*((volatile unsigned int *)0x71200f00)   
#define VIC1ADDRESS	*((volatile unsigned int *)0x71300f00)


u8 buffer[1500];

u8 host_mac_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
u8 mac_addr[6] = {9,8,7,6,5,4};
u8 ip_addr[4] = {10,5,114,107};
u8 host_ip_addr[4] = {10,5,114,109};
u16 packet_len;


void cs_init()
{
//	MEM_SYS_CFG
	SROM_BW &= (~(1<<4));								//����λ���
	SROM_BW |= (1<<4);
	SROM_BC1 = (0x0<<28)|(0x0<<24)|(0x7<<16)|(0x1<<12)|(0x0<<8)|(0x0<<4)|(0x0<<0);	//����ʱ��	
}

void int_init()
{
	GPNCON &= (~(0x3<<14));
	GPNCON |= (0x2<<14);

//	EINT0PEND &= (~(0x1<<7));
//	EINT0PEND |= (0x1<<7);
}

void dm9000_reg_write(u16 reg,u16 data)
{
	DM_ADD = reg;	
	DM_DAT = data;	
}

u8 dm9000_reg_read(u16 reg)
{
	DM_ADD = reg;
	return DM_DAT;	
}

void dm9000_reset()
{
    dm9000_reg_write(DM9000_GPCR, GPCR_GPIO0_OUT);
    dm9000_reg_write(DM9000_GPR, 0);	
    
    dm9000_reg_write(DM9000_NCR, (NCR_LBK_INT_MAC | NCR_RST));
    dm9000_reg_write(DM9000_NCR, 0);
    
    dm9000_reg_write(DM9000_NCR, (NCR_LBK_INT_MAC | NCR_RST));
    dm9000_reg_write(DM9000_NCR, 0);
}

void dm9000_probe(void)
{
	u32 id_val;
	id_val = dm9000_reg_read(DM9000_VIDL);
	id_val |= dm9000_reg_read(DM9000_VIDH) << 8;
	id_val |= dm9000_reg_read(DM9000_PIDL) << 16;
	id_val |= dm9000_reg_read(DM9000_PIDH) << 24;
	if (id_val == DM9000_ID) {
		printf("dm9000 is found !\n\r");
		return ;
	} else {
		printf("dm9000 is not found !\n\r");
		return ;
	}
}

void dm9000_init()
{
    u32 i;
    
    /*����Ƭѡ*/
    cs_init();
    
    /*�жϳ�ʼ��*/
    int_init();
    
    /*��λ�豸*/
    dm9000_reset();
    
    /*����dm9000*/
    dm9000_probe();
    
    /*MAC��ʼ��*/
    /* Program operating register, only internal phy supported */
	dm9000_reg_write(DM9000_NCR, 0x0);
	/* TX Polling clear */
	dm9000_reg_write(DM9000_TCR, 0);
	/* Less 3Kb, 200us */
	dm9000_reg_write(DM9000_BPTR, BPTR_BPHW(3) | BPTR_JPT_600US);
	/* Flow Control : High/Low Water */
	dm9000_reg_write(DM9000_FCTR, FCTR_HWOT(3) | FCTR_LWOT(8));
	/* SH FIXME: This looks strange! Flow Control */
	dm9000_reg_write(DM9000_FCR, 0x0);
	/* Special Mode */
	dm9000_reg_write(DM9000_SMCR, 0);
	/* clear TX status */
	dm9000_reg_write(DM9000_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);
	/* Clear interrupt status */
	dm9000_reg_write(DM9000_ISR, ISR_ROOS | ISR_ROS | ISR_PTS | ISR_PRS);
	
    /*���MAC��ַ*/
    for (i = 0; i < 6; i++)
        dm9000_reg_write(DM9000_PAR+i, mac_addr[i]);
    
    /*����DM9000*/
    	dm9000_reg_write(DM9000_RCR, RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN);
	/* Enable TX/RX interrupt mask */
	dm9000_reg_write(DM9000_IMR, IMR_PAR);
}

void dm9000_tx(u8 *data,u32 length)
{
    u32 i;
    
    /*��ֹ�ж�*/
    dm9000_reg_write(DM9000_IMR,0x80);
    
    /*д�뷢����ݵĳ���*/
    dm9000_reg_write(DM9000_TXPLL, length & 0xff);
    dm9000_reg_write(DM9000_TXPLH, (length >> 8) & 0xff);
    
    /*д����͵����*/
    DM_ADD = DM9000_MWCMD;
   
    for(i=0;i<length;i+=2)
    {
    	DM_DAT = data[i] | (data[i+1]<<8);
    }
    
    /*��������*/
    dm9000_reg_write(DM9000_TCR, TCR_TXREQ); 
    
    /*�ȴ��ͽ���*/
    while(1)
    {
       u8 status;
       status = dm9000_reg_read(DM9000_TCR);
       if((status&0x01)==0x00)
           break;	
    }
    
    /*�����״̬*/
    dm9000_reg_write(DM9000_NSR,0x2c);
    
    /*�ָ��ж�ʹ��*/
    dm9000_reg_write(DM9000_IMR,0x81);
}

#define PTK_MAX_LEN 1522

u32 dm9000_rx(u8 *data)
{
    u16 status,len;
    u16 tmp;
    u32 i;
    u8 ready = 0;
    
    /*�ж��Ƿ�����жϣ������*/
    if(dm9000_reg_read(DM9000_ISR) & 0x01)
        dm9000_reg_write(DM9000_ISR,0x01);
    else
        return 0;
        
    /*�ն�*/
    ready = dm9000_reg_read(DM9000_MRCMDX);
    
    if ((ready & 0x01) != 0x01)
    {
    	ready = dm9000_reg_read(DM9000_MRCMDX);
    	if ((ready & 0x01) != 0x01)
    	    return 0;
    }
    
    /*��ȡ״̬*/
    status = dm9000_reg_read(DM9000_MRCMD);
    
    /*��ȡ���*/
    len = DM_DAT;
    
    /*��ȡ�����*/
    if(len<PTK_MAX_LEN)
    {
       for(i=0;i<len;i+=2)
       {
           tmp = DM_DAT;
           data[i] = tmp & 0x0ff;
           data[i+1] = (tmp>>8)&0x0ff;
       }
    }
    
    return len;
}
