#!/usr/bin/bash

RED='\033[0;31m'
LIGHT_GREEN='\033[1;32m'
NC='\033[0m'
DHCPCD_CONFIG_PATH="/etc/dhcpcd.conf"

helper_function() {
    echo "Usage: network_config.sh <INTERFACE> <STATIC_IP> <ROUTER_IP> - [set static ip to defined interface]"
    echo "Usage: network_config.sh -i - [show available interfaces]"
}

show_ifaces() {
    ip -o link show | awk -F': ' '{print $2}'
}

check_ip() {
    if ping -c1 -w3 $1 >/dev/null 2>&1
    then
        return 1
    else
        return 0
    fi
}

check_iface() {
    show_ifaces | grep $1 1> /dev/null 2> /dev/null;
    if [ $? -eq 0 ]; then
        return 1
    else
        return 0
    fi
}

set_static_ip() {
echo "interface $1
static ip_address=$2/24
static routers=$3
static domain_name_servers=$3 8.8.8.8" 1> /dev/null 2> /dev/null; #>> $DHCPCD_CONFIG_PATH
}

main() {
    if [ "$#" -lt 1 ]; then
        echo "Helper:"
        helper_function
    fi

    if [ "$#" -eq 1 ]; then
        show_ifaces
    fi

    if [ "$#" -eq 3 ]; then
        check_iface $1
        IFACE_CHECK=$?
        check_ip $2;
        STATIC_IP_CHECK=$?
        check_ip $3;
        ROUTER_IP_CHECK=$?

        if [ $IFACE_CHECK -eq 1 ]; then
            if [ $STATIC_IP_CHECK -eq 0 ] && [ $ROUTER_IP_CHECK -eq 1 ]; then
                echo "Setting up static ip for interface..."
                set_static_ip $1 $2 $3
                echo -e "${LIGHT_GREEN}Setting up finished!${NC}"
                echo -e '\033[1mPlease reboot system!\033[0m'
            else
                echo -e "IP address ${RED}'$2'${NC} is bussy, please choose another ip for static adress"
            fi
        else
            echo -e "Interface ${RED}'$1'${NC} not found"
        fi
    fi
}

main $1 $2 $3