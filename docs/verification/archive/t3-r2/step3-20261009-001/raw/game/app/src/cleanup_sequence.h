#pragma once

#include <exception>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace SymoCraft::Application::Detail {
    class CleanupSequence {
    public:
        explicit CleanupSequence(std::ostream& diagnostics) noexcept : diagnostics_(diagnostics) {}

        template<class Action>
        void Run(const char* operation, Action&& action) noexcept {
            try {
                std::forward<Action>(action)();
            } catch (...) {
                const auto failure = std::current_exception();
                if (!first_failure_) {
                    first_failure_ = failure;
                    first_operation_ = operation;
                }
                // A diagnostic stream failure must not stop the remaining releases.
                try {
                    try {
                        std::rethrow_exception(failure);
                    } catch (const std::exception& error) {
                        diagnostics_ << "[cleanup] " << operation << ": " << error.what() << '\n';
                    } catch (...) {
                        diagnostics_ << "[cleanup] " << operation << ": unexpected non-standard exception\n";
                    }
                } catch (...) {}
            }
        }

        template<class Action>
        void RunOnce(const char* operation, bool& active, Action&& action) noexcept {
            if (std::exchange(active, false)) Run(operation, std::forward<Action>(action));
        }

        void RethrowFailure() const {
            if (!first_failure_) return;
            try {
                std::rethrow_exception(first_failure_);
            } catch (const std::exception& error) {
                throw std::runtime_error(std::string("Application cleanup failed at ") + first_operation_ + ": " + error.what());
            } catch (...) {
                throw std::runtime_error(std::string("Application cleanup failed at ") + first_operation_ +
                                         ": unexpected non-standard exception");
            }
        }

    private:
        std::ostream& diagnostics_;
        std::exception_ptr first_failure_;
        const char* first_operation_ = nullptr;
    };
}
