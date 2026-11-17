#include <cstdio>
#include <string>

// Exercise production getters even with the old build flag and saved flags on.
#define ENABLE_UPDATE_CHECK 1
#define Q_UNUSED(value) (void)value;

namespace SavedSettings {
struct Config {
	int getInt(const char *, const char *, bool) const {
		return 1;
	}
};
class SettingsModel {
public:
	Config config;
	Config *mConfig = &config;
	static constexpr const char *UiSection = "ui";
	bool autoCheckForUpdateOnStart() const;
	bool isCheckForUpdateAvailable() const;
	bool isCheckForUpdateEnabled() const;
};
#include "auto_update.inc"
#include "update_available.inc"
#include "update_enabled.inc"
} // namespace SavedSettings

namespace Dispatch {
struct MiniString {
	bool isEmpty() const {
		return true;
	}
};
struct SettingsModel {
	static SettingsModel *getInstance() {
		static SettingsModel instance;
		return &instance;
	}
	bool isCheckForUpdateEnabled() const {
		return true;
	}
	MiniString getVersionCheckUrl() const {
		return {};
	}
	void setVersionCheckUrl(const char *) {
	}
};
struct Constants {
	static constexpr const char *VersionCheckReleaseUrl = "https://retired.invalid";
};
struct NullLog {
	template <typename T>
	NullLog &operator<<(const T &) {
		return *this;
	}
	const char *arg(const char *text) {
		return text;
	}
};
inline NullLog log() {
	return {};
}
#define lInfo()                                                                                                        \
	NullLog {                                                                                                          \
	}
struct Core {
	int requests = 0;
	void checkForUpdate(const std::string &) {
		++requests;
	}
};
class CoreModel {
public:
	bool mCheckVersionRequestedByUser = false;
	Core core;
	Core *getCore() {
		return &core;
	}
	void checkForUpdate(const std::string &applicationVersion, bool requestedByUser);
};
#include "update_dispatch.inc"
} // namespace Dispatch

int main() {
	int failures = 0;
	auto check = [&failures](bool passed, const char *name) {
		std::printf("%s %s\n", passed ? "PASS" : "FAIL", name);
		if (!passed) ++failures;
	};
	SavedSettings::SettingsModel settings;
	check(!settings.autoCheckForUpdateOnStart(), "saved auto-update flag cannot enable startup checks");
	check(!settings.isCheckForUpdateAvailable(), "legacy enabled build cannot expose update action");
	check(!settings.isCheckForUpdateEnabled(), "saved update flag cannot enable checks");
	Dispatch::CoreModel model;
	model.checkForUpdate("3.1.2", false);
	check(model.core.requests == 0, "automatic request cannot reach SDK network updater");
	model.checkForUpdate("3.1.2", true);
	check(model.core.requests == 0, "explicit stale caller cannot reach SDK network updater");
	return failures ? 1 : 0;
}
