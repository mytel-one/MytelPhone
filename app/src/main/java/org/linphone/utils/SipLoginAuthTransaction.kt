package org.linphone.utils

/** Owns only the native authentication clone inserted by one unfinished login. */
class SipLoginAuthTransaction<T>(
    private val current: () -> List<T>,
    private val add: (T) -> Unit,
    private val remove: (T) -> Unit,
    private val pointer: (T) -> Long,
    private val sameIdentity: (T, T) -> Boolean
) {
    // Strong snapshots keep native-backed wrappers alive until commit/rollback.
    private var owned: T? = null
    private var displaced: List<T> = emptyList()

    fun begin(auth: T) {
        rollback()
        val before = current()
        val beforeIds = before.map(pointer).toSet()
        add(auth)
        val after = current()
        val afterIds = after.map(pointer).toSet()
        owned = after.firstOrNull {
            pointer(it) != 0L && pointer(it) !in beforeIds && sameIdentity(it, auth)
        }
        displaced = before.filter { pointer(it) !in afterIds && sameIdentity(it, auth) }
    }

    fun rollback() {
        val stored = owned
        if (stored != null && current().any { pointer(it) == pointer(stored) }) {
            remove(stored)
            displaced.forEach { previous ->
                if (current().none { sameIdentity(it, previous) }) add(previous)
            }
        }
        // A later independent replacement/deletion keeps its own current state.
        commit()
    }

    fun commit() {
        owned = null
        displaced = emptyList()
    }
}
