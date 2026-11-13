#include <symocraft/profiling/profiling.h>
#include <string>

int OtherTranslationUnit();
int main() {
    int count = 0;
    SYMOCRAFT_PROFILE_ZONE("profiling.contract");
    SYMOCRAFT_PROFILE_ZONE("profiling.same-line.a"); SYMOCRAFT_PROFILE_ZONE("profiling.same-line.b");
    {
        SYMOCRAFT_PROFILE_ZONE("profiling.nested");
        SYMOCRAFT_PROFILE_PLOT("contract.increment [count]", ++count);
        SYMOCRAFT_PROFILE_APP_INFO(std::string(++count, 'a'));
    }
    SYMOCRAFT_PROFILE_FRAME();
#if defined(SYMOCRAFT_TRACY_ENABLED)
    if (count != 2) return 1;
#else
    // Deliberately undefined names prove no hidden argument evaluation/formatting.
    SYMOCRAFT_PROFILE_ZONE(undefined_name());
    SYMOCRAFT_PROFILE_PLOT(undefined_name(), undefined_value());
    SYMOCRAFT_PROFILE_APP_INFO(undefined_identity());
    if (count != 0) return 2;
#endif
    return OtherTranslationUnit();
}
