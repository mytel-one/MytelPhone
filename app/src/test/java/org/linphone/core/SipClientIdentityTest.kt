package org.linphone.core

import java.io.ByteArrayInputStream
import java.io.File
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class SipClientIdentityTest {
    @get:Rule val temporaryFolder = TemporaryFolder()

    @Test
    fun importsAClientKeyAndRejectsAnIncorrectPassword() {
        val file = File(temporaryFolder.root, "client.p12")
        generateCertificate(file, "rsa", "clientAuth")
        val identity = file.inputStream().use {
            SipClientIdentity.fromPkcs12(it, TEST_PASSWORD.toCharArray())
        }
        assertTrue(identity.certificate.startsWith("-----BEGIN CERTIFICATE-----"))
        assertTrue(identity.privateKey.startsWith("-----BEGIN PRIVATE KEY-----"))

        assertThrows(java.io.IOException::class.java) {
            file.inputStream().use {
                SipClientIdentity.fromPkcs12(it, "not-the-password".toCharArray())
            }
        }
    }

    @Test
    fun rejectsAServerCertificateAndAnOversizedFile() {
        val file = File(temporaryFolder.root, "server.p12")
        generateCertificate(file, "ec", "serverAuth")
        assertThrows(IllegalArgumentException::class.java) {
            file.inputStream().use {
                SipClientIdentity.fromPkcs12(it, TEST_PASSWORD.toCharArray())
            }
        }
        assertThrows(IllegalArgumentException::class.java) {
            SipClientIdentity.fromPkcs12(
                ByteArrayInputStream(ByteArray(SipClientIdentity.MAX_BYTES + 1)),
                TEST_PASSWORD.toCharArray()
            )
        }
    }

    private fun generateCertificate(file: File, algorithm: String, usage: String) {
        val executableName = if (File.separatorChar == '\\') "keytool.exe" else "keytool"
        val executable = File(System.getProperty("java.home"), "bin/$executableName")
        val process = ProcessBuilder(
            executable.absolutePath,
            "-genkeypair", "-keyalg", algorithm, "-keystore", file.path,
            "-storetype", "PKCS12", "-storepass", TEST_PASSWORD,
            "-alias", "fixture", "-dname", "CN=Mytel test identity", "-ext", "EKU=$usage",
            "-validity", "2"
        ).redirectErrorStream(true).start()
        val output = process.inputStream.bufferedReader().use { it.readText() }
        check(process.waitFor() == 0) { output }
    }

    private companion object {
        const val TEST_PASSWORD = "test-certificate-fixture"
    }
}
