#include "common.h"
#include "rtl.h"


//WaveFormTool

static ULONG MP_ReadERIChannelDword(
        PMP_ADAPTER Adapter,
        UCHAR EriChannelType,
        USHORT ExtRegAddr)
{
        int sd;
        ULONG RetVal = 0xffffffff;

        if ((sd = create_control_socket()) < 0)
                return RetVal;

        RetVal = read_eri(sd, Adapter->testnicinfo, ExtRegAddr, 4);

        close(sd);

        if (Adapter->bShowDbgMsg)
                printf("Read ERI Reg 0x%04X Value 0x%08lX\r\n", ExtRegAddr, RetVal);

        return RetVal;

}

/*
static USHORT MP_ReadERIChannelWord(
    PMP_ADAPTER Adapter,
    USHORT EriChannelType,
    USHORT ExtRegAddr)
{
  int sd;
    UCHAR RetVal = 0xffff;

    if ((sd = create_control_socket()) < 0)
        return RetVal;

    RetVal = read_eri(sd, Adapter->testnicinfo, ExtRegAddr, 2);

    close(sd);

    if (Adapter->bShowDbgMsg)
        printf("Read ERI Reg 0x%04X Value 0x%04X\r\n", ExtRegAddr, RetVal);

    return RetVal;

}

static UCHAR MP_ReadERIChannelByte(
    PMP_ADAPTER Adapter,
    UCHAR EriChannelType,
    USHORT ExtRegAddr)
{
 int sd;
    UCHAR RetVal = 0xff;

    if ((sd = create_control_socket()) < 0)
        return RetVal;

    RetVal = read_eri(sd, Adapter->testnicinfo, ExtRegAddr, 1);

    close(sd);

    if (Adapter->bShowDbgMsg)
        printf("Read ERI Reg 0x%04X Value 0x%02X\r\n", ExtRegAddr, RetVal);

    return RetVal;
}
*/

static void MP_WriteERIChannelDword(
        PMP_ADAPTER Adapter,
        UCHAR EriChannelType,
        USHORT ExtRegAddr,
        ULONG RegData)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return;

        if (Adapter->bShowDbgMsg)
                printf("Write ERI Reg 0x%04X Value 0x%08lX\r\n", ExtRegAddr, RegData);

        write_eri(sd, Adapter->testnicinfo, ExtRegAddr, 4, RegData);

        close(sd);
}

/*
static void MP_WriteERIChannelWord(
    PMP_ADAPTER Adapter,
    UCHAR EriChannelType,
    USHORT ExtRegAddr,
    USHORT RegData)
{
int sd;

    if ((sd = create_control_socket()) < 0)
        return;

    if (Adapter->bShowDbgMsg)
        printf("Write ERI Reg 0x%04X Value 0x%04X\r\n", ExtRegAddr, RegData);

    write_eri(sd, Adapter->testnicinfo, ExtRegAddr, 2, RegData);

    close(sd);
}

static void MP_WriteERIChannelByte(
    PMP_ADAPTER Adapter,
    UCHAR EriChannelType,
    USHORT ExtRegAddr,
    UCHAR RegData)
{
int sd;

    if ((sd = create_control_socket()) < 0)
        return;

    if (Adapter->bShowDbgMsg)
        printf("Write ERI Reg 0x%04X Value 0x%02X\r\n", ExtRegAddr, RegData);

    write_eri(sd, Adapter->testnicinfo, ExtRegAddr, 1, RegData);

    close(sd);
}
*/

static USHORT
MP_ReadPhyUshort(
        PMP_ADAPTER Adapter,
        UCHAR   RegAddr)
{
        int sd;
        USHORT RetVal = 0xffff;

        if ((sd = create_control_socket()) < 0)
                return RetVal;

        RetVal = read_phy(sd, Adapter->testnicinfo, RegAddr);

        close(sd);

        if (Adapter->bShowDbgMsg)
                printf("Read Phy Reg 0x%02X Value 0x%04X\r\n", RegAddr, RetVal);

        return RetVal;
}

static void
MP_WritePhyUshort(
        PMP_ADAPTER Adapter,
        UCHAR   RegAddr,
        USHORT  RegData)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return;

        if (Adapter->bShowDbgMsg)
                printf("Write Phy Reg 0x%02X Value 0x%04X\r\n", RegAddr, RegData);

        write_phy(sd, Adapter->testnicinfo, RegAddr, RegData);

        close(sd);
}

static void ReadUchar(PMP_ADAPTER Adapter, BYTE RegisterType, DWORD address, BYTE *pValue)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return;

        usb_direct_read(sd, Adapter->testnicinfo->name, RegisterType, address, (u8 *)pValue, 1);

        if (Adapter->bShowDbgMsg)
                printf("Read Phy(Usb) Byte Reg 0x%04lX Value 0x%02X\r\n", address, *pValue);
}

static void
WriteUchar(PMP_ADAPTER Adapter, BYTE RegisterType, DWORD address, WORD value)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return;

        if (Adapter->bShowDbgMsg)
                printf("Write Phy(Usb) Byte Reg 0x%04lX Value 0x%02X\r\n", address, (BYTE)value);

        usb_direct_write(sd, Adapter->testnicinfo->name, RegisterType, address, (u8 *)&value, 1);

        close(sd);
}

static void ReadUshort(PMP_ADAPTER Adapter, BYTE RegisterType, DWORD address, WORD *pValue)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return;

        usb_direct_read(sd, Adapter->testnicinfo->name, RegisterType, address, (u8 *)pValue, 2);

        if (Adapter->bShowDbgMsg)
                printf("Read Phy(Usb) Word Reg 0x%04lX Value 0x%04X\r\n", address, *pValue);
}

static void
WriteUshort(PMP_ADAPTER Adapter, BYTE RegisterType, DWORD address, WORD value)
{
        int sd;

        if ((sd = create_control_socket()) < 0)
                return;

        if (Adapter->bShowDbgMsg)
                printf("Write Phy(Usb) Word Reg 0x%04lX Value 0x%04X\r\n", address, value);

        usb_direct_write(sd, Adapter->testnicinfo->name, RegisterType, address, (u8 *)&value, 2);

        close(sd);
}

