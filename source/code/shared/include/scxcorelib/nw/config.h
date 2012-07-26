#if defined(linux)
#define CONFIG_OS_LINUX
#endif

#if !defined(sun)
#define CONFIG_HAVE_FUNCTION_MACRO
#endif

#if defined(hpux) && defined(ia64)
#define CONFIG_PLATFORM_HPUX_IA64_HP
#endif
