// GCC 13.2 and later reference std::ios_base_library_init() so that
// libstdc++ is linked. Qt 6.12 ships the MinGW 13.1 libstdc++-6.dll,
// which does not export that symbol. Defining it here lets the program
// keep Qt's runtime DLL. Qt6Core.dll imports __cxa_call_terminate from
// that DLL; the cross compiler's libstdc++ does not provide it.
#if defined(__MINGW32__)
namespace std {
void ios_base_library_init() {}
}
#endif
