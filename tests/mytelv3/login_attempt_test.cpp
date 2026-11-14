#include "tool/LoginAttempt.hpp"
#include <atomic>
#include <cstdio>
#include <thread>

int main() {
	LoginAttempt cancelled;
	if (!cancelled.cancel() || cancelled.tryAccept() || !cancelled.isCancelled()) return 1;
	LoginAttempt accepted;
	if (!accepted.tryAccept() || accepted.cancel() || accepted.isCancelled()) return 1;
	for (int i = 0; i < 500; ++i) {
		LoginAttempt racing;
		std::atomic_bool committed{false};
		std::thread worker([&]() { committed = racing.tryAccept(); });
		const bool cancellationWon = racing.cancel();
		worker.join();
		if (cancellationWon && committed) return 1;
		if (cancellationWon != racing.isCancelled()) return 1;
	}
	std::puts("Login acceptance/cancellation: 502 scenarios PASS");
	return 0;
}
