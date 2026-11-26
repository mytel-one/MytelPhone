package org.linphone.core

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.AtomicFile
import androidx.annotation.WorkerThread
import java.io.File
import java.security.KeyStore
import java.security.MessageDigest
import java.util.Locale
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import org.json.JSONObject
import org.linphone.core.tools.Log

/**
 * SIP client identities are local to this installation. The SDK only receives inline PEM
 * on the Core thread. Android Keystore encrypts the app's copy at rest.
 */
@WorkerThread
internal class SipClientCertificates(context: Context) {
    private val directory = File(context.noBackupFilesDir, "sip-client-certificates")
    private val attached = mutableMapOf<Long, Target>()

    fun status(core: Core, account: Account): Status {
        val target = target(core, account)
        val identity = account.params.identityAddress?.asStringUriOnly()
        val stored = identity?.let { file(it).baseFile.exists() } == true
        return Status(target != null, stored)
    }

    fun install(core: Core, account: Account, clientIdentity: SipClientIdentity) {
        val target = requireNotNull(target(core, account))
        val payload = JSONObject()
            .put("destination", target.destination)
            .put("certificate", clientIdentity.certificate)
            .put("privateKey", clientIdentity.privateKey)
            .toString()
            .toByteArray(Charsets.UTF_8)
        try {
            val cipher = Cipher.getInstance(TRANSFORMATION)
            cipher.init(Cipher.ENCRYPT_MODE, key(create = true))
            cipher.updateAAD(target.identity.toByteArray(Charsets.UTF_8))
            val encrypted = cipher.doFinal(payload)
            val file = file(target.identity)
            val stream = file.startWrite()
            try {
                stream.write(cipher.iv)
                stream.write(encrypted)
                file.finishWrite(stream)
            } catch (exception: Exception) {
                file.failWrite(stream)
                throw exception
            }
            attach(account, target, clientIdentity)
        } finally {
            payload.fill(0)
        }
    }

    fun remove(account: Account) {
        val identity = account.params.identityAddress?.asStringUriOnly() ?: return
        val file = file(identity)
        file.delete()
        check(!file.baseFile.exists())
        clearBeforeAccountEdit(account)
    }

    fun restore(core: Core) {
        for (account in core.accountList) {
            val target = target(core, account) ?: continue
            val file = file(target.identity)
            if (!file.baseFile.exists()) continue
            try {
                val encrypted = file.readFully()
                check(encrypted.size in (NONCE_BYTES + 16)..MAX_STORED_BYTES)
                val cipher = Cipher.getInstance(TRANSFORMATION)
                cipher.init(
                    Cipher.DECRYPT_MODE,
                    key(create = false),
                    GCMParameterSpec(128, encrypted, 0, NONCE_BYTES)
                )
                cipher.updateAAD(target.identity.toByteArray(Charsets.UTF_8))
                val payload = cipher.doFinal(encrypted, NONCE_BYTES, encrypted.size - NONCE_BYTES)
                try {
                    val parsed = JSONObject(payload.toString(Charsets.UTF_8))
                    if (parsed.getString("destination") == target.destination) {
                        attach(
                            account,
                            target,
                            SipClientIdentity(parsed.getString("certificate"), parsed.getString("privateKey"))
                        )
                    }
                } finally {
                    payload.fill(0)
                }
            } catch (_: Exception) {
                Log.w("[SIP Client Certificates] Could not open the local client identity")
            }
        }
    }

    fun clearChangedAccounts(core: Core) {
        val iterator = attached.iterator()
        while (iterator.hasNext()) {
            val (pointer, previous) = iterator.next()
            val account = core.accountList.firstOrNull { it.nativePointer == pointer }
            val target = account?.let { target(core, it) }
            if (target == null || target.identity != previous.identity || target.destination != previous.destination ||
                target.auth.nativePointer != previous.auth.nativePointer
            ) {
                clear(previous.auth)
                iterator.remove()
            }
        }
    }

    fun clearBeforeAccountEdit(account: Account) {
        attached.remove(account.nativePointer)?.let { clear(it.auth) }
    }

    private fun attach(account: Account, target: Target, identity: SipClientIdentity) {
        target.auth.tlsCert = identity.certificate
        target.auth.tlsKey = identity.privateKey
        attached[account.nativePointer] = target
    }

    private fun clear(auth: AuthInfo) {
        auth.tlsCert = null
        auth.tlsKey = null
    }

    private fun target(core: Core, account: Account): Target? {
        val identity = account.params.identityAddress?.asStringUriOnly() ?: return null
        val address = outboundAddress(account) ?: return null
        if (address.transport != TransportType.Tls && !address.secure) return null
        val host = address.domain?.lowercase(Locale.ROOT) ?: return null
        val auth = account.findAuthInfo() ?: return null
        if (!host.equals(auth.domain, ignoreCase = true)) return null
        if (core.accountList.any {
                it.nativePointer != account.nativePointer && (
                    host.equals(outboundAddress(it)?.domain, ignoreCase = true) ||
                        host.equals(it.params.identityAddress?.domain, ignoreCase = true)
                    )
            }
        ) return null

        val port = address.port.takeIf { it > 0 } ?: 5061
        return Target(identity, "$host:$port", auth)
    }

    private fun outboundAddress(account: Account): Address? {
        val params = account.params
        return params.routesAddresses.firstOrNull() ?: params.serverAddress
    }

    private fun file(identity: String): AtomicFile {
        val digest = MessageDigest.getInstance("SHA-256").digest(identity.toByteArray(Charsets.UTF_8))
        return AtomicFile(File(directory, digest.joinToString("") { "%02x".format(it) }))
    }

    private fun key(create: Boolean): SecretKey {
        val store = KeyStore.getInstance("AndroidKeyStore").apply { load(null) }
        val existing = store.getKey(ALIAS, null) as? SecretKey
        if (existing != null) return existing
        check(create)
        val spec = KeyGenParameterSpec.Builder(
            ALIAS,
            KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
        )
            .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
            .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
            .setKeySize(256)
            .build()
        return KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore").run {
            init(spec)
            generateKey()
        }
    }

    data class Status(val eligible: Boolean, val stored: Boolean)

    private data class Target(val identity: String, val destination: String, val auth: AuthInfo)

    private companion object {
        const val ALIAS = "mytel-sip-client-identity-v1"
        const val TRANSFORMATION = "AES/GCM/NoPadding"
        const val NONCE_BYTES = 12
        const val MAX_STORED_BYTES = 512 * 1024
    }
}
