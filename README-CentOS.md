Infinity Plugin for CentOS 7.3
==============================

Another itch to scratch...

Make sure you have the nux repos installed
------------------------------------------

See (https://li.nux.ro/repos.html).

    yum -y install epel-release && rpm -Uvh http://li.nux.ro/download/nux/dextop/el7/x86_64/nux-dextop-release-0-5.el7.nux.noarch.rpm

Install dependencies
--------------------

    sudo yum install audacious
    sudo yum install SDL2-devel glib2-devel audacious-devel

Configure configure
-------------------

    ./configure --prefix=/usr --libdir=/usr/lib64

Build and install
-----------------

    ./make
    sudo make install

Note
----

When the infinity window shows, it looks like empty, just rescale it and the content will show up properly.


Contact
-------

    Philippe Back
    phil@highoctane.be
    @philippeback




