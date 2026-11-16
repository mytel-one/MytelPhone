/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of linphone-android
 * (see https://www.linphone.org).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
package org.linphone.ui.main.chat

/** SDK event identity and page ownership; never merge messages by body or time. */
object ConversationHistoryPolicy {
    data class MessageKey(val id: String, val outgoing: Boolean)
    data class Page(val begin: Int, val end: Int)

    fun messageKey(id: String?, outgoing: Boolean): MessageKey? =
        id?.takeIf { it.isNotBlank() }?.let { MessageKey(it, outgoing) }

    fun nextPage(loadedCoreEvents: Int, totalSdkEvents: Int, pageSize: Int): Page? {
        val begin = loadedCoreEvents
        if (begin < 0 || pageSize <= 0 || begin >= totalSdkEvents) return null
        return Page(begin, minOf(totalSdkEvents.toLong(), begin.toLong() + pageSize).toInt())
    }

    fun <T> unseen(existing: Set<MessageKey>, incoming: List<T>, key: (T) -> MessageKey?): List<T> {
        val seen = existing.toMutableSet()
        return incoming.filter { key(it)?.let(seen::add) ?: true }
    }
}

class ConversationHistoryPageGate {
    class Ticket internal constructor(internal val generation: Long)
    private var generation = 0L
    private var pending: Ticket? = null

    @Synchronized fun begin(): Ticket? {
        if (pending != null) return null
        return Ticket(generation).also { pending = it }
    }
    @Synchronized fun isCurrent(ticket: Ticket): Boolean = pending === ticket && ticket.generation == generation
    @Synchronized fun finish(ticket: Ticket) { if (pending === ticket) pending = null }
    @Synchronized fun invalidate() { generation += 1; pending = null }
}
