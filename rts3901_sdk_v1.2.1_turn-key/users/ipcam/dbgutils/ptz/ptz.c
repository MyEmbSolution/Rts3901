#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <termios.h>
#include  <errno.h>
#define TRUE  0
#define FALSE  -1

enum  Direct {
	STOP     =  0x00,
	RIGHT    =   0x02,
	LEFT     =   0x04,
	UP       =   0x08,
	DOWN     =   0x10
};

int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int  name_arr[] = { 38400,   19200,   9600,   4800,   2400,   1200,   300,
38400,   19200,   9600,  4800,  2400,  1200,   300, } ;

/** *@brief set uart port baud rate
* @param   fd   uart file handle
* @param   speed  baud rate
* @return   void
*/
void SetSpeed(int fd, int speed)
{
	int i;
	int status;
	struct termios Opt;
	tcgetattr(fd,  &Opt);
	for (i = 0; i < sizeof(speed_arr)/sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status  = tcsetattr(fd, TCSANOW, &Opt);
			if   (status != 0)
				printf("set_speed [Failue]\n ");
			return;
		}
		tcflush(fd, TCIOFLUSH);
	}
}

/**
*@brief
* @param    fd
* @param    databits
* @param    stopbits
* @param    parity
 */
int  SetParity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	if (tcgetattr(fd, &options) != 0) {
		printf(" set_Parity [Failue]\n");
		return FALSE;
	}
	options.c_cflag &= ~CSIZE;
	switch  (databits) {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag  |= CS8;
		break;
	default:
		fprintf(stderr, " Unsupported data size\n ");
		return FALSE;
	}

	switch  (parity) {
	case   'n':
	case   'N':
		options.c_cflag  &= ~PARENB;     /*  Clear parity enable  */
		options.c_iflag  &=  ~INPCK;      /*  Enable parity checking  */
		break;
	case   'o':
	case   'O':
		options.c_cflag  |=  (PARODD  |  PARENB);
		options.c_iflag  |=  INPCK;               /*  Disnable parity checking  */
		break;
	case   'e':
	case   'E':
		options.c_cflag  |= PARENB;       /*  Enable parity  */
		options.c_cflag  &= ~PARODD;
		options.c_iflag  |= INPCK;        /*  Disnable parity checking  */
		break;
	case   'S':
	case   's':   /* as no parity */
		options.c_cflag  &= ~PARENB;
		options.c_cflag  &= ~CSTOPB;
		break;
	default:
		fprintf(stderr, " Unsupported parity\n ");
	return  FALSE;
	}

	switch  (stopbits) {
	case   1:
		options.c_cflag &= ~CSTOPB;
		break;
	case   2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr, " Unsupported stop bits\n ");
	return  FALSE;
	}
	options.c_cflag |= CRTSCTS;
	/*  Set input parity option  */
	if  (parity != 'n')
		options.c_iflag  |= INPCK;
	options.c_cc[VTIME] = 150;  /*  15 seconds*/
	options.c_cc[VMIN] = 0;

	tcflush(fd, TCIFLUSH);  /*  Update the options and do it NOW  */
	if  (tcsetattr(fd, TCSANOW, &options) != 0) {
		perror(" SetupSerial 3 ");
		return FALSE;
	}
	return TRUE;
}

 /**
*@breif open uart
 */
int  OpenDev(char *Dev)
{
	int  fd = open(Dev, O_RDWR);          /* | O_NOCTTY | O_NDELAY*/

	if  (-1 == fd) {
		return -1;
	} else {
		return  fd;
	}
}

inline int CheckSum(char *strCommOut)
{
	int  i, sum = 0;
	for (i = 1 ; i < 6 ; i++) {
		sum += strCommOut[i];
	}
	if (sum > 0xFF)
		strCommOut[6] = sum % 0x100;
	else
		strCommOut[6] = sum;
}

char *MakeDirect(char *strCommOut, char chDirrect, char chCammerID)
{
	strCommOut[0]  = 0xFF;
	strCommOut[1]  = chCammerID;
	strCommOut[2]  = 0x0;
	strCommOut[3]  = chDirrect;
	strCommOut[4]  = 0x20;
	strCommOut[5]  = 0x20 ;
	CheckSum(strCommOut);
	return  strCommOut;
}

void Running(int fd, char chCammerID, char chDir)
{
	char strCommOut[7];
	MakeDirect(strCommOut, chDir, chCammerID);
	write(fd, strCommOut, sizeof(strCommOut));
}

int  main()
{
	char *dev2  = "/dev/ttyS0";
	int  fd = OpenDev(dev2);
	if  (fd > 0)
		SetSpeed(fd, 2400); /* set baud rate*/
	else {
		printf(" Can't Open Serial Port '%s'\n ", dev2);
		return -1;
	}

	if  (SetParity(fd, 8, 1, 'N') == -1) {
		printf(" Set Parity Error\n ");
		return -1;
	}

	Running(fd, 1, LEFT); /*address 1 , turn left*/
	printf(" press any key to stop.\n ");
	getchar();
	Running(fd, 1, STOP); /*address 1 , stop*/

	return 0;
}
