int FLAG_TFTP = 1;

char * buff="Hello World ->\n";

int gboot_main()
{
	int num;
	
	//unsigned char buf[1024*4];
#ifdef MMU_ON
    mmu_init();
#endif
	
	uart_init();
    	led_init();
    
    	button_init();
    
    	init_irq();
    
    	led_off();
    	
    	dma_init();
    	dma_start();
    	
	dm9000_init();
    	
   
    while(1)
    {
    	
	printf("\n***************************************\n\r");
    	printf("\n****************MYBOOT*****************\n\r");
    	printf("1:Set Out a Arp Package to Get Host Ip and MAC!\n\r");
    	printf("2:Download Linux Kernel from TFTP Server!\n\r");
    	printf("3:Boot Linux from RAM!\n\r");
    	printf("\n Plese Select:");
    	
    	scanf("%d",&num);
    
        switch (num)
        {
            case 1:
            arp_progress();
            break;
            
            case 2:
            tftp_send_request("tftp.c");
            while(FLAG_TFTP);
            break;
            
            case 3:
            boot_linux();
            //boot_linux_nand();
            break;
            
            default:
                printf("Error: wrong selection!\n\r");
            break;	
        }
    	
    }
    return 0;     
	
}
