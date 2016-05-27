Installing MOOSE
===============

Use pre-built packages
----------------------

Use our repositories hosted at [Open Build Service](http://build.opensuse.org).
We have packages for Debian, Ubuntu, CentOS, Fedora, OpenSUSE/SUSE, RHEL,
Scientific Linux.  Visit the following page and follow the instructions there. 

https://software.opensuse.org/download.html?project=home:moose&package=moose

Building from source
-------------------

First, Download the latest source code of moose from github.

    $ git clone https://github.com/BhallaLab/moose
    $ cd moose

Install dependencies
~~~~~~~~~~~~~~~~~~~


For `pymoose` (python module of MOOSE), following additional packages are required:

- gsl-1.16 or higher.
- libhdf5 
- libsbml (optional)

    Make sure that `libsml` is installed with `zlib` and `lxml` support.
    If you are using buildtools, then use the following to install libsbml.

        - wget http://sourceforge.net/projects/sbml/files/libsbml/5.9.0/stable/libSBML-5.9.0-core-src.tar.gz
        - tar -xzvf libSBML-5.9.0-core-src.tar.gz 
        - cd libsbml-5.9.0 
        - ./configure --prefix=/usr --with-zlib --with-bzip2 --with-libxml 
        - make 
        - ctest --output-on-failure # optional
        - sudo make install 

- Development package of python e.g. libpython-dev 
- python-numpy 

For MOOSE Graphical User Interface (GUI), there are additional dependencies: 
    
- python-matplotlib 
- Python-qt4

On Ubuntu/Fedora, these can be installed with:

    sudo apt-get install python-matplotlib python-qt4
    sudo yum install python-matplotlib python-qt4 

Now use `cmake` to build moose
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    $ mkdir _build
    $ cd _build 
    $ cmake -DWITH_DOC=OFF ..
    $ make 
    $ ctest --output-on-failure

This will build pyMOOSE (MOOSE's python extention), `ctest` will run few tests to
check if build process was successful.

To install MOOSE into non-standard directory, pass additional argument
`-DCMAKE_INSTALL_PREFIX=path/to/install/dir` to cmake.

After that installation is pretty easy.

    $ sudo make install

Building and installing moogli 
-----------------------------

MOOGLI is subproject of moogli for visualizing models. Details can be found
[here](http://moose.ncbs.res.in/moogli).

MOOGLI dependencies are huge! It uses `OpenSceneGraph` which has its own
dependencies. In nutshell, depending on your distribution, you would need
following packages to be installed.

- Development package of libopenscenegraph 
- [libQGLViewer-2.3.15-py](https://gforge.inria.fr/frs/?group_id=773). Install
instructions [here](http://www.libqglviewer.com//installUnix.html#linux)

- [PyQGLViewer0.10](https://gforge.inria.fr/frs/?group_id=773) (first install
libQGLViewer-2.3.15-py) and untar contents.

    $ cd / PyQGLViewer0.10
    $ python setup.py build # to compile
    $ python setup.py install # to install on your system
    $ python setup.py bdist # to create a binary distribution

On Ubuntu, following packages should suffice:

    $ sudo apt-get install python-qt4-dev python-qt4-gl libopenscenegraph-dev python-sip-dev
    libqt4-dev 