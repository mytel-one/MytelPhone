package org.linphone.core

import android.content.ContentResolver
import android.net.Uri
import java.io.ByteArrayInputStream
import java.io.InputStream
import java.security.KeyStore
import java.security.PrivateKey
import java.security.SecureRandom
import java.security.Signature
import java.security.cert.X509Certificate
import kotlin.io.encoding.Base64
import org.bouncycastle.jce.provider.BouncyCastleProvider

internal class SipClientIdentity(val certificate: String, val privateKey: String) {
    companion object {
        const val MAX_BYTES = 128 * 1024

        // The Android 12 platform provider cannot read all modern PBES2
        // packages. Keep this provider local to SIP certificate import.
        private val pkcs12Provider by lazy { BouncyCastleProvider() }

        fun fromDocument(resolver: ContentResolver, uri: Uri, password: CharArray): SipClientIdentity {
            try {
                return requireNotNull(resolver.openInputStream(uri)).use { fromPkcs12(it, password) }
            } finally {
                password.fill('\u0000')
            }
        }

        fun fromPkcs12(input: InputStream, password: CharArray): SipClientIdentity {
            val bytes = ByteArray(MAX_BYTES + 1)
            try {
                var length = 0
                while (length < bytes.size) {
                    val count = input.read(bytes, length, bytes.size - length)
                    if (count < 0) break
                    if (count == 0) {
                        val byte = input.read()
                        if (byte < 0) break
                        bytes[length++] = byte.toByte()
                    } else {
                        length += count
                    }
                }
                require(length in 1..MAX_BYTES)

                val store = KeyStore.getInstance("PKCS12", pkcs12Provider)
                store.load(ByteArrayInputStream(bytes, 0, length), password)
                val alias = store.aliases().toList().filter(store::isKeyEntry).single()
                val key = store.getKey(alias, password) as PrivateKey
                val chain = store.getCertificateChain(alias).map { it as X509Certificate }
                require(chain.isNotEmpty())
                val leaf = chain.first()
                leaf.checkValidity()
                require(leaf.basicConstraints == -1)
                require(leaf.keyUsage?.getOrNull(0) != false)
                require(leaf.extendedKeyUsage?.contains("1.3.6.1.5.5.7.3.2") != false)

                val algorithm = when (key.algorithm) {
                    "RSA" -> "SHA256withRSA"
                    "EC", "ECDSA" -> "SHA256withECDSA"
                    else -> throw IllegalArgumentException("Unsupported client key type")
                }
                val challenge = ByteArray(32).also(SecureRandom()::nextBytes)
                val signer = Signature.getInstance(algorithm, pkcs12Provider)
                signer.initSign(key)
                signer.update(challenge)
                val signature = signer.sign()
                signer.initVerify(leaf)
                signer.update(challenge)
                require(signer.verify(signature))

                val keyBytes = requireNotNull(key.encoded)
                try {
                    return SipClientIdentity(
                        chain.joinToString("") { pem("CERTIFICATE", it.encoded) },
                        pem("PRIVATE KEY", keyBytes)
                    )
                } finally {
                    keyBytes.fill(0)
                }
            } finally {
                bytes.fill(0)
            }
        }

        private fun pem(type: String, bytes: ByteArray): String {
            val body = Base64.encode(bytes).chunked(64).joinToString("\n")
            return "-----BEGIN $type-----\n$body\n-----END $type-----\n"
        }
    }
}
