# chttpd
A proof of concept HTTP server written in the C programming language--and extremely vulnerable! **chttpd is to not be used for serious applications, as it is unsafe to do so.**

chttpd can serve static files--determining the correct MIME type with a small list of known extensions, which may be extended--as well as execute CGI scripts (that of PHP only right now) completely. Since CGI is a fairly simple protocol, only requiring environment variables and the feeding of POST data into *stdin*, this is not too much of an accomplishment--never mind that it is, functionally, useless, due to the glaring security holes in chttpd.

chttpd has a plethora of fatal flaws:

 - It is written in C (especially, because I am inexperienced). Memory leaks and incorrect memory management will be a staple of chttpd. **If you do not want your system taken over, do not run chttpd over the open internet, by port forwarding it.**
 - It uses threading to handle each client asynchronously, which can lead to a Slowloris DDOS attack vulnerability.
 - It uses Linux sockets, reducing its cross-compatibility, as well as making it exponentially harder to equip it with TLS support.
 - Speaking of TLS: no TLS support; your banking details will be stolen from attackers on the network.
 - The vast majority of the HTTP 2.0 protocol is not implemented; the only methods supported are: GET, POST, and HEAD.
 - Not all client inputs are validated; somebody may have the fun idea to enter nothing via *netcat*.
 - Static configuration: chttpd must be reconfigured through recompilation. In addition to that, the abilities of chttpd are limited, relative to that of Apache or NGINX--chttpd will never have proxying.
 - The client can send a file that will fill up the host's memory, as the default directory is /tmp/, which is typically on a ram disk. 
 - ... many more things.

To be true to the claim that chttpd supports CGI applications, chttpd also supports cookies, which are surprisingly very easy to implement. The server will send a *Set-Cookie* header to the client when it wishes to store it on the client, and the client will send a *Cookie* header when it is sending one of its cookies. chttpd's job is to pass this *Cookie* header to PHP/CGI via an environment variable. 

chttpd employs the usage of special files to tell it about how it should treat directories:

 - .nocgi - which will specify that CGI applications will not be executed in the directory.
 - .private - which will make the directory and all of its files private.
 - .nolist - which will disable the directory listing for that directory.

The HTTP 2.0 protocol is a conceptually simple one, which is why chttpd exists, as a way to understand the HTTP 2.0 protocol and actually implement it. One can do this in Python quite easily--the ease coming with crudeness--by using a library or the sockets interface. Application protocols, however, are best designed in a lower-level language--relative to that of Python, of course--ensuring speed, security, and precision when it is required. 

I may update chttpd as to improve on its features and security, along with its overall design. For instance, chttpd is using heap allocation for variable strings, instead of stack VLA strings--the more efficient and safe approach.  
