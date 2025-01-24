#rm -rf /lib/firmware/nrf/wifi/*
#Working Firmware
#cp /root/ajay/Working_PCIE/nrf_wifi_*.bimg /lib/firmware/nrf/wifi/.

#service kdump-tools status
#systemctl enable kdump-tools

#/etc/init.d/kdump-tools start
#/etc/init.d/kexec start
#kdump-config show

#Not Working Firmware

#make clean all PLATFORM=WEZEN FUNC=WLAN LOW_POWER=0 HOST_CFG80211=Y INLINE_MODE=N RF=C0 CONFIG=72 FW_LOAD=NONE BUS_IF=PCIE SECURE_DOMAIN=Y

#make clean all PLATFORM=WEZEN FUNC=WLAN LOW_POWER=0 HOST_CFG80211=Y INLINE_MODE=N RF=C0 CONFIG=72 FW_LOAD=NONE BUS_IF=PCIE SECURE_DOMAIN=Y FW_LOAD_SUPPORT=Y HOST_FW_HEX_LOAD_SUPPORT=Y RPU_HARD_RESET_SUPPORT=Y
make clean all PLATFORM=WEZEN FUNC=WLAN LOW_POWER=0 HOST_CFG80211=Y INLINE_MODE=N RF=C0 CONFIG=72 FW_LOAD=NONE BUS_IF=PCIE SECURE_DOMAIN=Y INLINE_MODE_RX=y

lsmod | grep nrf_wifi_fmac_reg
/etc/init.d/network-manager stop
killall -9 wpa_supplicant
killall -9 wpa_supplicant
pkill -9 wpa_cli
rfkill unblock all
modprobe cfg80211
insmod nrf_wifi_fmac_reg.ko
sleep 2
ifconfig nrf_wifi up 192.168.1.77
sleep 2 
iw nrf_wifi set power_save off
ip link show
sleep 2
iw dev nrf_wifi info
#sleep 2
#sudo ifconfig nrf_wifi up
#sudo iw dev nrf_wifi scan | grep HOST
#sudo iw dev nrf_wifi scan | grep UMAC

