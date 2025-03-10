IcomGateway
===========

The IcomGateway is an D-Star IRCDDB gateway application that supports some Icom repeater hardware. IcomGateway support the latest generation of hardware, with or without a separate Icom controller. IcomGateway does not support the 23cm data-only module.

IcomGateway is dual-stack capable. This means it can simultaneously connect to ircv4.openquad.net, which is IPv4 based (using 32-bit internet addresses) and to ircv6.openquad.net which is IPv6 based (using 128-bit internet address). If your hot-spot/reapeater has IPv6 access you can enable dual-stack operation (it's IPv4-only by default) and then take advantage of direct world-routable address. The potential benefit of IPv6 to routing is significant.

To get started, clone this software to your Linux device:

```bash
git clone https://github.com/n7tae/IcomGateway.git
```

Then look to the CONFIG+INSTALL file for more information. There are several package requirements, including SQLite that you will need.

IcomGateway includes a "remote control" program, called `qnremote`. After you build and install the system, type `qnremote` for a prompt on how to use it. Using this and cron, it's possible to setup schedules where you system will automatically link up to a reflector, or subscribe to a Routing Group. For More information, see DTMF+REMOTE.README.

For other details of interesting things QnetGatway can do, see the OPERATING file. For example, with IcomGateway, you can execute up to 36 different Linux scripts from you radio. Three scripts are include:

```text
YourCall = "      HX"   will halt (shut down) your system.
YourCall = "      RX"   will reboot your system.
YourCall - "      GX"   will restart IcomGateway
```

IcomGateway is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation. IcomGateway is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the LICENSE file for more details.

73

Tom

N7TAE (at) arrl (dot) net
