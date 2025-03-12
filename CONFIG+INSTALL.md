# IcomGateway Configuration and Installation

## Preparation

You will need several packages to build the IcomGateway gateway. You will probably already have all or most of these but it still doesn't hurt to be sure:

```
sudo apt update
sudo apt upgrade
sudo apt install make g++ unzip git wget curl
sudo apt install libsqlite3-dev
```
You'll need to copy the repo and then move to the build directory:

```
git clone git://github.com/n7tae/IcomGateway.git
cd IcomGateway
```
## Configuration

Create your configuration file, qn.cfg:
```
./config
```
This will start a menu driven script. In this mode, only the most common, useful parameters are visible. Some parameters are hidden because they very seldom need to be altered. However, if you want access to absolutely every parameter use:
```
./config expert
```
Please be sure you know what you are doing if you change these normally hidden parameters. Most parameters have built-in default values, and will be labeled as `<DEFAULT>` if you have not overridden that value.

There are a few parameters that have to be set and so they do not have a default value. If you need to override the default value, the general method is to type the key for that value followed by the new value. If you want to delete an overridden value, type a "u" followed by the key you want to unset. Boolean values (true or false) can be toggled by just entering the key.

The `config` main menu is just made up of links to sub-menus.

### 1. The IRCDDB Menu

You have to specify your irc login. This is usually the callsign of the repeater. By default, IcomGateway will only connect to the IPv4 QuadNet server at *ircv4.openquad.net*. IcomGateway is capabile of dual-stack operation and can also connect to the IPv6 server at *ircv6.openquad.net*. If you want to operate in dual stack mode, enter the IRC sub-menuvand set `ha` to "ircv6.openquad.net" and `hb` to "ircv4.openquad.net". Once your operating in dual-stack mode, any routing will prefer an IPv6 address, if available. It's that easy.

### 2. The Gateway/APRS Menu

Most of the values in this sub-menu have to do with registering you reflector on APRS and the data that is displayed about your repeater. You don't have to enable APRS and if you don't most of these values can be left at their default values.

The gateway is used to make routing connections, like subscribing to a Smart Group or a StarNet Group. These groups broadcast their subscription callsigns once and hour and once your irc channel(s) receive this information your gateway will know how to route to those groups. If you try to route to a group before your gateway has received routing information for that group, you'll hear a message to try again. In that time your irc client will ask the irc server about the routing information for your target, and by the time you hear the message to try again, you should be able to successfully complete the route request. If you want to speed this up, you can specify route that you want to load up right away with the `fr` key. If you specify this, you'll see log entries for each route you want to preload.

### 3. The Link/D-Plus Menu

In this sub-menu you will set up which callsign are allowed to perform linking function and other control functions. Please note that you can define either a list of callsigns that can perform link functions or a list of callsign that cannot. You should not specify both lists.

Most of the other items in this sub-menu have to d with the closed-source, legacy D-Plus reflectors and repeaters. If you plan on connecting to these systems you need to make sure you are authorized to use them. They require that you are a registered user, see www.dstargateway.org for more information. If you are a registered user, you can enable IcomGateway to used this closed-source system by DPlus. By default, when QnetLink registers your callsign you will download both repeaters and reflectors. You can control this with configuration parameters. You can also control the priority of D-Plus reflectors and repeaters. By default, QnetLink first loads the gwys.txt file first and then the systems delivered by the D-Plus Authorization system. You can change the order, if you want definitions in your gwys.txt file to override any delivered by the D-Plus authorization server.

The information downloaded from the DPlus server is dynamic and will change from hour to hour. You can update QnetLink by sending `"       F"` to your system. This will purge the current table and re-authorize with the DPlus server and then reload gwys.txt.

Because of the way DPlus authorization works, QnetLink can't actually confirm that your authorization was successful. If your system is unlinked after trying to transmit into a DPlus system, it means that your authorization was unsuccessful. This might indicate that there may be a problem with your DPlus registration.

### 4. Logging Menu

If you want to see what's going on inside the programs, there are logging options available that will cause a lot more log entries. These extra entries can be useful if you are trying to track down problems with your system. The `Call(QSO)` menu is probably the most useful item to enable. It will show voice streams opening a closing on each configured module.

### 5. The Icom Menu

Most importantly in this sub-menu, you will specify if your Icom stack is using the RP2C controller. Once you do that, the main menu will allow you to access to the menus that configure each module A, B and/or C.

Make sure the local and stack IP addresses are correct. A local address of 0.0.0.0 mean that your system will bind to networks and accept UDP port 20000 traffic from any network. If you need to, you can change this to the specific address at which the computer that IcomGateway is running. A stack address of 172.16.0.1 is the default Icom address. If you system is different, you may need to change this. This is also where you specify if you are using the Icom RP2C controller. In the expert menu, you can also change the port number, but this is NOT recommended. When you exit the Icom menu, you will be able to configure each RF module.

Most importantly in this sub-menu, you will specify if your Icom stack is using the RP2C controller. Once you do that, the main menu will allow you to access to the menus that configure each module A, B and/or C.

### Specifying the RF Modules, A, B and/or C

By convention, specify a 23cm module on A, a 70cm module on B and a 2M module on C. IcomGateway supports a maximum of three modules. The 23cm data module is not currently supported.

If a module had been created that you don't need, enter the sub-menu for that module and the do `xx`. That will delete the module and return you to the main module.

Please note that when you select modules, you can specify your TX/RX frequencies but that does not actually set the module frequencies. You must set frequencies using your Icom based software. Specifying frequencies here will ensure that your repeater shows up in both the openquad.net gateways list and APRS with the correct information.

You will also see three integer settings, Type, Make and Module. These are parameters the Icom stack uses to control routing. Since you have already configure whether or not you have the RP2C controller, these Type/Mark/Module values should be correctly assigned.

### Finishing Up the Confguration

After you are happy with your configuration, be sure to write it out with the `w` key in the main menu. It will show you your qn.cfg file after it writes it out. After you install and try out your system, you may find that you need to change some configuration values. In that case just start the configure script again. It will read the current qn.cfg file when it start and initialize the menus accordingly.

## Administration

Once you have your qn.cfg file, your ready to compile and install your system, type:
```
./admin
```
The first thing you want to do is to create your gwys.txt file. use the 'gw' key to get into that sub-menu. There are several choices to initialize your gwys.txt file. Choose one and then you can edit the file to your satisfaction.

If you create a My_Hosts.txt file, it will automatically be appended to the end of your gwys.txt file. Because it is at the end, definitions in My_Hosts.txt will override anything defined before. The format for this file is just like gwys.txt:
```
# comments can begin with a hash mark
# and then: gateway ip_address port
# choose port for linking family (must be supported by the gateway):
#   20001 for DPlus 30001 for DExtra 30051 for DCS.
# Here is a bogus definition, with the proper format:
Q0XYZ 44.44.44.44 20001
```
Now, you can compile and install your system with the 'is' key. admin will use your qn.cfg file to figure out what needs to be compiled and how it is to be installed.

If you plan on using DTMF, use the 'id' key to install it. Once you install something, the admin will dynamically change and show you how to uninstall the installed items.

The maintenance sub-menu accessed with the 'm' key will let you stop and start different programs in your installed package. Note that this just uses systemctl to start and stop a service. It will not uninstall the service. You might want to do this if you have changed your configuration file.

The log sub-menu accessed with the 'l' key can be use to put a "tail" on different log files and you can watch a log file in real-time.

##

Tom Early, n7tae (at) arrl (dot) net
