#!/bin/bash
# Setup script for virtual CAN interfaces
set -e

create_vcan() {
    local iface=$1
    if ip link show "$iface" > /dev/null 2>&1; then
        echo "Interface $iface already exists."
        sudo ip link set up "$iface"
    else
        echo "Creating virtual CAN interface $iface..."
        sudo ip link add dev "$iface" type vcan
        sudo ip link set up "$iface"
    fi
}

echo "Loading vcan kernel module..."
sudo modprobe vcan 2>/dev/null || true

create_vcan vcan0
create_vcan vcan1

echo "Virtual CAN interfaces are active:"
ip -br link show type vcan
