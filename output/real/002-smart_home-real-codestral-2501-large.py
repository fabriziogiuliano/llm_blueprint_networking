
import json
import os
from fabric import Connection

def configure_hostapd(ap_config):
    hostapd_conf = f"""
interface=wlan0
driver=nl80211
ssid={ap_config['ssid']}
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase={ap_config['wpa_passphrase']}
wpa_key_mgmt={ap_config['wpa_key_mgmt']}
rsn_pairwise=CCMP
"""
    with open('/etc/hostapd/hostapd.conf', 'w') as f:
        f.write(hostapd_conf)

def configure_dnsmasq(stations):
    dnsmasq_conf = """
interface=wlan0
dhcp-range=192.168.2.50,192.168.2.150,12h
"""
    for station in stations:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"
    with open('/etc/dnsmasq.conf', 'w') as f:
        f.write(dnsmasq_conf)

def setup_wifi_ap(ap_config, stations):
    configure_hostapd(ap_config)
    configure_dnsmasq(stations)

    # Assuming the AP is on a remote machine
    conn = Connection(host=ap_config['eth_IP'], user='pi', connect_kwargs={'password': 'raspberry'})
    conn.run('sudo systemctl restart hostapd')
    conn.run('sudo systemctl restart dnsmasq')

def main():
    with open('TEST_BLUEPRINT.json', 'r') as f:
        blueprint = json.load(f)

    wifi_aps = blueprint['network_devices']['wifi_access_points']
    wifi_stations = blueprint['network_devices']['wifi_stations']

    for ap in wifi_aps:
        setup_wifi_ap(ap, wifi_stations)

if __name__ == "__main__":
    main()
