#ifndef MYTEL_LOGIN_ATTEMPT_HPP
#define MYTEL_LOGIN_ATTEMPT_HPP

#include <atomic>

// SDK acceptance and GUI cancellation share one linearization point. Once
// cancelled, this attempt can never register/default an account later.
class LoginAttempt {
public:
	bool tryAccept() {
		State expected = State::Active;
		return mState.compare_exchange_strong(expected, State::Accepted);
	}
	bool cancel() {
		State expected = State::Active;
		return mState.compare_exchange_strong(expected, State::Cancelled) || expected == State::Cancelled;
	}
	bool isCancelled() const {
		return mState.load() == State::Cancelled;
	}

private:
	enum class State { Active, Accepted, Cancelled };
	std::atomic<State> mState{State::Active};
};

#endif
