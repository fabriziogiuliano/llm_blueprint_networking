python
import os
from fabric import Connection

# Configuration for WiFi Access Point
hostapd_conf = """
interface=wlan0
driver=nl80211
ssid=SmartHomeAPMyExperiment
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=12345678
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
"""

dnsmasq_conf = """
interface=wlan0
dhcp-range=192.168.1.100,192.168.1.200,12h
"""

# Function to configure WiFi AP
def configure_wifi_ap(host, user, password):
    conn = Connection(host=host, user=user, connect_kwargs={"password": password})
    conn.put(io.StringIO(hostapd_conf), '/etc/hostapd/hostapd.conf')
    conn.put(io.StringIO(dnsmasq_conf), '/etc/dnsmasq.conf')
    conn.sudo('systemctl restart hostapd')
    conn.sudo('systemctl restart dnsmasq')

# Function to configure WiFi Stations
def configure_wifi_station(host, user, password, ssid, wpa_passphrase):
    conn = Connection(host=host, user=user, connect_kwargs={"password": password})
    wpa_supplicant_conf = f"""
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=US

network={{
    ssid="{ssid}"
    psk="{wpa_passphrase}"
    key_mgmt=WPA-PSK
}}
"""
    conn.put(io.StringIO(wpa_supplicant_conf), '/etc/wpa_supplicant/wpa_supplicant.conf')
    conn.sudo('wpa_cli -i wlan0 reconfigure')

# Configure WiFi AP
configure_wifi_ap(host='192.168.1.1', user='root', password='123456')

# Configure WiFi Stations
configure_wifi_station(host='192.168.1.103', user='root', password='123456', ssid='SmartHomeAPMyExperiment', wpa_passphrase='12345678')
configure_wifi_station(host='192.168.1.102', user='root', password='123456', ssid='SmartHomeAPMyExperiment', wpa_passphrase='12345678')
configure_wifi_station(host='192.168.1.101', user='root', password='123456', ssid='SmartHomeAPMyExperiment', wpa_passphrase='12345678')
configure_wifi_station(host='192.168.1.100', user='root', password='123456', ssid='SmartHomeAPMyExperiment', wpa_passphrase='12345678')
