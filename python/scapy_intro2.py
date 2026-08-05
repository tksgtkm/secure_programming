# sudo -E /home/onimas/.pyenv/shims/python scapy_intro2.py

from scapy.layers.inet import IP, TCP, ICMP, UDP
from scapy.layers.l2 import Ether
from scapy.layers.dns import DNS
from scapy.all import sr1, srp

packet = IP() / TCP()
print(Ether() / packet)

p = Ether() / IP(dst="www.secdev.org") / TCP(flags="F")
print(p.summary())

print(p.dst)
print(p[IP].src)
print(p.sprintf("%Ether.src% > %Ether.dst%\n%IP.src% > %IP.dst%"))

print([p for p in IP(ttl=(1, 5)) / ICMP()])

print([p for p in IP() / TCP(dport=[22, 80, 443])])

p = sr1(IP(dst="8.8.8.8") / UDP() / DNS())
print(p[DNS].an)

r, u = srp(Ether() / IP(dst="8.8.8.8", ttl=(5, 10)) / UDP() / DNS())
print(r, u)