void WaveFormInitAdapter(PMP_ADAPTER Adapter)
{
        USHORT	TmpUshort = 0;
        USHORT PhyRegValue;
        ULONG TmpUlong;

        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                // Switch PHY back to page 3.
                MP_WritePhyUshort(Adapter, 0x1f, 0x0003);
                MP_WritePhyUshort(Adapter, 0x16, 0x000A);
                MP_WritePhyUshort(Adapter, 0x12, 0xC096);
                // Switch PHY back to page 2.
                MP_WritePhyUshort(Adapter, 0x1f, 0x0002);
                MP_WritePhyUshort(Adapter, 0x00, 0x88DE);
                MP_WritePhyUshort(Adapter, 0x06, 0x0761);
                MP_WritePhyUshort(Adapter, 0x01, 0x82B1);
                MP_WritePhyUshort(Adapter, 0x09, 0x01F0);
                MP_WritePhyUshort(Adapter, 0x0A, 0x5500);
                MP_WritePhyUshort(Adapter, 0x03, 0x7002);
                MP_WritePhyUshort(Adapter, 0x0C, 0x00C8);
                // Switch PHY back to page 0.
                MP_WritePhyUshort(Adapter, 0x1f, 0x0000);
                Sleep(10);
                //Disable ALDPS
                MP_WritePhyUshort(Adapter, 0x16, 0x0100);
                MP_WritePhyUshort(Adapter, 0x00, 0x1200);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                //1.
                //801F0005    //page 5
                //80010b00    //disable uC and ALDPS
                //801F0000    //page 0
                //8011401c     //Enable PLL
                //801F0000
                //Set Reg 0x0c, Bit[12] = 0 //?œæ??²å…¥link Down Power Saving mode?„ç¸½?‹é?
                //?¶ç????¢é?waveform tool??
                //801F0000
                //Set Reg 0x0c, Bit[12] = 1
                //2.
                //801f0000     // page 0
                //Register 14  bit15 è¨­ç‚º1
                //æ¸?0M idle (3 ?‹pulse)+10M data???¼ç?crossover
                //3.
                //801f0000   // page 0
                //80141040   // Disable De-glitch circuit
                //801F0005   //page 5
                //8009FF64      // Disable De-glitch circuit

                MP_WritePhyUshort(Adapter, 0x1F, 0x0005);    //page 5
                MP_WritePhyUshort(Adapter, 0x01, 0x0b00);    //disable uC and ALDPS
                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);    //page 0
                MP_WritePhyUshort(Adapter, 0x11, 0x401c);     //Enable PLL
                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);     // page 0
                TmpUshort = MP_ReadPhyUshort(Adapter, 0x0C);
                TmpUshort &= ~BIT12;
                MP_WritePhyUshort(Adapter, 0x0C, TmpUshort);

                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);     // page 0
                //Register 14  set bit15 to 1
                TmpUshort = MP_ReadPhyUshort(Adapter, 0x0E);
                TmpUshort |= BIT15;
                MP_WritePhyUshort(Adapter, 0x0E, TmpUshort);
                //æ¸?0M idle (3 ?‹pulse)+10M data???¼ç?crossover

                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);   // page 0
                MP_WritePhyUshort(Adapter, 0x14, 0x1040);   // Disable De-glitch circuit
                MP_WritePhyUshort(Adapter, 0x1F, 0x0005);   // page 5
                MP_WritePhyUshort(Adapter, 0x09, 0xFF64);   // Disable De-glitch circuit

                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);   // page 0
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);

                //only for RTL8168E Rev.A
                if( Adapter->ChipType == RTL8168E ) {
                        //Initial Setting:
                        //1. SWREG Performance (2M Hz)
                        //801f0007, 801e002c, 80160e0c, 80161e0c, 801e00a1, 80180005, 80180015, 80180005, 801f0002, 800b89d7, 801f0000
                        MP_WritePhyUshort(Adapter, 0x1F, 0x0007);
                        MP_WritePhyUshort(Adapter, 0x1E, 0x002C);
                        MP_WritePhyUshort(Adapter, 0x16, 0x0E0C);
                        MP_WritePhyUshort(Adapter, 0x16, 0x1E0C);
                        MP_WritePhyUshort(Adapter, 0x1E, 0x00A1);
                        MP_WritePhyUshort(Adapter, 0x18, 0x0005);
                        MP_WritePhyUshort(Adapter, 0x18, 0x0015);
                        MP_WritePhyUshort(Adapter, 0x18, 0x0005);
                        MP_WritePhyUshort(Adapter, 0x1F, 0x0002);
                        MP_WritePhyUshort(Adapter, 0x0B, 0x89D7);
                        MP_WritePhyUshort(Adapter, 0x1F, 0x0000);

                        //2. 100M/ 1000M Amplitude
                        //801f0002, 8011ff00, 8010000a, 801f0000
                        MP_WritePhyUshort(Adapter, 0x1F, 0x0002);
                        MP_WritePhyUshort(Adapter, 0x11, 0xFF00);
                        MP_WritePhyUshort(Adapter, 0x10, 0x000A);
                        MP_WritePhyUshort(Adapter, 0x1F, 0x0000);
                }


                //ä¿®æ­£10M link pulse waveform (?œAFE 10M Tx Pwr Saving)
                //801F0002
                //set Reg 0x07 Bit[13] = 0 (?¿å??°å…¶é¤˜bits)
                MP_WritePhyUshort(Adapter, 0x1F,0x0002);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x07);
                PhyRegValue &= ~BIT13;
                MP_WritePhyUshort(Adapter, 0x07,PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);

                //Disable EEE
                //801f0005
                //80058B85
                //Set Reg 0x06 Bit[13] = 0      ( eee auto-off Disable)
                //801F0007
                //801e0020
                //Set Reg 0x15 Bit[8] = 0      ( eee_nway_en Disable)
                //801F0006
                //80005A00                            ( Giga/100 EEE Disable)
                //801F0000
                //800D0007                     ( Disable EEE Nway Ability )
                //800E003C
                //800D4007
                //800E0000
                //800D0000
                MP_WritePhyUshort(Adapter, 0x1F, 0x0005);
                MP_WritePhyUshort(Adapter, 0x05, 0x8B85);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x06);
                PhyRegValue &= ~(BIT13);
                MP_WritePhyUshort(Adapter, 0x06, PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F, 0x0007);
                MP_WritePhyUshort(Adapter, 0x1E, 0x0020);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x15);
                PhyRegValue &= ~(BIT8);
                MP_WritePhyUshort(Adapter, 0x15, PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F, 0x0006);
                MP_WritePhyUshort(Adapter, 0x00, 0x5A00);
                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);

                MP_WritePhyUshort(Adapter, 0x0D, 0x0007);
                MP_WritePhyUshort(Adapter, 0x0E, 0x003C);
                MP_WritePhyUshort(Adapter, 0x0D, 0x4007);
                MP_WritePhyUshort(Adapter, 0x0E, 0x0000);
                MP_WritePhyUshort(Adapter, 0x0D, 0x0000);

                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Disable EEE
                //MAC ERI 0x1b0 bit[1] & bit[0] set to 0  ( MAC EEE Disable)
                //801F0005
                //80058B85
                //Set Reg 0x06 Bit[13] = 0 (è«‹å‹¿?•åˆ°?¶é??„Bits) ( EEE auto-off Disable)
                //801F0004
                //801F0007
                //801e0020
                //Set Reg 0x15 Bit[8] = 0      ( eee_nway_en Disable)
                //801F0002
                //801F0000
                //800D0007                     ( Disable EEE Nway Ability )
                //800E003C
                //800D4007
                //800E0000
                //800D0000
                TmpUlong = MP_ReadERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0);
                TmpUlong &= ~( BIT1 | BIT0 );
                MP_WriteERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0, TmpUlong);

                MP_WritePhyUshort(Adapter, 0x1F,0x0005);
                MP_WritePhyUshort(Adapter, 0x05,0x8B85);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x06);
                PhyRegValue &= ~BIT13;
                MP_WritePhyUshort(Adapter, 0x06,PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F,0x0004);
                MP_WritePhyUshort(Adapter, 0x1F,0x0007);
                MP_WritePhyUshort(Adapter, 0x1E,0x0020);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x15);
                PhyRegValue &= ~BIT8;
                MP_WritePhyUshort(Adapter, 0x15,PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F,0x0002);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);

                MP_WritePhyUshort(Adapter, 0x0D, 0x0007);
                MP_WritePhyUshort(Adapter, 0x0E, 0x003C);
                MP_WritePhyUshort(Adapter, 0x0D, 0x4007);
                MP_WritePhyUshort(Adapter, 0x0E, 0x0000);
                MP_WritePhyUshort(Adapter, 0x0D, 0x0000);


                //ä¸€?²å…¥Waveform Toolå°±å?UC?šä»¥ä¸‹patch (æ­¤patch?¡é?recovery)
                //801F0005
                //80058A7A
                //80064900      (set Reg 0x06 = 0x4900)
                //801F0000
                MP_WritePhyUshort(Adapter, 0x1F,0x0005);
                MP_WritePhyUshort(Adapter, 0x05,0x8A7A);
                MP_WritePhyUshort(Adapter, 0x06,0x4900);
                MP_WritePhyUshort(Adapter, 0x1F,0x0002);

                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Disable EEE
                //MAC ERI 0x1b0 bit[1] & bit[0] set to 0  ( MAC EEE Disable)
                //801F0005
                //80058B85
                //Set Reg 0x06 Bit[13] = 0 (è«‹å‹¿?•åˆ°?¶é??„Bits) ( EEE auto-off Disable)
                //801F0007
                //801e0020
                //Set Reg 0x15 Bit[8] = 0      ( eee_nway_en Disable)
                //801F0000
                //800D0007                     ( Disable EEE Nway Ability )
                //800E003C
                //800D4007
                //800E0000
                //800D0000

                TmpUlong = MP_ReadERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0);
                TmpUlong &= ~( BIT1 | BIT0 );
                MP_WriteERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0, TmpUlong);

                MP_WritePhyUshort(Adapter, 0x1F,0x0005);
                MP_WritePhyUshort(Adapter, 0x05,0x8B85);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x06);
                PhyRegValue &= ~BIT13;
                MP_WritePhyUshort(Adapter, 0x06,PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F,0x0007);
                MP_WritePhyUshort(Adapter, 0x1E,0x0020);
                PhyRegValue = MP_ReadPhyUshort(Adapter, 0x15);
                PhyRegValue &= ~BIT8;
                MP_WritePhyUshort(Adapter, 0x15,PhyRegValue);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);

                MP_WritePhyUshort(Adapter, 0x0D, 0x0007);
                MP_WritePhyUshort(Adapter, 0x0E, 0x003C);
                MP_WritePhyUshort(Adapter, 0x0D, 0x4007);
                MP_WritePhyUshort(Adapter, 0x0E, 0x0000);
                MP_WritePhyUshort(Adapter, 0x0D, 0x0000);


                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0A43 // page 0x0A43
                //8011xxxx => set reg0x11 to Bit[4] = 1'b0, // ?œé?eee 10M?æŒ¯å¹?
                //801F0A40 // page 0x0A40
                //80109200 // phyreset, è¦å?PHY reset, ä¸Šé¢?œé?eee 10M?æŒ¯å¹…ç??•ä??æ?èµ·ä???
                //MP_WritePhyUshort(Adapter, 0x1F, 0x0A43);
                //PhyRegValue = MP_ReadPhyUshort(Adapter, 0x11);
                //PhyRegValue &= ~(BIT4);
                //MP_WritePhyUshort(Adapter, 0x11, PhyRegValue);
                //MP_WritePhyUshort(Adapter, 0x1F, 0x0A40);
                //MP_WritePhyUshort(Adapter, 0x10, 0x9200);
                //Sleep(3000); // Wait NWay complete.

                //Disable ALDPS, ?¿å??¨ALDPS modeä¸‹æ?äº›commandä¸‹ä??²åŽ»
                //801F0A43 // page 0x0A43
                //8010xxxx => set reg 0x10 to bit[2] = 1'b0 //disable ALDPS
                //MP_WritePhyUshort(Adapter, 0x1F, 0x0A43);
                //PhyRegValue = MP_ReadPhyUshort(Adapter, 0x10);
                //PhyRegValue &= ~(BIT2);
                //MP_WritePhyUshort(Adapter, 0x10, PhyRegValue);
              	MP_WritePhyUshort(Adapter, 0x1F, 0x04);
		Sleep(100);
		MP_WritePhyUshort(Adapter, 0x10, 0x4077);
		Sleep(100);
		MP_WritePhyUshort(Adapter, 0x15, 0xc5a0);

		MP_WritePhyUshort(Adapter, 0x1F, 0x0);
		Sleep(100);
                MP_WritePhyUshort(Adapter, 0x0, 0x8000);
		Sleep(3000);
                MP_WritePhyUshort(Adapter, 0x18, 0x310);
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                break;
        case RTL8136:
        case RTL8136_REV_E:
                MP_WritePhyUshort(Adapter, 0x1f,0x0001);
                Sleep(20);
                MP_WritePhyUshort(Adapter, 0x19,0x0041);
                Sleep(20);
                MP_WritePhyUshort(Adapter, 0x1f,0x0000);
                Sleep(20);
                MP_WritePhyUshort(Adapter, 0x00,0x0100);
                Sleep(50);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Initial Setting:
                //1
                //801F0000    //page 0
                //801115c0    // Enable PLL
                //80190080    // disable ALDPS
                //2
                //801f0003   //page 3
                //8008441d   // Select clock trigger edge
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);
                MP_WritePhyUshort(Adapter, 0x11,0x15C0);
                MP_WritePhyUshort(Adapter, 0x19,0x0080);
                MP_WritePhyUshort(Adapter, 0x1F,0x0003);
                MP_WritePhyUshort(Adapter, 0x08,0x441D);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8402:
                //Disable EEE
                //ERI reg. 0x1B0~0x1B1 write 16?™h0000
                //801F0004
                //8010401F
                //80197030
                //801F0000
                //Disable EEE Plus
                //ERI reg. 0x1D0~0x1D1 write 16?™h0000
                //Disable ALDPS
                //801F0000
                //80180310
                TmpUlong = MP_ReadERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0);
                TmpUlong &= 0xFFFF0000;
                MP_WriteERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0, TmpUlong);
                MP_WritePhyUshort(Adapter, 0x1F,0x0004);
                MP_WritePhyUshort(Adapter, 0x10,0x401F);
                MP_WritePhyUshort(Adapter, 0x19,0x7030);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);

                TmpUlong = MP_ReadERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1D0);
                TmpUlong &= 0xFFFF0000;
                MP_WriteERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1D0, TmpUlong);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);
                MP_WritePhyUshort(Adapter, 0x18,0x0310);
                break;
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
                //Disable ALDPS
                //801F0000
                //80180310
                //Disable EEE
                //ERI reg. 0x1B0~0x1B1 write 16?™h0000
                //801F0004
                //8010C07F
                //80197030
                //801F0000
                //Disable EEE Plus
                //ERI reg. 0x1D0~0x1D1 write 16?™h0000
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);
                MP_WritePhyUshort(Adapter, 0x18,0x0310);

                TmpUlong = MP_ReadERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0);
                TmpUlong &= 0xFFFF0000;
                MP_WriteERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1B0, TmpUlong);
                MP_WritePhyUshort(Adapter, 0x1F,0x0004);
                MP_WritePhyUshort(Adapter, 0x10,0xC07F);
                MP_WritePhyUshort(Adapter, 0x19,0x7030);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);

                TmpUlong = MP_ReadERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1D0);
                TmpUlong &= 0xFFFF0000;
                MP_WriteERIChannelDword(Adapter, ERI_CHANNEL_TYPE_EXT_GMAC, 0x1D0, TmpUlong);
                MP_WritePhyUshort(Adapter, 0x1F,0x0000);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B: {
                UCHAR ucTempData;

                //Disable ALDPS
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB010,0x0310);

                //Disable EEE
                ReadUchar(Adapter,PLAMCU_OCP,0xE040,&ucTempData);
                ucTempData &= ~(BIT0|BIT1);
                WriteUchar(Adapter,PLAMCU_OCP,0xE040,ucTempData);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB080,0xC07F);
                WriteUshort(Adapter,PLAMCU_OCP,0xB092,0x7030);
                WriteUshort(Adapter,PLAMCU_OCP,0xB094,0xFFE6);

                //Disable EEE Plus
                ReadUchar(Adapter,PLAMCU_OCP,0xE080,&ucTempData);
                ucTempData &= ~(BIT1);
                WriteUchar(Adapter,PLAMCU_OCP,0xE080,ucTempData);
        }
        break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C: {
                UCHAR ucTempData;

                //Disable ALDPS
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUchar(Adapter,PLAMCU_OCP,0xB430,&ucTempData);
                ucTempData &= ~BIT2;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,ucTempData);

                //Disable EEE
                ReadUchar(Adapter,PLAMCU_OCP,0xE040,&ucTempData);
                ucTempData &= ~(BIT0|BIT1);
                WriteUchar(Adapter,PLAMCU_OCP,0xE040,ucTempData);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUchar(Adapter,PLAMCU_OCP,0xB432,&ucTempData);
                ucTempData &= ~BIT4;
                WriteUshort(Adapter,PLAMCU_OCP,0xB432,ucTempData);
                ReadUchar(Adapter,PLAMCU_OCP,0xB5D0,&ucTempData);
                ucTempData &= ~(BIT1|BIT2);
                WriteUshort(Adapter,PLAMCU_OCP,0xB5D0,ucTempData);

                //Disable EEE Plus
                ReadUchar(Adapter,PLAMCU_OCP,0xE080,&ucTempData);
                ucTempData &= ~(BIT1);
                WriteUchar(Adapter,PLAMCU_OCP,0xE080,ucTempData);
        }
        break;
        }

        //clear 500M capability
        switch(Adapter->ChipType) {
        case RTL8168H:
                //801F0A42
                //8014xxxx => page 0xA42, reg 0x14, clr bit[9] // clr 500M nway cap
                MP_WritePhyUshort(Adapter, 0x1F, 0x0A42);
                TmpUshort = MP_ReadPhyUshort(Adapter, 0x14);
                TmpUshort &= ~(BIT9);
                MP_WritePhyUshort(Adapter, 0x14, TmpUshort);
                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);
                break;
        }

