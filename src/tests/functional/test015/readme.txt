Test #015

Test LoadableModule, in particular, check that static objects from dynamically loaded modules are destroyed when the modules are unloaded, and not on the program exit (this should be true for all sane compilers).

Note: this is known not to be true at least for the Embarcadero 10.1 Clang-based compiler. This particular bug manifests when both the host program and the module depend on the same C++ shared library.
