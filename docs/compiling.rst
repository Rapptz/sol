.. _sol-compiling:

Compiling Tests and Examples
==============================

Sol comes with tests that allow you to test if Sol is working properly or if your compiler supports the C++11 features
required to use Sol. The requirements to build the tests are:

- `ninja <https://github.com/martine/ninja>`_ v1.3 or higher.
- `python <https://www.python.org/>`_ v2.7 or higher.
- A C++11 compiler

.. _sol-compiling-cloning:

Proper Cloning
----------------

Sol uses git submodules, and as a result there is a bit of a strange way of cloning. When cloning, make sure
that you use the following command::

    $ git clone --recursive https://github.com/Rapptz/sol.git
    $ cd sol

If you forgot to clone through the recursive method, you should do it the following way instead::

    $ git clone https://github.com/Rapptz/sol.git
    $ cd sol
    $ git submodule update --init --recursive

.. _sol-compiling-bootstrap:

Bootstrapping
------------------

Sol comes with a ``bootstrap.py`` script that handles all the set up needed to create the ninja file used for compiling.
The following options are available:

.. program:: bootstrap.py

.. option:: --debug

    Creates a ninja file used for debugging purposes. There should be really no purpose to use this since debugging
    the test file is a painful experience anyway.
.. option:: --cxx

    Specifies the C++ compiler to use. The CLI for it must be similar to GCC. ``cl.exe`` is not supported.
    If not provided, the default value is ``g++``.
.. option:: --lua-dir

    Specifies the directory that lua resides in. There must be ``include`` and ``lib`` subdirectories. The ``include``
    subdirectory would be where all the ``.h`` files are located while the ``lib`` subdirectory is the directory that
    contains the ``.a`` file.
.. option:: --install-dir

    Specifies the directory to install the headers to when calling ``ninja install``. This is the directory where all
    the header files will be copied to. Compilers typically have a default location to look for header files, such as
    ``/usr/include`` on Linux.

    On Linux this defaults to ``/usr/local/include``. On Windows this defaults to ``./include``.

.. _sol-compiling-ninja:

Using Ninja
----------------

After bootstrapping the ninja file, you can now use it. The proper targets to use are as follows:

.. program:: ninja

.. option:: tests

    Compiles the test files. The resulting executable is located in ``./bin/tests``.
.. option:: examples

    Compiles all of the examples. The resulting executables are located in ``./bin/examples/``.
.. option:: install

    Copies the header files in the directory given by :option:`bootstrap.py --install-dir`.
.. option:: uninstall

    Removes the header files in the directory given by :option:`bootstrap.py --install-dir`.
.. option:: run

    Runs the test executable. Usually not called by the user since it's used for CI purposes.
