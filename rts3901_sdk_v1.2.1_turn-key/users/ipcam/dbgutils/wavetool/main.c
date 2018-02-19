#include <getopt.h>
#include "common.h"
#include "rtl.h"


static const struct rtk_wave_form_test_vector Wave_Form_Test_Vector[] = {
        {
                .rtk_test_vector_name = "10M Harmonic-all 1",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_10M_Harmonic,
                .rtk_wave_form_test_finish_fun = RTK_10M_Haromonic_Finish,
        },
        {
                .rtk_test_vector_name = "10M Harmonic-all 0",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_10M_Harmonic,
                .rtk_wave_form_test_finish_fun = RTK_10M_Haromonic_Finish,
        },        
        {
                .rtk_test_vector_name = "10M Normal",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_10M_Normal,
                .rtk_wave_form_test_finish_fun = RTK_10M_Normal_Finish,
        },
        {
                .rtk_test_vector_name = "10M Link Pluse",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_10M_Link_Pluse,
                .rtk_wave_form_test_finish_fun = RTK_10M_Normal_Finish,
        },        
        {
                .rtk_test_vector_name = "100M Channel A",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_100M_Channel_A,
                .rtk_wave_form_test_finish_fun = RTK_100M_Finish,
        },
        {
                .rtk_test_vector_name = "100M Channel B",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_100M_Channel_B,
                .rtk_wave_form_test_finish_fun = RTK_100M_Finish,
        },
        {
                .rtk_test_vector_name = "10M Return Loss Channel A",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_10M_Return_Loss_Channel_A,
                .rtk_wave_form_test_finish_fun = RTK_10M_Return_loss_Finish,
        },
        {
                .rtk_test_vector_name = "10M Return Loss Channel B",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_10M_Return_Loss_Channel_B,
                .rtk_wave_form_test_finish_fun = RTK_10M_Return_loss_Finish,
        },
        {
                .rtk_test_vector_name = "100M Return Loss Channel A",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_100M_Return_Loss_Channel_A,
                .rtk_wave_form_test_finish_fun = RTK_100M_Return_loss_Finish,
        },
        {
                .rtk_test_vector_name = "100M Return Loss Channel B",
                .support_bus_feature = (FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_100M_Return_Loss_Channel_B,
                .rtk_wave_form_test_finish_fun = RTK_100M_Return_loss_Finish,
        },
        {
                .rtk_test_vector_name = "1000M Return Loss",
                .support_bus_feature = (GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_1000M_Return_Loss,
                .rtk_wave_form_test_finish_fun = RTK_1000M_Return_loss_Finish,
        },
        {
                .rtk_test_vector_name = "1000M Test Mode 1",
                .support_bus_feature = (GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_1000M_Test_Mode_1,
                .rtk_wave_form_test_finish_fun = RTK_1000M_Test_Mode_1_Finish,
        },
        {
                .rtk_test_vector_name = "1000M Test Mode 2",
                .support_bus_feature = (GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_1000M_Test_Mode_2,
                .rtk_wave_form_test_finish_fun = RTK_1000M_Test_Mode_2_Finish,
        },
        {
                .rtk_test_vector_name = "1000M Test Mode 3",
                .support_bus_feature = (GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_1000M_Test_Mode_3,
                .rtk_wave_form_test_finish_fun = RTK_1000M_Test_Mode_3_Finish,
        },
        {
                .rtk_test_vector_name = "1000M Test Mode 4",
                .support_bus_feature = (GIGA_NIC),
                .rtk_wave_form_test_start_fun = RTK_1000M_Test_Mode_4,
                .rtk_wave_form_test_finish_fun = RTK_1000M_Test_Mode_4_Finish,
        },
        {
                .rtk_test_vector_name = "Exit",
                .support_bus_feature = (PCI_NIC|USB_NIC|FE_NIC|GIGA_NIC),
                .rtk_wave_form_test_start_fun = NULL,
                .rtk_wave_form_test_finish_fun = NULL,
        },
};

const int total_vector = sizeof(Wave_Form_Test_Vector)/sizeof(struct rtk_wave_form_test_vector);


/* functions definition */
static void print_chip_name(rt_nic_info *allnicinfo)
{
        if (allnicinfo == NULL) return;

        switch(allnicinfo->ChipType) {
        case RTL8169:
                printf("           Adapter Name: RTL8169 \n");
                break;
        case RTL8169S1:
        case RTL8169S2:
                printf("           Adapter Name: RTL8169S \n");
                break;
        case RTL8169SB:
                printf("           Adapter Name: RTL8169SB \n");
                break;
        case RTL8169SC:
        case RTL8169SC_REV_E:
                printf("           Adapter Name: RTL8110SC \n");
                break;
        case RTL8168B:
        case RTL8168B_REV_E:
        case RTL8168B_REV_F:
                printf("           Adapter Name: RTL8168B \n");
                break;
        case RTL8136:
        case RTL8136_REV_E:
                printf("           Adapter Name: RTL8136 \n");
                break;
        case RTL8101:
        case RTL8101_REV_C:
                printf("           Adapter Name: RTL8101E \n");
                break;
        case RTL8168C:
        case RTL8168C_REV_G:
        case RTL8168C_REV_K:
        case RTL8168C_REV_M:
                printf("           Adapter Name: RTL8168C \n");
                break;
        case RTL8102E:
        case RTL8102EL:
        case RTL8102E_EL_REV_B:
        case RTL8102E_EL_REV_C:
        case RTL8103:
        case RTL8103E_EL_REV_B:
        case RTL8103E_EL_REV_C:
                printf("           Adapter Name: RTL8102E/RTL8103E \n");
                break;
        case RTL8401:
                if(allnicinfo->Nic8401ChangeTo8103EVL)
                        printf("           Adapter Name: RTL8103E-VL \n");
                else
                        printf("           Adapter Name: RTL8401 \n");
                break;
        case RTL8105E_REV_C:
        case RTL8105E_REV_D:
        case RTL8105E_REV_F:
        case RTL8105E_REV_G:
        case RTL8105E_REV_H:
                printf("           Adapter Name: RTL8105E \n");
                break;
        case RTL8106E_REV_A:
        case RTL8106E_REV_B:
                printf("           Adapter Name: RTL8106E \n");
                break;
        case RTL8168D:
        case RTL8168D_REV_B:
        case RTL8168D_REV_C_REV_D:
                if(allnicinfo->Nic8168DChangeTo8104)
                        printf("           Adapter Name: RTL8104E \n");
                else
                        printf("           Adapter Name: RTL8168D \n");
                break;
        case RTL8168E:
        case RTL8168E_REV_B:
        case RTL8168E_REV_C:
                if(allnicinfo->Nic8168EChangeTo8105)
                        printf("           Adapter Name: RTL8105E \n");
                else
                        printf("           Adapter Name: RTL8168E \n");
                break;
        case RTL8168F:
        case RTL8168F_REV_B:
                if(allnicinfo->Nic8168FChangeTo8168E)
                        printf("           Adapter Name: RTL8168E-VL \n");
                else
                        printf("           Adapter Name: RTL8168F \n");
                break;
        case RTL8168FB:
        case RTL8168FB_REV_B:
                if(allnicinfo->Nic8168FChangeTo8168E)
                        printf("           Adapter Name: RTL8168E-VL \n");
                else if(allnicinfo->Nic8168FBChangeTo8119)
                        printf("           Adapter Name: RTL8119 \n");
                else
                        printf("           Adapter Name: RTL8168FB \n");
                break;
        case RTL8168G:
        case RTL8168G_REV_B:
                printf("           Adapter Name: RTL8168G \n");
                break;
        case RTL8168GU:
        case RTL8168GU_REV_B:
                printf("           Adapter Name: RTL8168GU \n");
                break;
        case RTL8168H:
                if(allnicinfo->Nic8168HChangeTo8107E)
                        printf("           Adapter Name: RTL8107E \n");
                else if(allnicinfo->Nic8168HChangeTo8118AS)
                        printf("           Adapter Name: RTL8118AS \n");
                else
                        printf("           Adapter Name: RTL8168H \n");
                break;
        case RTL8168CP:
        case RTL8168CP_REV_B:
        case RTL8168CP_REV_C:
        case RTL8168CP_REV_D:
                printf("           Adapter Name: RTL8168CP \n");
                break;
        case RTL8168DP:
        case RTL8168DP_REV_B:
        case RTL8168DP_REV_E:
        case RTL8168DP_REV_F:
                printf("           Adapter Name: RTL8168DP \n");
                break;
        case RTL8168EP:
        case RTL8168EP_REV_B:
                printf("           Adapter Name: RTL8168EP \n");
                break;
        case RTL8411:
                if(allnicinfo->Nic8411ChangeTo8411AAR)
                        printf("           Adapter Name: RTL8411AAR \n");
                else
                        printf("           Adapter Name: RTL8411 \n");
                break;
        case RTL8411B:
                printf("           Adapter Name: RTL8411B \n");
                break;
        case RTL8402:
                printf("           Adapter Name: RTL8402 \n");
                break;
        case RTL8152_REV_A:
        case RTL8152_REV_B:
                printf("           Adapter Name: RTL8152 \n");
                break;
        case RTL8153_REV_A:
        case RTL8153_REV_B:
        case RTL8153_REV_C:
                printf("           Adapter Name: RTL8153 \n");
                break;
        default:
                printf("           Adapter Name: Unknown \n");
                break;
        }
}

static void print_info(PMP_ADAPTER Adapter)
{
        int i;
        rt_nic_info *allnicinfo = Adapter->allnicinfo;

        for (i=0; i < Adapter->nicnum; i++) {
                if (Adapter->curridx == i)
                        printf(BOLD YELLOW);

                printf("%d) %6s: %s ", i, (allnicinfo+i)->name, (allnicinfo+i)->desc);
                printf("(%02X:%02X:%02X:%02X:%02X:%02X)\n", (allnicinfo+i)->macaddr[0],
                       (allnicinfo+i)->macaddr[1], (allnicinfo+i)->macaddr[2],
                       (allnicinfo+i)->macaddr[3], (allnicinfo+i)->macaddr[4],
                       (allnicinfo+i)->macaddr[5]);
                printf("           VID: %04X DID: %04X Bus-Info: %s\n",
                       (allnicinfo+i)->vid, (allnicinfo+i)->did, (allnicinfo+i)->bus_info);

                //if (Adapter->bShowDbgMsg)
                        print_chip_name((allnicinfo+i));

               // printf("           ChipType: %X", (allnicinfo+i)->ChipType);

                if (Adapter->curridx == i)
                        printf(NORMAL);

                printf("\n");
        }
        printf("\n");
}

/*
 * get supported cards and allocate memory
 * @param autorun: find interfaces automatically ?
 * @param nicnum: return supported NIC number
 * @return success or errcode
 */
static int get_adapter(PMP_ADAPTER Adapter)
{
        int i = 0, ret = 0;
        int nicnum = 0;

        if (!strlen(Adapter->devname))
                return -ECONFIG;

        /* get interface name/mac version */
        if ((nicnum = find_interface(Adapter->allnicinfo)) == 0) {
                ret = -EFINDNIC;
                goto get_adapter_exit;
        }

        Adapter->nicnum = nicnum;

        for (i = 0; i < nicnum; i++) {
                if (!strncmp(Adapter->allnicinfo[i].name, Adapter->devname, IFNAMSIZ)) {
                        Adapter->curridx = i;
                        Adapter->testnicinfo = Adapter->allnicinfo + Adapter->curridx;
                        Adapter->ChipType = Adapter->allnicinfo[Adapter->curridx].ChipType;
                        break;
                }
        }

        if (i >= nicnum)
                ret = -EFINDNIC;

get_adapter_exit:

        return ret;
}

static int Char_2_Num(char a)
{
        int num;

        if (a>='0' && a<='9') {
                num = a - '0';
        } else if(a>='a' && a<='f') {
                num = a - 'a' + 10;
        } else if(a>='A' && a<='F') {
                num = a - 'A' + 10;
        } else {
                num = ~0;
        }

        return num;
}

static int get_waveform_test_mode(PMP_ADAPTER Adapter)
{
        int i;
        int wave_form_test_mode = -1;
        u8 tmp_char;
        int ret = 0;

        do {
                printf ("Please Select Wave Form Test Mode:\n");
                for(i = 0; i<total_vector; i++) {
                        if(Wave_Form_Test_Vector[i].support_bus_feature & Adapter->testnicinfo->busfeature)
                                printf ("%X. %s.\n", i,  Wave_Form_Test_Vector[i].rtk_test_vector_name);
                }
                tmp_char = getchar();
                wave_form_test_mode = Char_2_Num(tmp_char);

                if(wave_form_test_mode >= 0 && wave_form_test_mode < total_vector) {
                        if(Wave_Form_Test_Vector[wave_form_test_mode].support_bus_feature & Adapter->testnicinfo->busfeature) {
                                Adapter->wave_form_test_mode = wave_form_test_mode;
                                break;
                        }
                }
        } while(TRUE);

        return ret;
}

static int start_waveform_test(PMP_ADAPTER Adapter)
{
        int ret = 0;
        u8 c = 0;
        bool bContinue = TRUE;

        enable_rtl_diag(Adapter->testnicinfo, 1);

        WaveFormInitAdapter(Adapter);

        do {
                bContinue = TRUE;
                Wave_Form_Test_Vector[Adapter->wave_form_test_mode].rtk_wave_form_test_start_fun(Adapter);
                printf ("Please Press Enter To Finish The Test");

                c = 0;
                while (c != '\n' && c != EOF)
                        c = getchar ();
                c = 0;
                while (c != '\n' && c != EOF)
                        c = getchar ();

                Wave_Form_Test_Vector[Adapter->wave_form_test_mode].rtk_wave_form_test_finish_fun(Adapter);
                printf ("Continue To Next Test\n");
                if ((ret = get_waveform_test_mode(Adapter)) < 0) {
                        printf("\n" PRINT_PAD "<<< Please enter a valid test mode!!! >>>\n\n");
                        break;
                }

                if( Adapter->wave_form_test_mode == (total_vector - 1))
                        bContinue = FALSE;
        } while(bContinue);

        enable_rtl_diag(Adapter->testnicinfo, 0);

        return ret;
}

int main(int argc, char *argv[])
{
        PMP_ADAPTER Adapter = NULL;
        int ret = -1;

        printf("**************************************************************************\n");
        printf("*             Realtek Linux WaveFormTool Version %-18s      *\n", PROGRAM_VERSION);
        printf("*   Copyright (C) 2014 Realtek Semiconductor Corp. All Rights Reserved.  *\n");
        printf("**************************************************************************\n");

        do {
                if (getuid() != 0) {
                        printf("\n" PRINT_PAD "<<< Please login with \"root\"!!! >>>\n\n");
                        return -ERIGHTS;
                }

                Adapter = (void*)malloc(sizeof(MP_ADAPTER));

                if (!Adapter) {
                        printf("\n" PRINT_PAD "<<< Allocate Resource Fail!!! >>>\n\n");
                        return -ECONFIG;
                }

                //Init MP_Adapter
                memset (Adapter,0,sizeof(MP_ADAPTER));

                if(argc>1) {
                        char *tmpStr = NULL;

                        strncpy(Adapter->devname, argv[1], IFNAMSIZ);
                        printf("\n" PRINT_PAD "<<< Interface Name: %s >>>\n\n", Adapter->devname);

                        tmpStr = argv[0];
                        if (strlen(argv[0]) > strlen(DEBUG_FILENAME))
                                tmpStr += (strlen(argv[0]) - strlen(DEBUG_FILENAME));

                        if (!strcmp(tmpStr, DEBUG_FILENAME)) {
                                printf("\n" PRINT_PAD "<<< Phy Para Debug Mode >>>\n\n");
                                Adapter->bShowDbgMsg = TRUE;
                        }
                } else {
                        printf("\n" PRINT_PAD "<<< Please assign a interface name!!! >>>\n\n");
                        return -ECONFIG;
                }

                /* find all the supported cards */
                if ((ret = get_adapter(Adapter)) < 0) {
                        printf("\n" PRINT_PAD "<<< could not find any supported cards!!! >>>\n\n");
                        return ret;
                }

                if ((ret = execute_lock()) < 0) {
                        printf("\n" PRINT_PAD "<<< Warn: unable to create execution lock!!! >>>\n\n");
                        //goto main_fail;
                }

                if (!Adapter->testnicinfo || collect_info(Adapter->testnicinfo) < 0) {
                        printf("\n" PRINT_PAD "<<< Get test card info fail!!! >>>\n\n");
                        goto main_fail;
                }

                print_info(Adapter);

                if ((ret = get_waveform_test_mode(Adapter)) < 0) {
                        printf("\n" PRINT_PAD "<<< Please enter a valid test mode!!! >>>\n\n");
                        goto main_fail;
                }

                if( Adapter->wave_form_test_mode < (total_vector - 1))
                        if ((ret = start_waveform_test(Adapter)) < 0)
                                goto main_fail;

                execute_unlock();

        } while(FALSE);

        if (Adapter != NULL)
                free(Adapter);

        return ret;

main_fail:

        printf("\n" PRINT_PAD "<<< error code = 0x%x >>>\n\n", ret);

        execute_unlock();

        if (Adapter != NULL)
                free(Adapter);

        return ret;
}