//        if(Adapter->testnicinfo->busfeature & PCI_NIC)
//                MP_WritePhyUshort(Adapter, 0x1F, 0x0000);   // page 0
}

static void RTK_10M_Config(PMP_ADAPTER Adapter)
{
        USHORT	TmpUshort;

        switch(Adapter->ChipType) {
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
        case RTL8136:
        case RTL8136_REV_E:
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                MP_WritePhyUshort(Adapter,0x1f,0x0000);
                MP_WritePhyUshort(Adapter,0x0e,0x0000);
                break;
        default:
                break;
        }
/*
        if(Adapter->testnicinfo->busfeature & PCI_NIC) {
                MP_WritePhyUshort(Adapter,0x1f,0x0000);
                TmpUshort=MP_ReadPhyUshort(Adapter,0x09);
                TmpUshort &= 0xfcff; //clear out Giga-advertise
                MP_WritePhyUshort(Adapter,0x09, TmpUshort);
                TmpUshort=MP_ReadPhyUshort(Adapter,0x04);
                TmpUshort &= 0xfe7f; //clear out 100-advertise
                MP_WritePhyUshort(Adapter,0x04, TmpUshort);

                switch(Adapter->ChipType) {
                case RTL8168FB:
                case RTL8168FB_REV_B:
                case RTL8411:
                        //10M PLL Bug must use 0x9200 instead of 0x1200
                        MP_WritePhyUshort(Adapter,0x0000, 0x9200); // re-start NWay
                        break;
                default:
                        MP_WritePhyUshort(Adapter,0x0000, 0x1200); // re-start NWay
                        break;
                }

                Sleep(3000); // Wait NWay complete.
        }
*/
        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                // Get PHY version in PhyReg3[3:0].
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                if((TmpUshort & 0x000f) == 0x0002) {
                        // 0010b means 8169SB 0.13.
                        // Switch PHY to page 2.
                        MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                        // PhyReg0 <- 0x7400.
                        MP_WritePhyUshort(Adapter,0x00, 0x7400);
                        // Switch PHY back to page 0.
                        MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                }
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
        case RTL8136:
        case RTL8136_REV_E:
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                // Get PHY version in PhyReg3[3:0].
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                if((TmpUshort & 0x000f) == 0x0002) {
                        // 0010b means 8169SB 0.13.
                        // Switch PHY to page 2.
                        MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                        // PhyReg0 <- 0x7400.
                        //bandgap voltage
                        MP_WritePhyUshort(Adapter,0x00, 0x7400);
                        // Switch PHY back to page 0.
                        MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                }
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                // Get PHY version in PhyReg3[3:0].
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                if((TmpUshort & 0x000f) == 0x0002) {
                        // Switch PHY back to page 0.
                        MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                }
                break;
        }
}

static void RTK_10M_Deconfig(PMP_ADAPTER Adapter)
{
        USHORT	TmpUshort;

        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                if((TmpUshort & 0x000f) == 0x0002) {
                        MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                        // Get PHY version in PhyReg3[3:0].
                        TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                        if((TmpUshort & 0x000f) == 0x0002) {
                                // 0010b means 8169SB 0.13.

                                // Switch PHY to page 2.
                                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                                // PhyReg0 <- 0x7c00.
                                MP_WritePhyUshort(Adapter,0x0, 0x7c00);
                                // Switch PHY back to page 0.
                                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                        }
                }
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
        case RTL8136:
        case RTL8136_REV_E:
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                if((TmpUshort & 0x000f) == 0x0002) {
                        USHORT	TmpUshort;

                        MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                        // Get PHY version in PhyReg3[3:0].
                        TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                        if((TmpUshort & 0x000f) == 0x0002) {
                                // 0010b means 8169SB 0.13.
                                // Switch PHY to page 2.
                                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                                // PhyReg0 <- 0x7c00.
                                //0x7c00 to 0x6400(default value)
                                MP_WritePhyUshort(Adapter,0x0, 0x6400);
                                // Switch PHY back to page 0.
                                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                        }
                }
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                if((TmpUshort & 0x000f) == 0x0002) {
                        USHORT	TmpUshort;

                        MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                        // Get PHY version in PhyReg3[3:0].
                        TmpUshort = MP_ReadPhyUshort(Adapter,0x03);
                        if((TmpUshort & 0x000f) == 0x0002) {
                                // 0010b means 8169SB 0.13.

                                // Switch PHY to page 2.
                                // Switch PHY back to page 0.
                                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                        }
                }
                break;
        }
}

