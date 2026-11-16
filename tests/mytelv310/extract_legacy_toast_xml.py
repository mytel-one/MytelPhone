"""Extract the exact pre-fix toast assembly for a reproducible QtCore-only RED.

Reads one immutable local Git object; no SDK/app/registry/network operations.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess

SOURCE_COMMIT = "c65d15af3c9a73392d53c35e5cc325826776ddfc"
SOURCE_FILE = "Linphone/core/notifier/WindowsNotificationBackend.cpp"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    source = subprocess.run(["git", "-C", str(root), "show", SOURCE_COMMIT + ":" + SOURCE_FILE],
                            check=True, capture_output=True).stdout
    text = source.decode("utf-8")

    def block(marker):
        start = text.index(marker)
        end = text.index('L"</toast>";', start) + len('L"</toast>";')
        return text[start:end]

    message = block('std::wstring xml = L"<toast>"')
    call = block('std::wstring xml = L"<toast scenario=\\"reminder\\">"')
    action_start = text.index("std::wstring wActions;", text.index("void NotificationBackend::sendCallNotification"))
    action_end = text.index('std::wstring xml = L"<toast scenario=\\"reminder\\">"', action_start)
    actions = text[action_start:action_end]
    output = '''// Exact pre-fix production XML assembly from %s:%s
#include <QDebug>
#include <QList>
#include <QString>
#include <string>
namespace MyTelToastXml {
struct Action { QString label; QString argument; QString icon; };
inline QString message(const QString &title, const QString &sender, const QString &body, const QString &icon) {
    const auto chatRoomName = title.toStdWString();
    const auto remoteAddress = sender.toStdWString();
    const auto msgTxt = body.toStdWString();
    const auto appIcon = icon.toStdWString();
    const bool isGroup = true;
    %s
    return QString::fromStdWString(xml);
}
inline QString call(const QString &title, const QString &address, const QString &description,
                    const QString &icon, const QList<Action> &actions) {
    const auto displayName = title.toStdWString();
    const auto remoteAddress = address.toStdWString();
    const auto callDescription = description.toStdWString();
    const auto appIcon = icon.toStdWString();
    %s
    %s
    return QString::fromStdWString(xml);
}
}
''' % (SOURCE_COMMIT, SOURCE_FILE, message, actions, call)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output, encoding="utf-8")
    print("LEGACY_PRODUCTION_SOURCE_SHA=" + SOURCE_COMMIT)
    print("LEGACY_PRODUCTION_FILE_SHA256=" + hashlib.sha256(source).hexdigest())


if __name__ == "__main__":
    main()
