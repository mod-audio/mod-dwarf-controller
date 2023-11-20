#!/bin/bash

# make clean
# make moddwarf

sshpass -p "mod" scp -O ./out/mod-dwarf-controller.bin root@192.168.51.1:/tmp/
sshpass -p 'mod' ssh root@192.168.51.1 hmi-update /tmp/mod-dwarf-controller.bin
sshpass -p 'mod' ssh root@192.168.51.1 systemctl restart mod-ui

# make clean
