#rm -rf /lib/firmware/nrf/wifi/*
#Working Firmware
#cp /root/ajay/Working_PCIE/nrf_wifi_*.bimg /lib/firmware/nrf/wifi/.

#service kdump-tools status
#systemctl enable kdump-tools

#/etc/init.d/kdump-tools start
#/etc/init.d/kexec start
#kdump-config show

#Not Working Firmware
cp /root/ajay/TEST/nrf_wifi_*.bimg /lib/firmware/nrf/wifi/.

#make clean all MODE=REG BUS_IF=PCIE FW_LOAD=RAM LOW_POWER=0 INLINE_MODE=N FUNC=WLAN HOST_CFG80211=Y
rmmod nrf_wifi_fmac_reg.ko
lsmod | grep nrf_wifi_fmac_reg
/etc/init.d/network-manager stop
killall -9 wpa_supplicant
pkill -9 wpa_cli
rfkill unblock all
modprobe cfg80211
insmod nrf_wifi_fmac_reg.ko base_mac_addr=001122334455
ip link show
sleep 2
iw dev nrf_wifi info
#sleep 2
sudo ifconfig nrf_wifi up
#sudo iw dev nrf_wifi scan | grep HOST
sudo iw dev nrf_wifi scan | grep UMAC

