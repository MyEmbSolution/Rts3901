#
# Realtek Semiconductor Corp.
#
# Tony Wu (tonywu@realtek.com)
# Jan. 10, 2011
#

all: samba

samba:
	make -C source

romfs:
	$(ROMFSINST) source/bin/smbd /bin
	$(ROMFSINST) source/bin/smbpasswd /bin
	$(ROMFSINST) source/bin/nmbd /bin
	$(ROMFSINST) .formosa/smb.conf /etc/samba/smb.conf
	$(ROMFSINST) .formosa/smbpasswd /etc/samba/smbpasswd

clean:
	make -C source clean