void RTK_10M_Haromonic_Finish(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        Adapter->kee_send_packet = FALSE;
        Sleep(3000); // Wait thread stop.

        // Undo Extra configuration PHY for 8169SB 0.13 in 10Mbps mode if necessary.

        RTK_10M_Deconfig(Adapter);

        switch(Adapter->ChipType) {
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168E:
                //Close Packetgen
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                break;
        case RTL8168E_REV_B:
                //Close Packetgen
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x08,0x8030);
                MP_WritePhyUshort(Adapter,0x09,0x01F0);
                MP_WritePhyUshort(Adapter,0x16,0x5900);
                MP_WritePhyUshort(Adapter,0x06,0x3080);

                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0046);
                MP_WritePhyUshort(Adapter,0x18,0x7777);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                break;
        case RTL8168E_REV_C:
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                //?„å?10M Harmonic Patch (for 8111E B??
                //801f0002, 80088030, 800901f0, 80165900, 80063080, 801f0000, 801f0007, 801e0046, 80187777, 801f0000
                //?„å?10M Harmonic Patch (for 8111E C??& D??
                //801f0002, Reg 0x1B bit[0]=0

                //Close Packetgen
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1B);
                PhyRegValue &= ~BIT0;
                MP_WritePhyUshort(Adapter,0x1B,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                //Enable ALDPS (Unlock Main)
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 0      (Unlock main)
                //801f0002, 801f0000, 80009200
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Enable ALDPS (Unlock Main)
                MP_WritePhyUshort(Adapter,0x1F,0x0004);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //10M harmonic

                //?žå¡« FineTune Amplitude
                //801f0007, 801e0078, 80170000, 801900FB, 801f0000
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                //Enable ALDPS (Unlock Main)
                //801f0007, 801e0023,
                //80160306, 80160300
                //80180000, Set Reg 0x18 Bit[0] = 0(Unlock main)
                //801f0000, 80009200


                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0078);
                MP_WritePhyUshort(Adapter,0x17,0x0000);
                MP_WritePhyUshort(Adapter,0x19,0x00FB);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Enable ALDPS (Unlock Main)
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x16,0x0306);
                MP_WritePhyUshort(Adapter,0x16,0x0300);
                MP_WritePhyUshort(Adapter,0x18,0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0C80 // page 0x0C80
                //8012xxxx => set reg 0x12 to bit[9,8] = 2'b 01 //select tx pseudo-random pattern
                //80105a00 // Close Packetgen and set to default
                //801F0A40 // page 0x0A40
                //80109200 // page 0xa40, reg0 to reset phy & renway
/*original
                MP_WritePhyUshort(Adapter,0x1f, 0x0c80);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x12);
                PhyRegValue &= ~(BIT9);
                PhyRegValue |= BIT8;
                MP_WritePhyUshort(Adapter,0x12, PhyRegValue);
                MP_WritePhyUshort(Adapter,0x10, 0x5a00);
                MP_WritePhyUshort(Adapter,0x1f, 0x0a40);
                MP_WritePhyUshort(Adapter,0x10, 0x9200);
*/
                MP_WritePhyUshort(Adapter,0x1f, 0x0020);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x12);
                PhyRegValue &= ~(BIT15);
                PhyRegValue |= BIT14;
                MP_WritePhyUshort(Adapter,0x12, PhyRegValue);
                MP_WritePhyUshort(Adapter,0x10, 0x5a00);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xC000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A00);
                WriteUshort(Adapter,PLAMCU_OCP,0xB804,0x0115);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_10M_Harmonic(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        RTK_10M_Config(Adapter);

// Step 2. Setup up variables needed in sending packet thread.
        // Extra configuration PHY for 0.13 in 10Mbps mode.
        switch(Adapter->ChipType) {
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Harmonic
                //Transmit 10M waveform
                //801f0006, 800205ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x05EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                break;
        case RTL8168E:
                //Harmonic
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x05EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168E_REV_B:
                //Harmonic
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x08,0x9C30);
                MP_WritePhyUshort(Adapter,0x09,0xA1F0);
                MP_WritePhyUshort(Adapter,0x16,0x59F0);
                MP_WritePhyUshort(Adapter,0x06,0xF080);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0046);
                MP_WritePhyUshort(Adapter,0x18,0xFFFF);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x05EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                break;
        case RTL8168E_REV_C:
                //Harmonic
        {
                USHORT PhyRegValue;

                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1B);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x1B,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);


                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x05EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
        }
        break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Harmonic
                //Disable ALDPS (?ªå?Lock Main)
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000
                //Transmit 10M waveform
                //801f0006, 800205ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0004);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Transmit 10M waveform
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x05EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Harmonic
                //FineTune Amplitude
                //801f0007, 801e0078, 8017aa00, 80190008, 801f0000
                //Disable ALDPS (?ªå?Lock Main)
                //801f0007, 801e0023,
                //80180001, Set Reg 0x18 Bit[0] = 1 ( Lock main)
                //Transmit 10M waveform
                //801f0006, 800205ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0078);
                MP_WritePhyUshort(Adapter,0x17,0xAA00);
                MP_WritePhyUshort(Adapter,0x19,0x0008);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x18,0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                //Transmit 10M waveform
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x05EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //Harmonic
                //801F0A40 // page 0x0A40
                //80100100
                //801F0C80 // page 0x0C80
                //80105a00 // page 0xc80, reg16 , set to default
                //8012xxxx => set reg 0x12 to bit[9,8] = 2'b 00 //select tx fixed pattern
                //8010ff21
/* original
                MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                MP_WritePhyUshort(Adapter,0x10, 0x0100);
                MP_WritePhyUshort(Adapter,0x1F, 0x0c80);
                MP_WritePhyUshort(Adapter,0x10, 0x5A00);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x12);
                PhyRegValue &= ~(BIT9);
                PhyRegValue &= ~(BIT8);
                MP_WritePhyUshort(Adapter,0x12, PhyRegValue);
                MP_WritePhyUshort(Adapter,0x10, 0xFF21);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x0100);
                MP_WritePhyUshort(Adapter,0x1F, 0x0020);
                MP_WritePhyUshort(Adapter,0x10, 0x5A00);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x12);
                PhyRegValue &= ~(BIT15);
                PhyRegValue &= ~(BIT14);
                MP_WritePhyUshort(Adapter,0x12, PhyRegValue);
                MP_WritePhyUshort(Adapter,0x10, 0xFF21);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //Harmonic
                //Transmit 10M waveform
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB408,0x0061);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //Harmonic
                //Transmit 10M waveform
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xC000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A00);
                WriteUshort(Adapter,PLAMCU_OCP,0xB804,0x0015);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0xFF21);
                break;
        default:
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

        //send packet
        send_waveform_test_packet(Adapter);
}

void RTK_10M_Normal_Finish(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        Adapter->kee_send_packet = FALSE;
        Sleep(3000); // Wait thread stop.

        // Undo Extra configuration PHY for 8169SB 0.13 in 10Mbps mode if necessary.
        RTK_10M_Deconfig(Adapter);

        switch(Adapter->ChipType) {
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000

                //Close Packetgen
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                //Patch For 10M Normal waveform
                /*
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x15, m_usPhyRegValue2_15);
                MP_WritePhyUshort(Adapter,0x16, m_usPhyRegValue2_16);
                */
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                //Enable ALDPS (Unlock Main)
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 0      (Unlock main)
                //801f0002, 801f0000, 80009200
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Enable ALDPS (Unlock Main)
                MP_WritePhyUshort(Adapter,0x1F,0x0004);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //10M normal

                //?žå¡« FineTune Amplitude
                //801f0007, 801e0078, 80170000, 801900FB, 801f0000
                //Close Packetgen
                //801f0006, 80005a00, 800245ee, 801f0000
                //Enable ALDPS (Unlock Main)
                //801f0007, 801e0023,
                //80160306, 80160300
                //80180000, Set Reg 0x18 Bit[0] = 0 (Unlock main)
                //801f0000, 80009200
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0078);
                MP_WritePhyUshort(Adapter,0x17,0x0000);
                MP_WritePhyUshort(Adapter,0x19,0x00FB);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                //close packetgen
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x00,0x5A00);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Enable ALDPS (Unlock Main)
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x16,0x0306);
                MP_WritePhyUshort(Adapter,0x16,0x0300);
                MP_WritePhyUshort(Adapter,0x18,0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0C80 // page 0x0C80
                //80105a00 // Close Packetgen & set to default
                //801F0A40 // page 0x0A40
                //80109200 // page 0xa40, reg0 to reset phy & renway
/*original
                MP_WritePhyUshort(Adapter,0x1f, 0x0c80);
                MP_WritePhyUshort(Adapter,0x10, 0x5a00);
                MP_WritePhyUshort(Adapter,0x1f, 0x0a40);
                MP_WritePhyUshort(Adapter,0x10, 0x9200);
*/
                MP_WritePhyUshort(Adapter,0x1f, 0x0020);
                MP_WritePhyUshort(Adapter,0x10, 0x5a00);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xC000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A00);
                WriteUshort(Adapter,PLAMCU_OCP,0xB804,0x0115);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_10M_Normal(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        RTK_10M_Config(Adapter);

// Step 2. Setup up variables needed in sending packet thread.
        // Extra configuration PHY for 0.13 in 10Mbps mode.
        switch(Adapter->ChipType) {
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Transmit 10M waveform
                //801f0006, 800245ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Patch For 10M Normal waveform
                //801F0002 (Page 2)
                //8016590F (Page 2, Set Reg 0x16 = 0x590F, Tune 10M GDAC current up + 15*2.5%)
                //801534EC (Page 2, Set Reg 0x15 = 0x34EC)
                //801F0000
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x16,0x590F);
                MP_WritePhyUshort(Adapter,0x15,0x34EC);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                //Transmit10Mwaveform
                //801f0006,800245ee, 80005a21, 801f0000
                //* 10M normal waveformä¸é?patch harmonic?ƒæ•¸
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0x5A21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Disable ALDPS (?ªå?Lock Main)
                //801f0004, 801f0007, 801e0023,
                //80171711  Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000
                //Transmit 10M waveform
                //801f0006, 800245ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0004);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Transmit 10M waveform
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //FineTune Amplitude
                //801f0007, 801e0078, 8017aa00, 80190008 801f0000
                //Disable ALDPS (?ªå?Lock Main)
                //801f0007, 801e0023,
                //80180001, Set Reg 0x18 Bit[0] = 1 ( Lock main)
                //Transmit 10M waveform
                //801f0006, 800245ee, 80005a21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0078);
                MP_WritePhyUshort(Adapter,0x17,0xAA00);
                MP_WritePhyUshort(Adapter,0x19,0x0008);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x18,0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                //Transmit 10M waveform
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0x5a21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                801F0A40 // page 0x0A40
                80100100 // force 10M
                801F0C80 // page 0x0C80
                80105a21 //Transmit 10M waveform
                */
//                MP_WritePhyUshort(Adapter,0x1f, 0x0A40);
//                MP_WritePhyUshort(Adapter,0x10, 0x0100);
//                MP_WritePhyUshort(Adapter,0x1f, 0x0C80);
//                MP_WritePhyUshort(Adapter,0x10, 0x5A21);
                MP_WritePhyUshort(Adapter,0x1f, 0x00);
                MP_WritePhyUshort(Adapter,0x00, 0x0100);
                MP_WritePhyUshort(Adapter,0x1f, 0x0020);
                MP_WritePhyUshort(Adapter,0x10, 0x5A21);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //Transmit 10M waveform
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB408,0x0061);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //Transmit 10M waveform
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xB000);
                WriteUshort(Adapter,PLAMCU_OCP,0xBCC0,0xA419);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xC000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A00);
                WriteUshort(Adapter,PLAMCU_OCP,0xB804,0x0115);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A21);
                break;
        default:
                break;
        }

        //send packet
        send_waveform_test_packet(Adapter);
}


