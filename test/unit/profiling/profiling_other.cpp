#include <symocraft/profiling/profiling.h>
int OtherTranslationUnit() {
    SYMOCRAFT_PROFILE_ZONE("profiling.other");
    SYMOCRAFT_PROFILE_PLOT("contract.other [count]", 1);
    SYMOCRAFT_PROFILE_APP_INFO("Symocraft profiling multi-translation-unit contract");
    return 0;
}
