package org.linphone.utils

import org.junit.Assert.assertEquals
import org.junit.Test

class SipLoginAuthTransactionTest {
    // JNI wrappers use object identity; repeated native getters can return fresh wrappers.
    private class Auth(val pointer: Long, val identity: String, val password: String, val token: Boolean = false) {
        fun copy(pointer: Long = this.pointer) = Auth(pointer, identity, password, token)
    }

    private class Core {
        val records = mutableListOf<Auth>()
        private var nextPointer = 100L
        fun same(left: Auth, right: Auth) = left.identity == right.identity && left.token == right.token
        fun add(auth: Auth) {
            records.removeAll { same(it, auth) }
            records += auth.copy(pointer = nextPointer++)
        }
        fun transaction() = SipLoginAuthTransaction(
            current = { records.map { it.copy() } },
            add = ::add,
            remove = { auth -> records.removeAll { it.pointer == auth.pointer } },
            pointer = { auth -> auth.pointer },
            sameIdentity = ::same
        )
    }

    @Test fun cancellationRemovesTheCoreCloneInsteadOfTheFactoryObject() {
        val core = Core()
        val original = Auth(1, "201@pbx", "new")
        val transaction = core.transaction()
        transaction.begin(original)
        assertEquals(100L, core.records.single().pointer)
        transaction.rollback()
        assertEquals(emptyList<Auth>(), core.records)
    }

    @Test fun rollbackRestoresReplacedCredentialsAndPreservesUnrelatedAccounts() {
        val core = Core()
        core.add(Auth(1, "201@pbx", "old"))
        core.add(Auth(2, "202@pbx", "unrelated"))
        val unrelated = core.records.last()
        val transaction = core.transaction()
        transaction.begin(Auth(3, "201@pbx", "new"))
        transaction.rollback()
        assertEquals("old", core.records.first { it.identity == "201@pbx" }.password)
        assertEquals(unrelated, core.records.first { it.identity == "202@pbx" })
    }

    @Test fun freshWrappersForTheSameNativePointerStillBelongToTheTransaction() {
        val core = Core()
        val transaction = core.transaction()
        transaction.begin(Auth(1, "201@pbx", "new"))
        core.records[0] = core.records[0].copy()
        transaction.rollback()
        assertEquals(0, core.records.size)
    }

    @Test fun laterIndependentReplacementIsNotRemovedOrOverwritten() {
        val core = Core()
        core.add(Auth(1, "201@pbx", "old"))
        val transaction = core.transaction()
        transaction.begin(Auth(2, "201@pbx", "attempt"))
        core.add(Auth(3, "201@pbx", "later"))
        transaction.rollback()
        assertEquals("later", core.records.single().password)
    }

    @Test fun laterIndependentDeletionIsNotResurrected() {
        val core = Core()
        core.add(Auth(1, "201@pbx", "old"))
        val transaction = core.transaction()
        transaction.begin(Auth(2, "201@pbx", "attempt"))
        core.records.clear()
        transaction.rollback()
        assertEquals(0, core.records.size)
    }

    @Test fun successCommitsNewCredentialsAndDropsRollbackState() {
        val core = Core()
        core.add(Auth(1, "201@pbx", "old"))
        val transaction = core.transaction()
        transaction.begin(Auth(2, "201@pbx", "new"))
        transaction.commit()
        transaction.rollback()
        assertEquals("new", core.records.single().password)
    }

    @Test fun startingAnotherAttemptFirstRollsBackTheAbandonedAttempt() {
        val core = Core()
        val transaction = core.transaction()
        transaction.begin(Auth(1, "201@pbx", "attempt1"))
        transaction.begin(Auth(2, "202@pbx", "attempt2"))
        assertEquals("202@pbx", core.records.single().identity)
        transaction.rollback()
        transaction.rollback()
        assertEquals(0, core.records.size)
    }

    @Test fun oauthAndPasswordSlotsWithSameSipIdentityStayIndependent() {
        val core = Core()
        core.add(Auth(1, "201@pbx", "token", token = true))
        val existing = core.records.single()
        val transaction = core.transaction()
        transaction.begin(Auth(2, "201@pbx", "password"))
        transaction.rollback()
        assertEquals(listOf(existing), core.records)
    }

    @Test fun signedNativePointersRemainValidOwnershipKeys() {
        val records = mutableListOf<Auth>()
        val transaction = SipLoginAuthTransaction(
            current = { records.map { it.copy() } },
            add = { auth: Auth -> records.add(auth.copy(pointer = Long.MIN_VALUE + 17)) },
            remove = { auth -> records.removeAll { it.pointer == auth.pointer } },
            pointer = { auth -> auth.pointer },
            sameIdentity = { left, right -> left.identity == right.identity }
        )
        transaction.begin(Auth(1, "201@pbx", "new"))
        transaction.rollback()
        assertEquals(0, records.size)
    }
}