void RTK_10M_Link_Pluse(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        RTK_10M_Config(Adapter);

// Step 2. Setup up variables needed in sending packet thread.
        // Extra configuration PHY for 0.13 in 10Mbps mode.
        switch(Adapter->ChipType) {
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Transmit 10M waveform
                //801f0006, 800245ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Patch For 10M Normal waveform
                //801F0002 (Page 2)
                //8016590F (Page 2, Set Reg 0x16 = 0x590F, Tune 10M GDAC current up + 15*2.5%)
                //801534EC (Page 2, Set Reg 0x15 = 0x34EC)
                //801F0000
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x16,0x590F);
                MP_WritePhyUshort(Adapter,0x15,0x34EC);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                //Transmit10Mwaveform
                //801f0006,800245ee, 80005a21, 801f0000
                //* 10M normal waveformä¸é?patch harmonic?ƒæ•¸
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0x5A21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Disable ALDPS (?ªå?Lock Main)
                //801f0004, 801f0007, 801e0023,
                //80171711  Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000
                //Transmit 10M waveform
                //801f0006, 800245ee, 8000ff21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0004);
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F,0x0002);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //Transmit 10M waveform
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0xFF21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //FineTune Amplitude
                //801f0007, 801e0078, 8017aa00, 80190008 801f0000
                //Disable ALDPS (?ªå?Lock Main)
                //801f0007, 801e0023,
                //80180001, Set Reg 0x18 Bit[0] = 1 ( Lock main)
                //Transmit 10M waveform
                //801f0006, 800245ee, 80005a21, 801f0000
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0078);
                MP_WritePhyUshort(Adapter,0x17,0xAA00);
                MP_WritePhyUshort(Adapter,0x19,0x0008);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);

                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x18,0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                //Transmit 10M waveform
                MP_WritePhyUshort(Adapter,0x1F,0x0006);
                MP_WritePhyUshort(Adapter,0x02,0x45EE);
                MP_WritePhyUshort(Adapter,0x00,0x5a21);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                801F0A40 // page 0x0A40
                80100100 // force 10M
                801F0C80 // page 0x0C80
                80105a21 //Transmit 10M waveform
                */
//                MP_WritePhyUshort(Adapter,0x1f, 0x0A40);
//                MP_WritePhyUshort(Adapter,0x10, 0x0100);
//                MP_WritePhyUshort(Adapter,0x1f, 0x0C80);
//                MP_WritePhyUshort(Adapter,0x10, 0x5A21);
                MP_WritePhyUshort(Adapter,0x1f, 0x00);
                MP_WritePhyUshort(Adapter,0x00, 0x0100);
                MP_WritePhyUshort(Adapter,0x1f, 0x0020);
                MP_WritePhyUshort(Adapter,0x10, 0x5A21);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //Transmit 10M waveform
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB408,0x0061);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //Transmit 10M waveform
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xB000);
                WriteUshort(Adapter,PLAMCU_OCP,0xBCC0,0xA419);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xC000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A00);
                WriteUshort(Adapter,PLAMCU_OCP,0xB804,0x0115);
                WriteUshort(Adapter,PLAMCU_OCP,0xB800,0x5A21);
                break;
        default:
                break;
        }
}

void RTK_100M_Channel_A(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        switch(Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //Channel A:
                //801f0002   //page 2
                //80088230  // Increase Amplitude (V1.1 2008.8.5)
                //801f0000    //page 0
                //800e0660    //Output MLT-3
                //80100020    //MDI (V1.3 2009.7.6)
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x08, 0x8230);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0660);
                MP_WritePhyUshort(Adapter,0x10, 0x0020);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                //Channel A:
                //801f0002   //page 2
                //800086a0   //default value  (V1.5 2009.12.25)
                //801f0000    //page 0
                //800e0660    //Output MLT-3

                MP_WritePhyUshort(Adapter,0x1F, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x00, 0x86A0);   //default value  (V1.5 2009.12.25)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x0E, 0x0660);   //Output MLT-3
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Channel A:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006,
                //801f0007, 801e0023, 80160004, 801f0000
                //Force MDI
                //801f0007, 801e002d, 8018f020, 801f0000, 801001ae, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 80171818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01AE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Channel A:
                //LockNctl
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000, wait(100ms),  80150006, 801f0004, 801f0007, 801e0023, 80160006, 801f0002, 801f0000
                //Force MDI
                //801f0007, 801e002d, 8018f020, 801f0002, 801f0000, 801001ae, 801f0000
                //Force100Pow
                //801f0004, 801f0007, 801e002f, 8017d818, 801f0002, 801f0000
                //RstDaFIFO
                //801f0004, 801f0007, 801e002c, 801800c3, 801800cb, 801f0002, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                Sleep(100);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01AE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Channel A:
                //LockNctl
                //801f0007, 801e0023,
                //80180001,  Set Reg 0x18 Bit[0] = 1      ( Lock main)
                //wait(20ms), 80160006, 801f0000
                //Force MDI
                //801f0007, 801e002d, 8018f060, 801f0000, 8010092e, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 8017d818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                Sleep(20);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF060);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x092E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                Channel A:
                801F0A40 // page 0x0A40
                80102100 // force 100M
                801f0a43,
                8010xxxx=> reg0x10 to bit[9,8] = 2'b 11 //p0 reg24 bit[8]=1 mdi mode
                */
                //MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                //MP_WritePhyUshort(Adapter,0x10, 0x2100);
                //MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                //PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                //PhyRegValue |= (BIT9 | BIT8);
                //MP_WritePhyUshort(Adapter,0x10,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
		Sleep(100);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C2);
		Sleep(100);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
		Sleep(100);
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Channel A:
                //801F0002    //page 2
                //800086A9    //Increase 100M amplitude 1%
                //800E8001    // Increase ChA amplitude (Add 12mV)
                //801F0000    //page 0
                //800e0660    //Output MLT-3
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x86A9);
                MP_WritePhyUshort(Adapter,0x0E, 0x8001);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x0E, 0x0660);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Channel A:
                //801f0001   //page 1
                //80191f01   // MDI
                //801f0000    //page 0
                //80002100    //Force 100M
                MP_WritePhyUshort(Adapter,0x1F, 0x0001);
                MP_WritePhyUshort(Adapter,0x19, 0x1F01);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //100 channel A
                //801F0000
                //801C40C2
                //80002100
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C2);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //100 channel A
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C2);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //100 channel A
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue |= (BIT8|BIT9);
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        }
}

void RTK_100M_Finish(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        //801f0002   //page 2    V1.3 2008.6.6
        //800086a0   //          V1.3 2008.6.6
        //801f0000    //page 0
        //800e0000   // Disable MLT-3
        //801000A0    //MDI

        switch(Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //801f0002   //page 2
                //80088030   //  default value
                //801f0000    //page 0
                //800e0000   // Disable MLT-3
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x08, 0x8030);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x00, 0x86a0);   //Increase Amplitude
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);   //Output MLT-3
                MP_WritePhyUshort(Adapter,0x10, 0x00a0);   //MDIX
                break;

        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Finish MLT-3:
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f000, 801f0000, 801001ee, 801f0000
                //ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01EE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Finish MLT-3:
                //Auto MDI / MDIX
                //801f0004, 801f0007, 801e002d, 8018f000, 801f0002, 801f0000, 801001ee, 801f0000
                //ForceDefaultPow
                //801f0004, 801f0007, 801e002f, 8017d88f, 801f0002, 801f0000
                //Release Nctl
                //801f0004, 801f0007, 801e0023, 80160306, 80160300,
                //Set Reg 0x17 Bit[0] = 0      (Unlock main)
                //801f0002, 801f0000, 80009200
                //RstDaFIFO
                //801f0004, 801f0007, 801e002c, 801800c3, 801800cb, 801f0002, 801f0000

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01EE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD88F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0306);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Finish MLT-3:
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f040, 801f0000, 8010096e, 801f0000
                //ForceDefaultPow
                //801f0007, 801e002f, 8017d88f, 801f0000
                //Release Nctl
                //801f0007, 801e0023, 80160306, 80160300,
                //80180000  Set Reg 0x18 Bit[0] = 0      (Unlock main)
                //801f0000, 80009200
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF040);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x096E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD88F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0306);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x18, 0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                finish 100M:
                801f0a43,
                8010xxxx=> reg0x10 to bit[9,8] = 2'b 00 //enable auto-crossover
                801F0A40 // page 0x0A40
                80109200 // page 0xa40, reg0 to reset phy & renway
                */
