# Fake, fully interacting ET Clients
  
_demo movie:_ [http://www.youtube.com/watch?v=SW3iPRBq0U0](http://www.youtube.com/watch?v=SW3iPRBq0U0)  
  
This project ranks among the funniest I ever engaged in in the ET world.  
  
From the programming point of view, because it combines many different fields; service (daemon) programming, client<->server model implementation, modularized programming, reverse engineering, integrating assembly, C, and C++, GUI design, oOo programming, plugins, and game hooking, and even learning to use 3rd party tools like making my first (and based on the experience, hopefully last) youtube movie.  
  
From the entertainment point of view because it's a potent and helluva funny tool to use.  
  
## Features  

-   fully interacting et clients
-   support for plugins to extend functionality
-   automated server downloads (www & direct)
-   sample plugins: aimbot, follow the leader, medicbot, spammer, jaymod client spoof
-   virtually all mods supported
-   total pk3 pure checksum spoof - no pk3's needed to connect to a server with the included pk3 checksum data (3GB worth of pk3s, you still need server specific pk3s like custom sounds etc)
-   gui controller interface, cli interface
-   minimal footprint - you can connect to a server using the 60kb cli client, using only 100kb of memory.
-   Runs from a Nokia N810 handheld PDA

## Limitations
  
Because the clients are minimal, there is no kind of pk3/level loading, so the clients have no idea of maps, walls, etc. This places a few limitations on the bots, you cannot use trace() functions. Although it's possible to create a service that loads the level and exposes trace functionality and interface the bots to it, this is beyond my original scope.  
The second limitation is the lack of PB support. This is a delicate subject; suffice it to say it's possible to link up PB to use with a single bot. However, because enemies of freedom are lurking everywhere, I feel it will not be to the best advantage to release this code right now.  
The third limitation is the lack of Etpro support. Because most etpro servers run PB anyway, it is useless to add etpro support. Again, if at a later time someone wants to add support, there only need to be written a plugin for it like the jaymod plugin.  
  
## About
  
As Luigi showed long ago it is an easy matter to connect a fake client to a Q3-based game. Except for proof of concept, or to DOS a server, it is not very useful. Constructing an army of fully interacting fake clients indistinguishable from the real thing is another matter entirely.  
  
The fake clients, called slaves, can connect to a server using command line interface, or trough the GUI master, which manages and allows intuitive control of many slaves connected to many different servers. To avoid confusion I label the components as follows; the ET server is simply called server, an entity that connects to the server is called a client, one of my fake clients connecting to a server is called a slave, and the object controlling many slaves is called a master. So, server <-> client/slave <-> master. The slaves can be controlled trough the master, or semi-independently trough external plugins.  
  
## Uses

-   DoS a server, the lame way (fill it up with slaves until no one can join anymore)
-   DoS a server, the lamer way (fill it halfway up and spam the hell out of it)
-   DoS a server, the fun way (fill it up with slaves fitted with miniature  
    aimbots that shoot everything that moves)
-   Follow The Leader (make the bots follow a client)
-   Bodyguards (make the bots follow you, with aimbots turned on to kick the hell  
    out of anyone who dares to oppose you)
-   Automated ServerFile downloader (Not included, but with the code in place you could make this easily enough. BIG FAT WARNING: even hinting of requesting me to make this for you will result in instant promotion to my perm-ignore list)
-   Offline' server-window (see whats going on without loading up ET)
-   ... much more, only limited by the imagination of the dev.

The project was developed on and designed for unix-like operating system. Except for the posix_spawn() command and unix domain sockets the code itself ought to be reasonably platform independent.  
  
ET is based on the q3 engine. Naturally the existence of the Q3 sdk was a big help in constructing the fake client, however, ET differs from Q3 in some vital areas, most notably the compressed message bitfields, pure checksumming, and download handling.
  
## How it works
  
This image schematically captures the flow between the components. Within a Slave, msg.c is responsible for low-level server communication, net.c handles the higher level cgame and ET emulation, while the controlling interface can be either command line interface or a daemon for use with the Master. The Master has a NetStuff and Server object for each server session. NetStuff handles slave <-> master communications, while Server holds server config and data. Each slave is managed by a ClientSlave object (not drawn), and are assigned to the appropriate NetStuff object. Each Slave is a physical new and independent process, not part of Master. A Slave is very defensive and  
cowardly, on the first sign of trouble it commits suicide and informs Master, which can then spawn a new one in its place. The Master and Slaves communicate over UDP sockets. This may all seem needlessly complex and contrived, but its actually very clean and simple programmatically because no complex special case handling & cleanup code is necessary. In short, a defective Slave won't bring down the whole Castle.  
  
The very first Slave connected to a server is automatically promoted to Scout. After it gives the oh-kay signal, other Slaves are allowed to join. A Scout is a Slave which relays all server data back to Master. This is done for efficiency reasons, because all Slaves tend to receive more or less the same data from server. When Scout dies, NetStuff automatically assigns another.  
  
## follow bot - how it works  
  
The easiest way to make follow work is to write a small hook for ET that captures all the usercmd's generated and sends them directly to the master. The master then sends it to all connected/configured slaves which again send it to the server and hence make an exact copy of the movements. The only thing that remains is adjust the angles so the bots always 'point' at the target.  
  
## pk3-less ET clients on pure servers  

Pure servers is an inbuilt protection scheme to assure the validity (or purity) of connecting clients so that the clients are seeing what the server sees. The scheme works by loading only those pk3's which are 'referenced' by the server. To verify the integrity of the PK3 files, a checksum is generated and send to the server. If this is equal to the checksum the server calculated, the client is allowed to connect, else it gets dropped with the message "Unpure pk3 files referenced"  
  
To generate a checksum, a message is needed to hash. To hash the entire pk3 contents would be hopelessly slow. Instead, each pk3 (or, zip) file contains headers which contain the crc32 (an 32bit number) of the unpacked file. The message on which the checksum is based is simply an array of all these crc32 numbers.  
  
However to circumvent the pk3 checksums, one could simply spoof them by sending the computed checksum! Id Software was not that thick. Not only do you need the pk3 "message" itself, but also a server supplied random number to generate the pure checksum - the random number changes on each server restart (map change).  
  
The exact nature of ET's pk3 checksum algorithm I do not know, and it is quite different from the one in the Q3 sdk, but it appears to be some variation of md4 or md5. I did not bother to find out, however. I located the place of the subroutines used to calculate the checksums, and wrote a small utility (its under /utils) to convert the disassembly of these functions into usable asm code to be included directly into etdos fake clients. For more info, look  
  
## Dir List  

-   /etdos - core fake client program
-   /cli - cli interface
-   /daemon - daemon interface
-   /gui - GUI controller ('Master')
-   /utils - all small helper programs I wrote to aid in the construction of the program
-   /data - name list, cfg files, pk3 list
-   /ethook - hooks et.x86 (2.60/2.55) to relay ucmds

## How to compile    
You need a unix-like OS, gcc, g++, yasm and Qt4. Go to the gui dir and do _'qmake'_ then in the root dir simply _'make'_ .  
  
Main view, the connected slaves list in the center, a server client view in the top center, the server chat window in the top left corner, and the spycam showing the relative positions of the connected clients present in the snapshot  
  
Also included a small ET 2.60 etmain demo showing the various plugins in action, namely, medicbot, followbot, and aimbot.  
  
## credits:  
Special thanks to Blof & Leaker of nixcoders for the compilation of the huge pk3 list  
  
The speedy completion wouldn't have been possible if not for the open source nature and excellent documentation of the tools I used. I therefore dedicate this work to all people who are committed to the free flow of Knowledge and ideas.  
  
May The Force be with you!