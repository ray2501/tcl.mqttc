
mqttc
=====

[MQTT](http://mqtt.org/) is a light-weight publish/subscribe messaging
protocol, originally created by IBM and Arcom (later to become part of
Eurotech) around 1998.

This Tcl extension is based on [Paho MQTT C Client](https://www.eclipse.org/paho/clients/c/),
but only support synchronous API and SSL/TLS support.

I write this extension to research MQTT client.

This extension requires OpenSSL development files.


License
=====

Paho MQTT C Client is licensed under Eclipse Public License (EPL) or
Eclipse Distribution License (EDL).

This extension is licensed under BSD 3-Clause License


UNIX BUILD
=====

Building under most UNIX systems is easy, just run the configure script and
then run make. For more information about the build process, see the
tcl/unix/README file in the Tcl src dist. The following minimal example will
install the extension in the /opt/tcl directory.

    $ cd tcl.mqttc
    $ ./configure --prefix=/opt/tcl
    $ make
    $ make install

If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

    $ cd tcl.mqttc
    $ ./configure --with-tcl=/opt/activetcl/lib
    $ make
    $ make install


WINDOWS BUILD
=====

The recommended method to build extensions under windows is to use the
Msys + Mingw build process. This provides a Unix-style build while generating
native Windows binaries. Using the Msys + Mingw build tools means that you
can use the same configure script as per the Unix build to create a Makefile.

I download [OpenSSL Windows installer](https://slproweb.com/products/Win32OpenSSL.html)
to install (v1.1.0).

If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

    $ cd tcl.mqttc
    $ ./configure --with-tcl=/c/tcl/lib
    $ make
    $ make install


Commands
=====

mqttc HANDLE serverURI clientId persistence_type ?-timeout timeout? ?-keepalive keepalive? ?-cleansession cleansession? ?-username username? ?-password password? ?-sslenable boolean? ?-trustStore truststore? ?-keyStore keystore? ?-privateKey privatekey? ?-privateKeyPassword password? ?-enableServerCertAuth boolean?  
HANDLE isConnected  
HANDLE publishMessage topic payload QoS retained  
HANDLE subscribe topic QoS   
HANDLE unsubscribe topic  
HANDLE receive  
HANDLE close  

The interface to the Paho MQTT C Client library consists of single tcl command
named `mqttc`. Once a MQTT broker connection is created, it can be controlled
using methods of the HANDLE command.

server connection URLs should be in the form: `(tcp|ssl)://hostname:port`.

`serverURI` is specifying the server to which the client will connect.
`clientId` is the client identifier passed to the server when the client connects to it.
`persistence_type` is used by the client. 1 is using in-memory persistence.
0 is using the default (file system-based) persistence mechanism.

`-trustStore` is specifying the file in PEM format containing the public digital
certificates trusted by the client. `-enableServerCertAuth` is an option to enable
verification of the server certificate. If -enableServerCertAuth flag setup to 1,
user needs setup the PEM format file path by using `-trustStore`.

Sub command `publishMessage` QoSs parameter is he quality of service (QoS)
assigned to the message.
0 - Fire and forget - the message may not be delivered.
1 - At least once - the message will be delivered, but may be delivered
more than once in some circumstances.
2 - Once and one only - the message will be delivered exactly once.

`subscribe` attempts to subscribe a client to a single topic.

`receive` command attempts to receive message. User will get a list:  
{topic} {message payload} dup_flag

The dup flag indicates whether or not this message is a duplicate.
It is only meaningful when receiving QoS1 messages.


Example
=====

Publish:

    package require mqttc
    mqttc client "tcp://localhost:1883" "USERSPub" 1 -timeout 1000
    client publishMessage "MQTT Examples" "Hello MQTT!" 1 0
    client publishMessage "MQTT Examples" "Exit" 1 0
    client close

Subscribe:

    package require mqttc
    mqttc client "tcp://localhost:1883" "USERSSub" 1 -cleansession 1 
    client subscribe  "MQTT Examples" 1
    while 1 {
        if {[catch {set result [client  receive]}]} {
            puts "Receive error!!!"
            break
        }
        if {[llength $result] > 0} {
            puts "[lindex $result 0] - [lindex $result 1]"
            if {![string compare -nocase [lindex $result 1] "Exit"]} {
                break
            }
        }
    }
    client unsubscribe  "MQTT Examples"
    client close

Publish (SSL anonymous connection):

    package require mqttc
    mqttc client "ssl://localhost:8883" "USERSPub" 1 -sslenable 1
    client publishMessage "MQTT Examples" "Hello MQTT!" 1 0
    client publishMessage "MQTT Examples" "Exit" 1 0
    client close

Subscribe (SSL server authentication):

    package require mqttc
    mqttc client "ssl://localhost:8883" "USERSSub" 1 -cleansession 1  -sslenable 1 \
    -trustStore /home/danilo/Public/client_local.pem -enableServerCertAuth 1
    client subscribe  "MQTT Examples" 1
    while 1 {
        if {[catch {set result [client  receive]}]} {
            puts "Receive error!!!"
            break
        }
        if {[llength $result] > 0} {
            puts "[lindex $result 0] - [lindex $result 1]"
            if {![string compare -nocase [lindex $result 1] "Exit"]} {
                break
            }
        }
    }
    client unsubscribe  "MQTT Examples"
    client close

Publish (SSL mutual authentication):

    package require mqttc
    mqttc client "ssl://localhost:8883" "USERSPub" 1 -sslenable 1 \
    -trustStore /home/danilo/Public/client_local.pem -enableServerCertAuth 1 \
    -keyStore /home/danilo/Public/client.crt \
    -privateKey /home/danilo/Public/key.pem -privateKeyPassword password
    client publishMessage "MQTT Examples" "Hello MQTT!" 1 0
    client publishMessage "MQTT Examples" "Exit" 1 0
    client close