/*original
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue &= ~(BIT9 | BIT8);
                MP_WritePhyUshort(Adapter,0x10,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                MP_WritePhyUshort(Adapter,0x10, 0x9200);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue |= (BIT2);
                MP_WritePhyUshort(Adapter,0x1c,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //801F0002    //page 2
                //800086A0    //Default value
                //800E8000    // Default value
                //801F0000    //page 0
                //800e0000   // Disable MLT-3
                //801000A0    //MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x86A0);
                MP_WritePhyUshort(Adapter,0x0E, 0x8000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x0E, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x00A0);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Finish MLT-3:
                //801f0001   //page 1
                //80190003   // Auto MDIX
                //801f0000    //page 0
                //80001200    //Restart Nway
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x0003);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //Finish 100M
                //801F0000
                //801C40C6
                //80001200
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C6);
                MP_WritePhyUshort(Adapter,0x00, 0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //Finish 100M
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C6);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //Finish 100M
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue |= BIT8;
                PhyRegValue &= ~BIT9;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_100M_Channel_B(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        switch(Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //Channel B:
                //801f0002   //page 2
                //80088230  // Increase Amplitude (V1.1 2008.8.5)
                //801f0000    //page 0
                //800e0660    //Output MLT-3
                //80100000    //MDIX
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x8230);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0660);
                MP_WritePhyUshort(Adapter,0x10, 0x0000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                //Channel B:
                //801f0002   //page 2
                //800086a0   //default value  (V1.5 2009.12.25)
                //801f0000    //page 0
                //800e0660    //Output MLT-3
                //80100080    //MDIX

                MP_WritePhyUshort(Adapter,0x1F, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x00, 0x86A0);   //default value  (V1.5 2009.12.25)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x0E, 0x0660);   //Output MLT-3
                MP_WritePhyUshort(Adapter,0x10, 0x0080);   //MDIX
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Channel B:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006,
                //801f0007, 801e0023, 80160004, 801f0000
                //Force MDIX
                //801f0007, 801e002d, 8018f020, 801f0000, 8010018e, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 80171818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x018E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Channel B:
                //LockNctl
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000, wait(100ms),  80150006, 801f0004, 801f0007, 801e0023, 80160006, 801f0002, 801f0000
                //Force MDIX
                //801f0007, 801e002d, 8018f020, 801f0002, 801f0000, 8010018e, 801f0000
                //Force100Pow
                //801f0004, 801f0007, 801e002f, 8017d818, 801f0002, 801f0000
                //RstDaFIFO
                //801f0004, 801f0007, 801e002c, 801800c3, 801800cb, 801f0002, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                Sleep(100);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x018E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Channel B:
                //LockNctl
                //801f0007, 801e0023,
                //80180001,  Set Reg 0x18 Bit[0] = 1      ( Lock main)
                //wait(20ms), 80160006, 801f0000
                //Force MDIX
                //801f0007, 801e002d, 8018f060, 801f0000, 8010090e, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 8017d818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                Sleep(20);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF060);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x090E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                Channel B:
                801F0A40 // page 0x0A40
                80102100 // force 100M
                801f0a43,
                8010xxxx=> reg0x10 to bit[9,8] = 2'b 10 //p0 reg24 bit[8]=0 mdix mode
                */
                //MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                //MP_WritePhyUshort(Adapter,0x10, 0x2100);
                //MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                //PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                //PhyRegValue &= ~(BIT8);
                //PhyRegValue |= (BIT9);
                //MP_WritePhyUshort(Adapter,0x10,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C0);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //Channel B:
                //801F0002    //page 2
                //800086A9    //Increase 100M amplitude 1%
                //800E8001    // Increase ChA amplitude (Add 12mV)
                //801f0000    //page 0
                //800e0660    //Output MLT-3
                //80100080    //MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x86A9);
                MP_WritePhyUshort(Adapter,0x0E, 0x8001);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x0E, 0x0660);   //Output MLT-3
                MP_WritePhyUshort(Adapter,0x10, 0x0080);   //MDIX
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Channel B:
                //801f0001   //page 1
                //80191f00   // MDIX
                //801f0000    //page 0
                //80002100    //Force 100M
                MP_WritePhyUshort(Adapter,0x1F, 0x0001);
                MP_WritePhyUshort(Adapter,0x19, 0x1F00);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //100 channel B
                //801F0000
                //801C40C0
                //80002100
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C0);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //100 channel B
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C0);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //100 channel B
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue &= ~BIT8;
                PhyRegValue |= BIT9;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        }
}

void RTK_10M_Return_loss_Finish(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        //Finish 10M:
        //801f0000   //page 0
        //800e0000   // Disable MLT-3

        switch(Adapter->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Enable ALDPS
                //801f0007, 801e0023, 80170116, 80009200
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f000, 801f0000, 801001ee, 801f0000

                //Enable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1F, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01EE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Enable ALDPS (Unlock Main)
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 0      (Unlock main)
                //801f0002, 801f0000, 80009200
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f000, 801f0000, 801001ee, 801f0000

                //Enable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01EE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Enable ALDPS (Unlock Main)
                //801f0007, 801e0023,
                //80180000 Set Reg 0x18 Bit[0] = 0      (Unlock main)
                //801f0000, 80009200
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f040, 801f0000, 8010096e, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF040);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x096E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                Finish 10M return loss
                801F0A43 // page 0x0A43
                8010xxxx => set reg 0x10 to bit[9,8] = 2'b 00 //Auto MDI / MDIX
                */
/*
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue &= ~(BIT8|BIT9);
                MP_WritePhyUshort(Adapter,0x10, PhyRegValue);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue |= (BIT2);
                MP_WritePhyUshort(Adapter,0x1c, PhyRegValue);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Finish 10M:
                //801f0000   //page 0
                //800401E1   // 100M Nway
                //801f0001   //page 1
                //80190003   // Auto MDIX
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x04,0x01E1);
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x0003);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //Finish 10M
                //801F0000
                //801C40C6
                //80001200
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x1C,0x40C6);
                MP_WritePhyUshort(Adapter,0x00,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //Finish 10M
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C6);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //Finish 10M
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue |= BIT8;
                PhyRegValue &= ~BIT9;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}
void RTK_10M_Return_Loss_Channel_A(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        //800e0700 (Fix MDI/MDIX and force power 10M TX)

        switch(Adapter->ChipType) {
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Channel A:
                //Disable ALDPS
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us)
                //Force MDI
                //801f0007, 801e002d, 8018f020, 801f0000, 801001ae, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x17,0x0117);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //MP_WritePhyUshort(Adapter,0x00,0x9200);
                Sleep(1000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01AE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Channel A:
                //Disable ALDPS (?ªå?Lock Main)
                //801f0004, 801f0007, 801e0023
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000
                //Force MDI
                //801f0004, 801f0007, 801e002d, 8018f020, 801f0002, 801f0000, 801001ae, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01AE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Channel A:
                //Disable ALDPS (?ªå?Lock Main)
                //801f0007, 801e0023
                //80180001  Set Reg 0x18 Bit[0] = 1      ( Lock main)
                //801f0000
                //Force MDI
                //801f0007, 801e002d, 8018f060, 801f0000, 8010092e, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF060);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x092E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                10M channel A:
                801F0A43 // page 0x0A43
                8010xxxx => set reg 0x10 to bit[9,8] = 2'b 11 //force mdi mode
                */
/*
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue |= (BIT8|BIT9);
                MP_WritePhyUshort(Adapter,0x10, PhyRegValue);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x04,0x0061);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue &= ~(BIT2|BIT1);
                PhyRegValue |= (BIT1);
                MP_WritePhyUshort(Adapter,0x1c, PhyRegValue);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Channel A:
                //801f0000   //page 0
                //80040061   // 10M Nway
                //801f0001   //page 1
                //80190001   // MDI
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x04,0x0061);
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x0001);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //10 channel A
                //801F0000
                //801C40C2
                //80000100
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x1C,0x40C2);
                MP_WritePhyUshort(Adapter,0x00,0x0100);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C2);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue |= (BIT8|BIT9);
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                break;
        }
}

void RTK_10M_Return_Loss_Channel_B(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        //800e0700 (Fix MDI/MDIX and force power 10M TX)

        switch(Adapter->ChipType) {
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Channel B:
                //Disable ALDPS
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us)
                //Force MDIX
                //801f0007, 801e002d, 8018f020, 801f0000, 8010018e, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F,0x0007);
                MP_WritePhyUshort(Adapter,0x1E,0x0023);
                MP_WritePhyUshort(Adapter,0x17,0x0117);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                //MP_WritePhyUshort(Adapter,0x00,0x9200);
                Sleep(1000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x018E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Channel B:
                //Disable ALDPS (?ªå?Lock Main)
                //801f0004, 801f0007, 801e0023
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000
                //Force MDIX
                //801f0004, 801f0007, 801e002d, 8018f020, 801f0002, 801f0000, 8010018e, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x018E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Channel A:
                //Disable ALDPS (?ªå?Lock Main)
                //801f0007, 801e0023
                //80180001 Set Reg 0x18 Bit[0] = 1      ( Lock main)
                //801f0000
                //Force MDIX
                //801f0007, 801e002d, 8018f060, 801f0000, 8010090e, 801f0000

                //Disable ALDPS
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF060);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x090E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                10M channel B:
                801F0A43 // page 0x0A43
                8010xxxx => set reg 0x10 to bit[9,8] = 2'b 10 //force mdix mode
                */
/* original
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue &= ~(BIT8);
                PhyRegValue |= BIT9;
                MP_WritePhyUshort(Adapter,0x10, PhyRegValue);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x04,0x0061);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue &= ~(BIT2|BIT1);
                MP_WritePhyUshort(Adapter,0x1c, PhyRegValue);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Channel B:
                //801f0000   //page 0
                //80040061   // 10M Nway
                //801f0001   //page 1
                //80190000   // MDIX
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x04,0x0061);
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x0000);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //10 channel B
                //801F0000
                //801C40C0
                //80000100
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x1C,0x40C0);
                MP_WritePhyUshort(Adapter,0x00,0x0100);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C0);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue &= ~BIT8;
                PhyRegValue |= BIT9;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x0100);
                break;
        }
}

void RTK_100M_Return_loss_Finish(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

        //Finish 100M:
        //801f0000   //page 0
        //800e0000   // Disable MLT-3

        switch(Adapter->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f000, 801f0000, 801001ee, 801f0000
                //ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01EE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Auto MDI / MDIX
                //801f0004, 801f0007, 801e002d, 8018f000, 801f0002, 801f0000, 801001ee, 801f0000
                //ForceDefaultPow
                //801f0004, 801f0007, 801e002f, 8017188f, 801f0002, 801f0000
                //Release Nctl
                //801f0004, 801f0007, 801e0023, 80160306, 80160300,
                //Set Reg 0x17 Bit[0] = 0      (Unlock main)
                //801f0002, 801f0000, 80009200
                //RstDaFIFO
                //801f0004, 801f0007, 801e002c, 801800c3, 801800cb, 801f0002, 801f0000

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01EE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0306);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Auto MDI / MDIX
                //801f0007, 801e002d, 8018f040, 801f0000, 8010096e, 801f0000
                //ForceDefaultPow
                //801f0007, 801e002f, 8017d88f, 801f0000
                //Release Nctl
                //801f0007, 801e0023, 80160306, 80160300,
                //80180000 Set Reg 0x18 Bit[0] = 0      (Unlock main)
                //801f0000, 80009200
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //Auto MDI / MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF040);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x096E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD88F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0306);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x18, 0x0000);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                finish 100M:
                801f0a43,
                8010xxxx=> reg0x10 to bit[9,8] = 2'b 00 //enable auto-crossover
                801F0A40 // page 0x0A40
                80109200 // page 0xa40, reg0 to reset phy & renway
                */
/*  original
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue &= ~(BIT9 | BIT8);
                MP_WritePhyUshort(Adapter,0x10,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                MP_WritePhyUshort(Adapter,0x10, 0x9200);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue |= (BIT2);
                MP_WritePhyUshort(Adapter,0x1c,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Finish 100M:
                //801f0001   //page 1
                //80190003   // Auto MDIX
                //801f0000    //page 0
                //80001200    //Restart Nway
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x0003);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;

        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //Finish 100M
                //801F0000
                //801C40C6
                //80001200
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C6);
                MP_WritePhyUshort(Adapter,0x00, 0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //Finish 100M
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C6);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //Finish 100M
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue |= BIT8;
                PhyRegValue &= ~BIT9;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x1200);

                Sleep(3000); // Wait NWay complete.
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_100M_Return_Loss_Channel_A(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

//Channel A:
        //801f0000    //page 0
        //800e0660    //Output MLT-3

        switch(Adapter->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0660);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //Channel A:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Force MDI
                //801f0007, 801e002d, 8018f020, 801f0000, 801001ae, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 80171818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01AE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Channel A:
                //LockNctl
                //801f0004, 801f0007, 801e0023
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000, wait(100ms),  80150006, 801f0004, 801f0007, 801e0023, 80160006, 801f0002, 801f0000
                //Force MDI
                //801f0004, 801f0007, 801e002d, 8018f020, 801f0002, 801f0000, 801001ae, 801f0000
                //Force100Pow
                //801f0004, 801f0007, 801e002f, 8017d818, 801f0002, 801f0000
                //RstDaFIFO
                //801f0004, 801f0007, 801e002c, 801800c3, 801800cb, 801f0002, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                Sleep(100);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x01AE);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Channel A:
                //LockNctl
                //801f0007, 801e0023
                //80180001  Set Reg 0x18 Bit[0] = 1      ( Lock main)
                //wait(20ms), 80160006, 801f0000
                //Force MDI
                //801f0007, 801e002d, 8018f060, 801f0000, 8010092e, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 8017d818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000
                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                Sleep(20);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDI
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF060);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x092E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                Channel A:
                801F0A40 // page 0x0A40
                80102100 // force 100M
                801f0a43,
                8010xxxx=> reg0x10 to bit[9,8] = 2'b 11 //p0 reg24 bit[8]=1 mdi mode
                */
/*  original
                MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                MP_WritePhyUshort(Adapter,0x10, 0x2100);
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue |= (BIT9 | BIT8);
                MP_WritePhyUshort(Adapter,0x10,PhyRegValue);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x04,0x0061);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue &= ~(BIT2);
                PhyRegValue |= (BIT1);
                MP_WritePhyUshort(Adapter,0x1c,PhyRegValue);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Channel A:
                //801f0001   //page 1
                //80191f01   //MDI
                //801f0000   //page 0
                //80002100   // Force 100M
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x1F01);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x2100);
                break;

        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //100 channel A
                //801F0000
                //801C40C2
                //80002100
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C2);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //100 channel A
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C2);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //100 channel A
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue |= (BIT8|BIT9);
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        }
}

void RTK_100M_Return_Loss_Channel_B(PMP_ADAPTER Adapter)
{
        USHORT PhyRegValue;

//Channel B:
        //801f0000    //page 0
        //800e0660    //Output MLT-3
        //80100080    //MDIX

        switch(Adapter->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0660);
                MP_WritePhyUshort(Adapter,0x10, 0x0080);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Force MDIX
                //801f0007, 801e002d, 8018f020, 801f0000, 8010018e, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 80171818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x018E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                //Channel B:
                //LockNctl
                //801f0004, 801f0007, 801e0023,
                //Set Reg 0x17 Bit[0] = 1      ( Lock main)
                //801f0002, 801f0000, wait(100ms),  80150006, 801f0004, 801f0007, 801e0023, 80160006, 801f0002, 801f0000
                //Force MDIX
                //801f0004, 801f0007, 801e002d, 8018f020, 801f0002, 801f0000, 8010018e, 801f0000
                //Force100Pow
                //801f0004, 801f0007, 801e002f, 8017d818, 801f0002, 801f0000
                //RstDaFIFO
                //801f0004, 801f0007, 801e002c, 801800c3, 801800cb, 801f0002, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x17);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x17,PhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                Sleep(100);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF020);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x018E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Channel B:
                //LockNctl
                //801f0007, 801e0023
                //80180001  Set Reg 0x18 Bit[0] = 1      ( Lock main)
                //wait(20ms), 80160006, 801f0000
                //Force MDIX
                //801f0007, 801e002d, 8018f040, 801f0000, 8010090e, 801f0000
                //Force100Pow
                //801f0007, 801e002f, 8017d818, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x18, 0x0001);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x18);
                PhyRegValue |= BIT0;
                MP_WritePhyUshort(Adapter,0x18,PhyRegValue);
                Sleep(20);
                MP_WritePhyUshort(Adapter,0x16, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force MDIX
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002D);
                MP_WritePhyUshort(Adapter,0x18, 0xF060);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x10, 0x090E);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Force100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0xD818);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                Channel B:
                801F0A40 // page 0x0A40
                80102100 // force 100M
                801f0a43,
                8010xxxx=> reg0x10 to bit[9,8] = 2'b 10 //p0 reg24 bit[8]=0 mdix mode
                */
/* original
                MP_WritePhyUshort(Adapter,0x1F, 0x0A40);
                MP_WritePhyUshort(Adapter,0x10, 0x2100);
                MP_WritePhyUshort(Adapter,0x1F, 0x0A43);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x10);
                PhyRegValue &= ~(BIT8);
                PhyRegValue |= (BIT9);
                MP_WritePhyUshort(Adapter,0x10,PhyRegValue);
*/
                MP_WritePhyUshort(Adapter,0x1F, 0);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                MP_WritePhyUshort(Adapter,0x1F, 0);
		MP_WritePhyUshort(Adapter,0x04,0x0061);
                PhyRegValue = MP_ReadPhyUshort(Adapter,0x1c);
                PhyRegValue &= ~(BIT2|BIT1);
                MP_WritePhyUshort(Adapter,0x1c,PhyRegValue);
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
        case RTL8401:
                //Channel B:
                //801f0001   //page 1
                //80191f00   //MDIX
                //801f0000   //page 0
                //80002100   // Force 100M
                MP_WritePhyUshort(Adapter,0x1F,0x0001);
                MP_WritePhyUshort(Adapter,0x19,0x1F00);
                MP_WritePhyUshort(Adapter,0x1F,0x0000);
                MP_WritePhyUshort(Adapter,0x00,0x2100);
                break;

        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
        case RTL8402:
                //100 channel B
                //801F0000
                //801C40C0
                //80002100
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x1C, 0x40C0);
                MP_WritePhyUshort(Adapter,0x00, 0x2100);
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                //100 channel B
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0x2000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB018,0x40C0);
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                //100 channel B
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                ReadUshort(Adapter,PLAMCU_OCP,0xB430,&PhyRegValue);
                PhyRegValue &= ~BIT8;
                PhyRegValue |= BIT9;
                WriteUshort(Adapter,PLAMCU_OCP,0xB430,PhyRegValue);
                WriteUshort(Adapter,PLAMCU_OCP,0xB400,0x2100);
                break;
        }
}

