#include "graph_flow.h"
#include <algorithm>

namespace epidemic::runtime::graph::detail {
FlowInstance::FlowInstance(GraphInstance &owner)
    : owner_(owner), cache_(owner.values_.size()) {
  Activate(owner_.program_->flow_->entry, std::nullopt);
}
void FlowInstance::Activate(std::uint32_t entry,
                            std::optional<std::int64_t> payload) {
  work_.clear();
  if (entry == kNoNode)
    return;
  owner_.instruction_ = entry;
  if (payload)
    owner_.values_[entry] = *payload;
  Push(entry);
}
void FlowInstance::Push(std::uint32_t node) {
  if (node != kNoNode)
    work_.push_back({Kind::Execute, node});
}
void FlowInstance::NextEpoch() {
  if (++epoch_ == 0) {
    std::fill(cache_.begin(), cache_.end(), 0);
    epoch_ = 1;
  }
}
ExecutionDiagnostic FlowInstance::Diagnostic(std::string_view code) const {
  return {owner_.program_->Asset(),
          owner_.program_->instructions_[owner_.instruction_].node,
          owner_.instruction_,
          foundation::Error::Create(code, "control graph execution")};
}
ExecutionReport FlowInstance::Fail(std::string_view code,
                                   ExecutionReport report) {
  return Fail(Diagnostic(code).error, std::move(report));
}
ExecutionReport FlowInstance::Fail(foundation::Error error,
                                   ExecutionReport report) {
  owner_.failure_ = Diagnostic(error.code);
  owner_.failure_->error = std::move(error);
  report.status = ExecutionStatus::Failed;
  report.diagnostic = owner_.failure_;
  return report;
}
void FlowInstance::Cancel() noexcept {
  if (!completed_ && !owner_.failure_) {
    owner_.started_ = true;
    owner_.cancelled_ = true;
  }
}
ExecutionReport FlowInstance::Run(ExecutionBudget budget) {
  ExecutionReport report;
  if (owner_.running_)
    return Fail("graph.vm.reentrant", report);
  if (owner_.failure_)
    return {ExecutionStatus::Failed, 0, 0, owner_.failure_};
  if (owner_.cancelled_)
    return {ExecutionStatus::Cancelled, 0, 0, Diagnostic("graph.vm.cancelled")};
  if (owner_.wait_)
    return {ExecutionStatus::Waiting, 0, 0, Diagnostic("graph.vm.waiting"),
            owner_.wait_->duration};
  owner_.started_ = true;
  owner_.running_ = true;
  struct Guard {
    bool &running;
    ~Guard() { running = false; }
  } guard{owner_.running_};
  const auto &program = *owner_.program_->flow_;
  auto &values = owner_.values_;
  while (!work_.empty()) {
    const auto work = work_.back();
    owner_.instruction_ = work.node;
    const auto &step = owner_.program_->instructions_[work.node];
    const auto &node = program.nodes[work.node];
    if (report.instructions == budget.instructions ||
        (work.kind == Kind::Apply && step.operation == Operation::NativeCall &&
         report.native_calls == budget.native_calls)) {
      report.status = ExecutionStatus::BudgetExhausted;
      report.diagnostic = Diagnostic("graph.vm.budget_exhausted");
      return report;
    }
    ++report.instructions;
    if (work.kind == Kind::Clear) {
      values[work.node].reset();
      cache_[work.node] = 0;
      work_.pop_back();
      continue;
    }
    if (work.kind == Kind::Repeat) {
      NextEpoch();
      if (work.iteration == work.count) {
        values[work.node].reset();
        work_.pop_back();
        Push(node.next[1]);
      } else {
        values[work.node] = work.iteration;
        ++work_.back().iteration;
        Push(node.next[0]);
        for (auto i = node.clear.rbegin(); i != node.clear.rend(); ++i)
          work_.push_back({Kind::Clear, *i});
      }
      continue;
    }
    if (work.kind == Kind::Execute) {
      NextEpoch();
      work_.back().kind = Kind::Apply;
      for (auto i = node.data.rbegin(); i != node.data.rend(); ++i)
        work_.push_back({Kind::Evaluate, *i});
      continue;
    }
    if (work.kind == Kind::Evaluate) {
      if (step.operation == Operation::Add ||
          step.operation == Operation::Multiply) {
        if (cache_[work.node] == epoch_) {
          work_.pop_back();
          continue;
        }
        work_.back().kind = Kind::Finish;
        for (auto i = node.data.rbegin(); i != node.data.rend(); ++i)
          work_.push_back({Kind::Evaluate, *i});
      } else {
        if (!values[work.node])
          return Fail("graph.flow.value_unavailable", report);
        work_.pop_back();
      }
      continue;
    }
    if (work.kind == Kind::Finish) {
      const auto &a = values[node.data[0]];
      const auto &b = values[node.data[1]];
      if (!a || !b)
        return Fail("graph.flow.value_unavailable", report);
      std::int64_t result = 0;
      if (!GraphInstance::Arithmetic(step.operation, *a, *b, result))
        return Fail("graph.vm.integer_overflow", report);
      values[work.node] = result;
      cache_[work.node] = epoch_;
      work_.pop_back();
      continue;
    }
    switch (step.operation) {
    case Operation::Entry:
    case Operation::EventEntry:
    case Operation::Sequence:
      work_.pop_back();
      for (auto i = node.next.rbegin(); i != node.next.rend(); ++i)
        Push(*i);
      break;
    case Operation::Branch: {
      const auto &condition = values[node.data[0]];
      if (!condition)
        return Fail("graph.flow.value_unavailable", report);
      work_.pop_back();
      Push(node.next[*condition != 0 ? 0 : 1]);
      break;
    }
    case Operation::Repeat: {
      const auto &count = values[node.data[0]];
      if (!count)
        return Fail("graph.flow.value_unavailable", report);
      if (*count < 0 || *count > kMaxRepeatIterations)
        return Fail("graph.flow.repeat_count", report);
      work_.pop_back();
      work_.push_back({Kind::Repeat, work.node, *count, 0});
      break;
    }
    case Operation::NativeCall: {
      const auto &input = values[node.data[0]];
      if (!input)
        return Fail("graph.flow.value_unavailable", report);
      ++report.native_calls;
      const auto result = owner_.InvokeNative(step.callable, *input);
      if (owner_.failure_) {
        report.status = ExecutionStatus::Failed;
        report.diagnostic = owner_.failure_;
        return report;
      }
      if (owner_.cancelled_) {
        report.status = ExecutionStatus::Cancelled;
        report.diagnostic = Diagnostic("graph.vm.cancelled");
        return report;
      }
      if (!result)
        return Fail(result.GetError(), report);
      values[work.node] = result.Value();
      work_.pop_back();
      Push(node.next[0]);
      break;
    }
    case Operation::Wait: {
      const auto &value = values[node.data[0]];
      const auto &duration = values[node.data[1]];
      if (!value || !duration)
        return Fail("graph.flow.value_unavailable", report);
      if (*duration < 0)
        return Fail("graph.vm.negative_wait", report);
      owner_.wait_ =
          GraphInstance::PendingWait{GameDuration{*duration}, step.node,
                                     owner_.instruction_, work.node, *value};
      values[work.node].reset();
      work_.pop_back();
      Push(node.next[0]);
      report.status = ExecutionStatus::Waiting;
      report.diagnostic = Diagnostic("graph.vm.waiting");
      report.wait = owner_.wait_->duration;
      return report;
    }
    case Operation::Output: {
      const auto &input = values[node.data[0]];
      if (!input)
        return Fail("graph.flow.value_unavailable", report);
      values[work.node] = *input;
      work_.pop_back();
      break;
    }
    default:
      return Fail("graph.vm.invalid_instruction", report);
    }
  }
  completed_ = true;
  return report;
}
} // namespace epidemic::runtime::graph::detail
