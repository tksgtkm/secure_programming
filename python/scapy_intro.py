from scapy.layers.inet import IP, UDP
from scapy.layers.l2 import Ether

target = "www.target.com/30"
ip = IP(dst=target)
print(ip)

print([p for p in ip])

print(Ether(dst="ff:ff:ff:ff:ff:ff") / IP(dst=["ketchup.com", "mayo.com"], ttl=(1, 9)) / UDP())