void RTK_1000M_Return_loss_Finish(PMP_ADAPTER Adapter)
{

//Finish 1000M:
        //801f0000   //page 0
        //80090000   // Disable Test mode

        switch(Adapter->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Disable Test Mode
                //80090000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Disable Test Mode
                MP_WritePhyUshort(Adapter,0x09, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Disable Test Mode
                //80090000

                //Disable Test Mode
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                801F0A41 // page 0x0A41
                80110000
                */
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x0000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x0000);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_1000M_Return_Loss(PMP_ADAPTER Adapter)
{
//801f0000    //page 0
        //80098000

        switch(Adapter->ChipType) {
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x8000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Test Mode 4
                //80098000
                //ForceGigaPow
                //801f0007, 801e002f, 80171820, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                ////Test Mode 4
                MP_WritePhyUshort(Adapter,0x09, 0x8000);

                //ForceGiga100Pow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1820);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Test Mode 4
                //80098000

                //Test Mode 4
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x8000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                1000M return loss
                801F0A41 // page 0x0A41
                80118000
                */
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x8000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x8000);
                break;
        }
}

void RTK_1000M_Test_Mode_1_Finish(PMP_ADAPTER Adapter)
{
        USHORT TmpUshort;

        //8168C Test mode1
        //when exiting test mode1, increase amplitude
        //by hao
        switch(Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //Finish Test mode 1:
                //801f0002   //page 2
                //80048100
                //80048100
                //801f0000   //page 0
                //80090000
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x04, 0x8100);
                MP_WritePhyUshort(Adapter,0x01, 0x82B1);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x00, 0x86a0);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Disable Test Mode
                //80090000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Disable Test Mode
                MP_WritePhyUshort(Adapter,0x09, 0x0000);

                //Release Nctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                /*
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0046);
                MP_WritePhyUshort(Adapter,0x18, m_usPhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                */
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:

                //Disable Test Mode
                //80090000
                //801F0007
                //801E0046
                //0017xxxx
                //8018xxxx, xxxx= Rx RC codeword
                //801E0078
                //8014 Bit[0] = 0
                //801F0000

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0046) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x17) ;
                MP_WritePhyUshort(Adapter,0x18, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0078) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x14) ;
                TmpUshort &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x14, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1F, 0x0000) ;

                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0A41 // page 0x0A41
                //80110000
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x0000);
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //801f0000   //page 0
                //80090000
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x0000);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_1000M_Test_Mode_1(PMP_ADAPTER Adapter)
{
        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x2c00);
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                // Switch PHY to page 2.
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x6500);
                // Switch PHY back to page 0.
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x2c00);
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //1000M Test mode 1:
                //801f0002  (page2)
                //8001c2d1  // Increase Amplitude
                //8004810C  //Set power giga and getpowctrl=1
                //801f0000  (page0)
                //80092000  (Test mode 1)
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x01, 0xC2D1);
                MP_WritePhyUshort(Adapter,0x04, 0x810C);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x2000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x00, 0x86a8);    // Increase Amplitude
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);    //page 0
                MP_WritePhyUshort(Adapter,0x0e, 0x0660);
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);    // Reset 100M MLT-3
                MP_WritePhyUshort(Adapter,0x09, 0x2000);    //Test mode 1
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0046);
                MP_WritePhyUshort(Adapter,0x18, 0x2222);

                //	1000M Test mode 1:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Test Mode 1
                //80092000
                //ForceGigaPow
                //801f0007, 801e002f, 80171820, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Test Mode 1
                MP_WritePhyUshort(Adapter,0x09, 0x2000);

                //ForceGigaPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1820);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //	1000M Test mode 1:
                //Test Mode 1
                //80092000

                //Test Mode 1
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x2000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                801F0A41 // page 0x0A41
                80112000
                */
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x2000);
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //801f0000   //page 0
                //80092000   //Test mode 1
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x2000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x2000);
                break;
        default:
                break;
        }
}

