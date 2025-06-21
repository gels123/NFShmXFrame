#include "evpp/inner_pre.h"

#include "evpp/event_loop.h"
#include "evpp/event_watcher.h"

namespace evpp {

InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic)
    : loop_(evloop), timeout_(timeout), functor_(f), periodic_(periodic) {
    DEVPP_LOG_TRACE << "loop=" << loop_;
}

InvokeTimer::InvokeTimer(EventLoop* evloop, Duration timeout, Functor&& f, bool periodic)
    : loop_(evloop), timeout_(timeout), functor_(std::move(f)), periodic_(periodic) {
    DEVPP_LOG_TRACE << "loop=" << loop_;
}

InvokeTimerPtr InvokeTimer::Create(EventLoop* evloop, Duration timeout, const Functor& f, bool periodic) {
    InvokeTimerPtr it(new InvokeTimer(evloop, timeout, f, periodic));
    it->self_ = it;
    return it;
}

InvokeTimerPtr InvokeTimer::Create(EventLoop* evloop, Duration timeout, Functor&& f, bool periodic) {
    InvokeTimerPtr it(new InvokeTimer(evloop, timeout, std::move(f), periodic));
    it->self_ = it;
    return it;
}

InvokeTimer::~InvokeTimer() {
    DEVPP_LOG_TRACE << "loop=" << loop_;
}

void InvokeTimer::Start() {
    DEVPP_LOG_TRACE << "loop=" << loop_ << " refcount=" << self_.use_count();
    auto f = [this]() {
        timer_.reset(new TimerEventWatcher(loop_, [time_weak = std::weak_ptr<InvokeTimer>(shared_from_this())]() {
            auto time_ptr = time_weak.lock();
            if (time_ptr) {
                time_ptr->OnTimerTriggered();
            }
        }, timeout_));

        timer_->SetCancelCallback([time_weak = std::weak_ptr<InvokeTimer>(shared_from_this())]() {
            auto time_ptr = time_weak.lock();
            if (time_ptr) {
                time_ptr->OnCanceled();
            }
        });
        timer_->Init();
        timer_->AsyncWait();
        DEVPP_LOG_TRACE << "timer=" << timer_.get() << " loop=" << loop_ << " refcount=" << self_.use_count() << " periodic=" << periodic_ << " timeout(ms)=" << timeout_.Milliseconds();
    };
    loop_->RunInLoop(std::move(f));
}

void InvokeTimer::Cancel() {
    DEVPP_LOG_TRACE;
    auto f = [time_weak = std::weak_ptr<InvokeTimer>(shared_from_this())]() {
        auto time_ptr = time_weak.lock();
        if (time_ptr && time_ptr->timer_) {
            time_ptr->timer_->Cancel();
        }
    };
    loop_->RunInLoop(std::move(f));
}

void InvokeTimer::OnTimerTriggered() {
    DEVPP_LOG_TRACE << "loop=" << loop_ << " use_count=" << self_.use_count();
    functor_();

    if (periodic_) {
        timer_->AsyncWait();
    } else {
        timer_.reset();
        self_.reset();
    }
}

void InvokeTimer::OnCanceled() {
    DEVPP_LOG_TRACE << "loop=" << loop_ << " use_count=" << self_.use_count();
    periodic_ = false;
    if (cancel_callback_) {
        cancel_callback_();
    }
    timer_.reset();
    self_.reset();
}

}
