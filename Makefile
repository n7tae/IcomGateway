# Copyright (c) 2018-2021,2025 by Thomas A. Early N7TAE
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# locations for the executibles and other files are set here
# NOTE: IF YOU CHANGE THESE, YOU WILL NEED TO UPDATE THE service.* FILES AND
# if you change these locations, make sure the sgs.service file is updated!
# you will also break hard coded paths in the dashboard file, index.php.

BINDIR=/usr/local/bin
CFGDIR=/usr/local/etc
WWWDIR=/usr/local/www
SYSDIR=/etc/systemd/system
IRC=ircddb

# use this if you want debugging help in the case of a crash
#CPPFLAGS=-ggdb -W -std=c++11 -Iircddb -DCFG_DIR=\"$(CFGDIR)\" -DBIN_DIR=\"$(BINDIR)\"

# or, you can choose this for a much smaller executable without debugging help
CPPFLAGS=-W -std=c++11 -Iircddb -DCFG_DIR=\"$(CFGDIR)\" -DBIN_DIR=\"$(BINDIR)\"

LDFLAGS=-L/usr/lib -lrt

IRCOBJS = $(IRC)/IRCDDB.o $(IRC)/IRCClient.o $(IRC)/IRCReceiver.o $(IRC)/IRCMessageQueue.o $(IRC)/IRCProtocol.o $(IRC)/IRCMessage.o $(IRC)/IRCDDBApp.o $(IRC)/IRCutils.o
SRCS = $(wildcard *.cpp) $(wildcard $(IRC)/*.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

ALL_PROGRAMS = qngateway qnlink qnistack qnremote qnvoice

all    : $(ALL_PROGRAMS)

qngateway : QnetGateway.o Base.o aprs.o UnixDgramSocket.o TCPReaderWriterClient.o QnetConfigure.o QnetDB.o CacheManager.o DStarDecode.o Location.o $(IRCOBJS)
	g++ -o $@ $^ $(LDFLAGS) -l sqlite3 -pthread

qnlink : QnetLink.o Base.o DPlusAuthenticator.o TCPReaderWriterClient.o UnixDgramSocket.o UDPSocket.o QnetConfigure.o QnetDB.o
	g++ -o $@ $^ $(LDFLAGS) -l sqlite3 -pthread

qnistack : QnetIcomStack.o Base.o UnixDgramSocket.o QnetConfigure.o
	g++ -o $@ $^ $(LDFLAGS)

qnremote : QnetRemote.o UnixDgramSocket.o QnetConfigure.o
	g++ -o $@ $^ $(LDFLAGS)

qnvoice : QnetVoice.o QnetConfigure.o
	g++ -o $@ $^ $(LDFLAGS)

%.o : %.cpp
	g++ $(CPPFLAGS) -MMD -MD -c $< -o $@

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) $(ALL_PROGRAMS) *.gch

-include $(DEPS)

install : $(ALL_PROGRAMS) gwys.txt qn.cfg
	######### QnetGateway #########
	/bin/cp -f qngateway $(BINDIR)
	/bin/cp -f qnremote qnvoice $(BINDIR)
	/bin/ln -f -s $(shell pwd)/qn.cfg $(CFGDIR)
	/bin/ln -f -s $(shell pwd)/index.php $(WWWDIR)
	/bin/ln -f -s $(shell pwd)/dashboardV2 $(WWWDIR)
	/bin/cp -f defaults $(CFGDIR)
	/bin/cp -f system/qngateway.service $(SYSDIR)
	systemctl enable qngateway.service
	systemctl daemon-reload
	systemctl start qngateway.service
	######### QnetLink #########
	/bin/cp -f qnlink $(BINDIR)
	/bin/cp -f announce/*.dat $(CFGDIR)
	/bin/ln -f -s $(shell pwd)/gwys.txt $(CFGDIR)
	/bin/cp -f exec_?.sh $(BINDIR)
	/bin/cp -f system/qnlink.service $(SYSDIR)
	systemctl enable qnlink.service
	systemctl daemon-reload
	systemctl start qnlink.service
	######### QnetIcomStack #########
	/bin/ln -f qnistack $(BINDIR)/qnistack
	sed -e "s/XXX/qnistack/" system/qnistack.service > $(SYSDIR)/qnistack.service
	systemctl enable qnistack.service
	systemctl daemon-reload
	systemctl start qnistack.service

installdtmf : qndtmf
	/bin/ln -f -s $(shell pwd)/qndtmf $(BINDIR)
	/bin/cp -f system/qndtmf.service $(SYSDIR)
	systemctl enable qndtmf.service
	systemctl daemon-reload
	systemctl start qndtmf.service

installdash : index.php
	/usr/bin/apt update
	/usr/bin/apt install -y php-common php-fpm sqlite3 php-sqlite3 dnsutils
	mkdir -p $(WWWDIR)
	mkdir -p dashboardV2/jsonData
	/bin/ln -f -s $(shell pwd)/index.php $(WWWDIR)
	/bin/ln -f -s $(shell pwd)/dashboardV2 $(WWWDIR)
	if [ ! -e system/qndash.service ]; then cp system/qndash.service.80 system/qndash.service; fi
	/bin/cp -f system/qndash.service $(SYSDIR)
	systemctl enable qndash.service
	systemctl daemon-reload
	systemctl start qndash.service

uninstall :
	######### QnetGateway #########
	systemctl stop qngateway.service
	systemctl disable qngateway.service
	/bin/rm -f $(SYSDIR)/qngateway.service
	/bin/rm -f $(BINDIR)/qngateway
	/bin/rm -f $(BINDIR)/qnremote
	/bin/rm -f $(BINDIR)/qnvoice
	/bin/rm -f $(CFGDIR)/qn.cfg
	/bin/rm -f $(CFGDIR)/defaults
	######### QnetLink #########
	systemctl stop qnlink.service
	systemctl disable qnlink.service
	/bin/rm -f $(SYSDIR)/qnlink.service
	/bin/rm -f $(BINDIR)/qnlink
	/bin/rm -f $(CFGDIR)/*.dat
	/bin/rm -f $(CFGDIR)/qn.db
	/bin/rm -f $(CFGDIR)/gwys.txt
	/bin/rm -f $(BINDIR)/exec_?.sh
	######### QnetIcomStack #########
	systemctl stop qnistack.service
	systemctl disable qnistack.service
	/bin/rm -f $(SYSDIR)/qnistack.service
	/bin/rm -f $(BINDIR)/qnistack
	systemctl daemon-reload

uninstalldtmf :
	systemctl stop qndtmf.service
	systemctl disable qndtmf.service
	/bin/rm -f $(SYSDIR)/qndtmf.service
	systemctl daemon-reload
	/bin/rm -f $(BINDIR)/qndtmf

uninstalldash :
	systemctl stop qndash.service
	systemctl disable qndash.service
	/bin/rm -f $(SYSDIR)/qndash.service
	systemctl daemon-reload
	/bin/rm -f $(WWWDIR)/index.php
	/bin/rm -f $(WWWDIR)/dashboardV2
	/bin/rm -f $(CFGDIR)/qn.db
	/bin/rm -rf dashboardV2/jsonData