void RTK_1000M_Test_Mode_2_Finish(PMP_ADAPTER Adapter)
{
        USHORT TmpUshort;

        switch(Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //Finish Test mode 2:
                //801f0000   //Page 0
                //80090000   // Disable Test mode
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x0000);   // Disable Test mode
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x0000);   // Disable Test mode
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Disable Test Mode
                //80090000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Disable Test Mode
                MP_WritePhyUshort(Adapter,0x09, 0x0000);

                //Giga Unbalance Pattern
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                /*
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0046);
                MP_WritePhyUshort(Adapter,0x18, m_usPhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                */
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:

                //Disable Test Mode
                //80090000
                //801F0007
                //801E0046
                //0017xxxx
                //8018xxxx, xxxx= Rx RC codeword
                //801E0078
                //8014 Bit[0] = 0
                //801F0000

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0046) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x17) ;
                MP_WritePhyUshort(Adapter,0x18, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0078) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x14) ;
                TmpUshort &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x14, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1F, 0x0000) ;

                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0A41 // page 0x0A41
                //80110000
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x0000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x0000);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_1000M_Test_Mode_2(PMP_ADAPTER Adapter)
{
        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x5B00);
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                // Switch PHY to page 2.
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x6400);
                // Switch PHY back to page 0.
                MP_WritePhyUshort(Adapter,0x1f, 0x000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x5B00);
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //1000M Test mode 2:
                //801f0000   //page 0
                //80094000   // Test mode 2
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x4000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x4000);   // Test mode 3
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Test Mode 2
                //80094000
                //ForceGigaPow
                //801f0007, 801e002f, 80171820, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Test Mode 2
                MP_WritePhyUshort(Adapter,0x09, 0x4000);

                //ForceGigaPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1820);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Test Mode 2
                //80094000

                //Test Mode 2
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x4000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                801F0A41 // page 0x0A41
                80114000
                */
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x4000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x4000);
                break;
        }
}

void RTK_1000M_Test_Mode_3_Finish(PMP_ADAPTER Adapter)
{
        USHORT TmpUshort;

        switch(Adapter->ChipType) {
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //Finish Test mode 3:
                //801f0000   //Page 0
                //80090000   // Disable Test mode
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x0000);   // Disable Test mode
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x0000);   // Disable Test mode
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                ///ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Disable Test Mode
                //80090000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Disable Test Mode
                MP_WritePhyUshort(Adapter,0x09, 0x0000);

                //Giga Unbalance Pattern
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                /*
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0046);
                MP_WritePhyUshort(Adapter,0x18, m_usPhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                */
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Disable Test Mode
                //80090000
                //801F0007
                //801E0046
                //0017xxxx
                //8018xxxx, xxxx= Rx RC codeword
                //801E0078
                //8014 Bit[0] = 0
                //801F0000

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0046) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x17) ;
                MP_WritePhyUshort(Adapter,0x18, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0078) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x14) ;
                TmpUshort &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x14, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1F, 0x0000) ;

                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0A41 // page 0x0A41
                //80110000

                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x0000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x0000);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_1000M_Test_Mode_3(PMP_ADAPTER Adapter)
{
        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                MP_WritePhyUshort(Adapter,0xe, 0x0210);
                MP_WritePhyUshort(Adapter,0x9, 0x7300);
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                // Switch PHY to page 2.
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x6400);
                // Switch PHY back to page 0.
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x7300);
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //801f0000   //page 0
                //80096000   // Test mode 3
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x6000);   // Test mode 3
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x6000);   // Test mode 3
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Test Mode 3
                //80096000
                //ForceGigaPow
                //801f0007, 801e002f, 80171820, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Test Mode 3
                MP_WritePhyUshort(Adapter,0x09, 0x6000);

                //ForceGigaPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1820);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
        case RTL8411:
                //Test Mode 3
                //80096000

                //Test Mode 3
                MP_WritePhyUshort(Adapter,0x1F, 0x0002);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x6000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                /*
                801F0A41 // page 0x0A41
                80116000
                */
                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x6000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x6000);
                break;
        }
}

void RTK_1000M_Test_Mode_4_Finish(PMP_ADAPTER Adapter)
{
        USHORT TmpUshort;

        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                //Disable test mode4
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x06, 0x2236);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                //Disable test mode4
                MP_WritePhyUshort(Adapter,0x0e, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //Finish Test mode 4:
                //801f0000   //page 0
                //80090000   // Disable Test mode
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x06, 0x3461);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //ForceDefaultPow
                //801f0007, 801e002f, 8017188f, 801f0000
                //Disable Test Mode
                //80090000
                //Release Nctl
                //801f0007, 801e0023, 80170116, 80160300, 801f0000, 80151006
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //ForceDefaultPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x188F);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Disable Test Mode
                MP_WritePhyUshort(Adapter,0x09, 0x0000);

                //Giga Unbalance Pattern
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0116);
                MP_WritePhyUshort(Adapter,0x16, 0x0300);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(3000); // Wait NWay complete.

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                /*
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0046);
                MP_WritePhyUshort(Adapter,0x18, m_usPhyRegValue);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                */
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
                //Disable Test Mode
                //801F0000
                //80090000
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;

        case RTL8411:

                //Disable Test Mode
                //80090000
                //801F0007
                //801E0046
                //0017xxxx
                //8018xxxx, xxxx= Rx RC codeword
                //801E0078
                //8014 Bit[0] = 0
                //801F0000

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0046) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x17) ;
                MP_WritePhyUshort(Adapter,0x18, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1E, 0x0078) ;
                TmpUshort = MP_ReadPhyUshort(Adapter,0x14) ;
                TmpUshort &= ~(BIT0);
                MP_WritePhyUshort(Adapter,0x14, TmpUshort) ;
                MP_WritePhyUshort(Adapter,0x1F, 0x0000) ;

                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //801f0000   //page 0
                //80090000   // Disable Test mode
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x0000);
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0A41 // page 0x0A41
                //80110000

                MP_WritePhyUshort(Adapter,0x1f, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x0000);
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x0000);
                break;
        }

        if(Adapter->testnicinfo->busfeature & PCI_NIC)
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
}

void RTK_1000M_Test_Mode_4(PMP_ADAPTER Adapter)
{
        USHORT TmpUshort;

        switch(Adapter->ChipType) {
        case RTL8169S1:
        case RTL8169S2:
        case RTL8169SB:
        case RTL8169SC:
        case RTL8169SC_REV_E:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x06, 0x223F);
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x8300);
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                // Switch PHY to page 2.
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);
                MP_WritePhyUshort(Adapter,0x00, 0x6400);
                MP_WritePhyUshort(Adapter,0x06, 0x223F);
                // Switch PHY back to page 0.
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x0e, 0x0210);
                MP_WritePhyUshort(Adapter,0x09, 0x8300);
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                //801f0000   //page 0
                //80098000   //Test mode 4
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x8000);
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                MP_WritePhyUshort(Adapter,0x1f, 0x0002);   //page 2
                MP_WritePhyUshort(Adapter,0x06, 0x7761);   // Increase Tx bias/IQ current
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x8000);   //Test mode 4
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                //LockNctl
                //LockNctl
                //801f0007, 801e0023, 80170117, 801f0000, 80009200, wait(300us), 80150006, 801f0007, 801e0023, 80160004, 801f0000
                //Test Mode 4
                //80098000
                //ForceGigaPow
                //801f0007, 801e002f, 80171820, 801f0000
                //RstDaFIFO
                //801f0007, 801e002c, 801800c3, 801800cb, 801f0000

                //LockNctl
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x17, 0x0117);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                //MP_WritePhyUshort(Adapter,0x00, 0x9200);
                Sleep(1000);
                MP_WritePhyUshort(Adapter,0x15, 0x0006);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0023);
                MP_WritePhyUshort(Adapter,0x16, 0x0004);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //Test Mode 4
                MP_WritePhyUshort(Adapter,0x09, 0x8000);

                //ForceGigaPow
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002F);
                MP_WritePhyUshort(Adapter,0x17, 0x1820);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);

                //RstDaFIFO
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x002C);
                MP_WritePhyUshort(Adapter,0x18, 0x00C3);
                MP_WritePhyUshort(Adapter,0x18, 0x00CB);
                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
        case RTL8168FB:
        case RTL8168FB_REV_B:
                //Test Mode 4
                //801F0000
                //80098000

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x8000);
                break;

        case RTL8411:
                //Test Mode 4
                //80098000
                //801F0007
                //801E0046
                //8018FFFF
                //801E0078
                //8014 Bit[0] = 1
                //801F0000

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                MP_WritePhyUshort(Adapter,0x09, 0x8000);
                MP_WritePhyUshort(Adapter,0x1F, 0x0007);
                MP_WritePhyUshort(Adapter,0x1E, 0x0046);
                MP_WritePhyUshort(Adapter,0x18, 0xFFFF);
                MP_WritePhyUshort(Adapter,0x1E, 0x0078);

                TmpUshort= MP_ReadPhyUshort(Adapter,0x14);
                TmpUshort |= BIT0 ;
                MP_WritePhyUshort(Adapter,0x14, TmpUshort);

                MP_WritePhyUshort(Adapter,0x1F, 0x0000);
                break;

        case RTL8168G:
        case RTL8168G_REV_B:
        case RTL8168GU:
        case RTL8168GU_REV_B:
        case RTL8168H:
        case RTL8411B:
        case RTL8168EP:
        case RTL8168EP_REV_B:
        case RTL8168EP_REV_C:
                //801F0A41 // page 0x0A41
                //80118000
                MP_WritePhyUshort(Adapter,0x1F, 0x0a41);
                MP_WritePhyUshort(Adapter,0x11, 0x8000);
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                //801f0000   //page 0
                //80098000   //Test mode 4
                MP_WritePhyUshort(Adapter,0x1f, 0x0000);   //page 0
                MP_WritePhyUshort(Adapter,0x09, 0x8000);   //Test mode 4
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                WriteUshort(Adapter,PLAMCU_OCP,0xE86C,0xA000);
                WriteUshort(Adapter,PLAMCU_OCP,0xB412,0x8000);
                break;
        }
}
