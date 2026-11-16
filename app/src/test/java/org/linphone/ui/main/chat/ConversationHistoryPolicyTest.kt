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

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class ConversationHistoryPolicyTest {
    private data class Event(val id: String?, val outgoing: Boolean = false, val body: String = "same text", val time: Long = 100)
    private fun key(event: Event) = ConversationHistoryPolicy.messageKey(event.id, event.outgoing)
    private fun unseen(existing: List<Event>, page: List<Event>) = ConversationHistoryPolicy.unseen(existing.mapNotNull(::key).toSet(), page, ::key)

    @Test fun staleAdapterTwoCannotPrependAWhenCoreAlreadyOwnsAOutgoingBAndC() {
        val core = listOf(Event("A"), Event("B", true), Event("C"))
        val newestFirstDatabase = core.reversed()
        // The adapter still reports two rows; only the current three SDK events own the offset.
        val page = ConversationHistoryPolicy.nextPage(core.size, newestFirstDatabase.size, 30)
        val result = if (page == null) core else newestFirstDatabase.subList(page.begin, page.end).reversed() + core
        assertEquals(listOf("A", "B", "C"), result.map { it.id })
    }

    @Test fun pagingUsesLatestCoreRawEventCountAfterDeleteOrArrival() {
        assertEquals(ConversationHistoryPolicy.Page(3, 8), ConversationHistoryPolicy.nextPage(3, 8, 30))
        assertEquals(ConversationHistoryPolicy.Page(1, 3), ConversationHistoryPolicy.nextPage(1, 3, 30))
        assertNull(ConversationHistoryPolicy.nextPage(3, 3, 30))
    }

    @Test fun overlapAgainstCurrentAndWithinOneBatchCreatesOnlyNewModels() {
        val current = listOf(Event("A"), Event("B", true))
        val page = listOf(Event("older"), Event("older"), Event("A"), Event("B", true))
        assertEquals(listOf("older"), unseen(current, page).map { it.id })
    }

    @Test fun initialHistoryAlsoDeduplicatesRepeatedSdkMessageIds() {
        assertEquals(listOf("A", "B"), unseen(emptyList(), listOf(Event("A"), Event("A"), Event("B"))).map { it.id })
    }

    @Test fun distinctIdsWithIdenticalBodyAndTimestampRemainSeparate() {
        val page = listOf(Event("A"), Event("B"))
        assertEquals(page, unseen(emptyList(), page))
    }

    @Test fun oppositeDirectionsWithTheSameIdRemainSeparate() {
        assertEquals(listOf(Event("A", true)), unseen(listOf(Event("A")), listOf(Event("A", true))))
    }

    @Test fun blankAndMissingIdsIncludingNonMessageEventsAreRetained() {
        val page = listOf(Event(null), Event(null), Event(""), Event(" "))
        assertEquals(page, unseen(listOf(Event(null)), page))
    }

    @Test fun onlyOnePageCanBeQueuedAndOldFinishCannotReleaseANewRoomPage() {
        val gate = ConversationHistoryPageGate()
        val old = gate.begin()!!
        assertNull(gate.begin()); assertTrue(gate.isCurrent(old))
        gate.invalidate(); assertFalse(gate.isCurrent(old))
        val fresh = gate.begin()!!
        gate.finish(old)
        assertTrue(gate.isCurrent(fresh)); assertNull(gate.begin())
        gate.finish(fresh); assertNotNull(gate.begin())
    }

    @Test fun roomOrLifecycleInvalidationAfterQueryPreventsStaleCommit() {
        val gate = ConversationHistoryPageGate()
        val ticket = gate.begin()!!
        gate.invalidate()
        assertFalse(gate.isCurrent(ticket))
    }
}
