#pragma once
#include <symocraft/telemetry/performance.h>

namespace SymoCraft::Test {
    // Keep existing protocol/publication assertions while consuming the new owned receipt.
    inline bool ExportSealed(Performance::Session& session, bool normal_exit) {
        session.Seal({normal_exit, true, session.Completed(), {}, {}});
        const auto result = session.Export();
        if (result.state != Performance::ExportState::Saved) {
            if (result.native_error) throw std::system_error(result.native_error, "Export " + result.stage
                + "; native_code=" + std::to_string(result.native_error.value())
                + "; category=" + result.native_error.category().name() + ": " + result.diagnostic);
            throw std::runtime_error("Export " + result.stage + ": " + result.diagnostic);
        }
        return result.valid_run;
    }
}
