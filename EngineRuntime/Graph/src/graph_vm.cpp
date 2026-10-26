#include "Epidemic/Runtime/Graph/graph_vm.h"
#include "graph_flow.h"

#include <exception>
#include <limits>

namespace epidemic::runtime::graph {
namespace {
foundation::Error Error(std::string_view code) {
    return foundation::Error::Create(code, "i64 graph execution rejected");
}
bool Add(std::int64_t left, std::int64_t right, std::int64_t &result) {
    constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
    constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
    if ((right > 0 && left > maximum - right) || (right < 0 && left < minimum - right)) return false;
    result = left + right;
    return true;
}
bool Multiply(std::int64_t left, std::int64_t right, std::int64_t &result) {
    constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
    constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
    if (left == 0 || right == 0) { result = 0; return true; }
    if ((left == -1 && right == minimum) || (right == -1 && left == minimum)) return false;
    if (left > 0 ? (right > 0 ? left > maximum / right : right < minimum / left)
                 : (right > 0 ? left < minimum / right : left < maximum / right)) return false;
    result = left * right;
    return true;
}
foundation::Result<std::int64_t> Invoke(INativeInvocation &state, std::int64_t argument) {
    try { return state.Invoke(argument); }
    catch (const std::exception &error) {
        return foundation::Result<std::int64_t>::Failure(
            foundation::Error::Create("graph.vm.native_exception", "native callable threw", error.what()));
    } catch (...) {
        return foundation::Result<std::int64_t>::Failure(Error("graph.vm.native_exception"));
    }
}
foundation::Result<std::unique_ptr<INativeInvocation>> CreateState(const INativeCallableFactory &factory) {
    try { return factory.CreateInvocation(); }
    catch (const std::exception &error) {
        return foundation::Result<std::unique_ptr<INativeInvocation>>::Failure(
            foundation::Error::Create("graph.vm.factory_exception", "native factory threw", error.what()));
    } catch (...) {
        return foundation::Result<std::unique_ptr<INativeInvocation>>::Failure(Error("graph.vm.factory_exception"));
    }
}
struct RunningGuard {
    bool &running;
    ~RunningGuard() { running = false; }
};
} // namespace

GraphInstance::GraphInstance(CompiledGraphPtr program)
    : program_(std::move(program)), values_(program_->RegisterCount()) {
    if (program_->flow_) flow_ = std::make_unique<detail::FlowInstance>(*this);
}
GraphInstance::~GraphInstance() = default;
foundation::Result<std::int64_t> GraphInstance::InvokeNative(std::uint32_t site, std::int64_t value) {
    return Invoke(*native_[site], value);
}
bool GraphInstance::Arithmetic(Operation operation, std::int64_t left, std::int64_t right, std::int64_t &value) {
    return operation == Operation::Add ? Add(left, right, value) : Multiply(left, right, value);
}

foundation::Result<std::unique_ptr<GraphInstance>> GraphInstance::Create(CompiledGraphPtr program) {
    if (!program) return foundation::Result<std::unique_ptr<GraphInstance>>::Failure(Error("graph.vm.invalid_program"));
    auto instance = std::unique_ptr<GraphInstance>(new GraphInstance(std::move(program)));
    for (const auto &site : instance->program_->native_sites_) {
        auto state = CreateState(*site.factory);
        if (!state) {
            auto error = state.GetError();
            error.context += ";node=" + std::to_string(site.node.Raw());
            return foundation::Result<std::unique_ptr<GraphInstance>>::Failure(std::move(error));
        }
        if (!state.Value()) {
            auto error = Error("graph.vm.invalid_native_state");
            error.context = "node=" + std::to_string(site.node.Raw());
            return foundation::Result<std::unique_ptr<GraphInstance>>::Failure(std::move(error));
        }
        instance->native_.push_back(std::move(state).Value());
    }
    return foundation::Result<std::unique_ptr<GraphInstance>>::Success(std::move(instance));
}
foundation::Result<void> GraphInstance::SetInput(ElementId port, std::int64_t value) {
    if (started_) return foundation::Result<void>::Failure(Error("graph.vm.inputs_sealed"));
    for (const auto &slot : program_->slots_) {
        if (slot.port == port && slot.input) {
            values_[slot.index] = value;
            return foundation::Result<void>::Success();
        }
    }
    return foundation::Result<void>::Failure(Error("graph.vm.unknown_input"));
}
foundation::Result<std::int64_t> GraphInstance::ReadValue(ElementId port) const {
    for (const auto &slot : program_->slots_) {
        if (slot.port == port) {
            const auto &value = values_[slot.index];
            if (!value) return foundation::Result<std::int64_t>::Failure(Error("graph.vm.value_unavailable"));
            return foundation::Result<std::int64_t>::Success(*value);
        }
    }
    return foundation::Result<std::int64_t>::Failure(Error("graph.vm.unknown_port"));
}
foundation::Result<void> GraphInstance::ResumeWait() {
    if (running_ || cancelled_ || failure_ || !wait_)
        return foundation::Result<void>::Failure(Error("graph.vm.invalid_wait_resume"));
    values_[wait_->destination] = wait_->value;
    wait_.reset();
    return foundation::Result<void>::Success();
}
void GraphInstance::Cancel() noexcept {
    if (flow_) { flow_->Cancel(); return; }
    if (!failure_ && (wait_ || instruction_ < program_->instructions_.size())) {
        started_ = true;
        cancelled_ = true;
    }
}
ExecutionReport GraphInstance::Run(ExecutionBudget budget) {
    if (flow_) return flow_->Run(budget);
    if (running_) {
        failure_ = ExecutionDiagnostic{program_->Asset(), program_->instructions_[instruction_].node,
                                       instruction_, Error("graph.vm.reentrant")};
        return {ExecutionStatus::Failed, 0, 0, failure_};
    }
    if (failure_) return {ExecutionStatus::Failed, 0, 0, failure_};
    const auto cancelled = [&](std::size_t instructions, std::size_t calls) {
        const auto node = wait_ ? wait_->node : program_->instructions_[instruction_].node;
        const auto at = wait_ ? wait_->instruction : instruction_;
        return ExecutionReport{ExecutionStatus::Cancelled, instructions, calls,
                               ExecutionDiagnostic{program_->Asset(), node, at, Error("graph.vm.cancelled")}};
    };
    if (cancelled_) return cancelled(0, 0);
    const auto waiting = [&](std::size_t instructions, std::size_t calls) {
        return ExecutionReport{ExecutionStatus::Waiting, instructions, calls,
                               ExecutionDiagnostic{program_->Asset(), wait_->node, wait_->instruction, Error("graph.vm.waiting")},
                               wait_->duration};
    };
    if (wait_) return waiting(0, 0);
    started_ = true;
    running_ = true;
    RunningGuard guard{running_};
    ExecutionReport report;
    const auto fail = [&](foundation::Error error) {
        failure_ = ExecutionDiagnostic{program_->Asset(), program_->instructions_[instruction_].node,
                                       instruction_, std::move(error)};
        report.status = ExecutionStatus::Failed;
        report.diagnostic = failure_;
        return report;
    };
    while (instruction_ < program_->instructions_.size()) {
        const auto &step = program_->instructions_[instruction_];
        if (report.instructions == budget.instructions ||
            (step.operation == Operation::NativeCall && report.native_calls == budget.native_calls)) {
            report.status = ExecutionStatus::BudgetExhausted;
            report.diagnostic = ExecutionDiagnostic{program_->Asset(), step.node, instruction_, Error("graph.vm.budget_exhausted")};
            return report;
        }
        ++report.instructions;
        if (step.operation == Operation::Input) {
            if (!values_[step.destination]) return fail(Error("graph.vm.missing_input"));
        } else {
            const auto &left_value = values_[step.left];
            if (!left_value) return fail(Error("graph.vm.value_unavailable"));
            const auto left = *left_value;
            std::int64_t result = 0;
            switch (step.operation) {
            case Operation::Add:
            case Operation::Multiply: {
                const auto &right_value = values_[step.right];
                if (!right_value) return fail(Error("graph.vm.value_unavailable"));
                if (!(step.operation == Operation::Add ? Add(left, *right_value, result)
                                                       : Multiply(left, *right_value, result)))
                    return fail(Error("graph.vm.integer_overflow"));
                break;
            }
            case Operation::NativeCall: {
                ++report.native_calls;
                const auto invoked = InvokeNative(step.callable, left);
                if (failure_) return {ExecutionStatus::Failed, report.instructions, report.native_calls, failure_};
                if (cancelled_) return cancelled(report.instructions, report.native_calls);
                if (!invoked) return fail(invoked.GetError());
                result = invoked.Value();
                break;
            }
            case Operation::Output: result = left; break;
            case Operation::Wait: {
                const auto &duration = values_[step.right];
                if (!duration) return fail(Error("graph.vm.value_unavailable"));
                if (*duration < 0) return fail(Error("graph.vm.negative_wait"));
                wait_ = PendingWait{GameDuration{*duration}, step.node, instruction_, step.destination, left};
                ++instruction_;
                return waiting(report.instructions, report.native_calls);
            }
            case Operation::Input:
            case Operation::Entry:
            case Operation::Sequence:
            case Operation::Branch: return fail(Error("graph.vm.invalid_instruction"));
            }
            values_[step.destination] = result;
        }
        ++instruction_;
    }
    return report;
}
} // namespace epidemic::runtime::graph
