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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
package org.linphone.ui.assistant.viewmodel

import android.net.Uri
import android.os.Build
import androidx.annotation.UiThread
import androidx.annotation.WorkerThread
import androidx.lifecycle.MediatorLiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import java.util.Locale
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.linphone.LinphoneApplication.Companion.coreContext
import org.linphone.LinphoneApplication.Companion.corePreferences
import org.linphone.R
import org.linphone.compatibility.Compatibility
import org.linphone.core.Account
import org.linphone.core.Address
import org.linphone.core.AuthInfo
import org.linphone.core.Core
import org.linphone.core.CoreListenerStub
import org.linphone.core.Factory
import org.linphone.core.Reason
import org.linphone.core.RegistrationState
import org.linphone.core.SipClientIdentity
import org.linphone.core.TransportType
import org.linphone.core.tools.Log
import org.linphone.ui.GenericViewModel
import org.linphone.ui.assistant.SipSetupTransport
import org.linphone.ui.assistant.SipSetupFields
import org.linphone.ui.assistant.SipSetupServerUri
import org.linphone.ui.assistant.SipLocalNetworkPolicy
import org.linphone.ui.assistant.SipLoginLanGate
import org.linphone.utils.AppUtils
import org.linphone.utils.Event
import org.linphone.utils.SipLoginAttempt
import org.linphone.utils.SipLoginAuthTransaction

class ThirdPartySipAccountLoginViewModel
    @UiThread
    constructor() : GenericViewModel() {
    companion object {
        private const val TAG = "[Third Party SIP Account Login ViewModel]"
        private const val REGISTRATION_TIMEOUT_MS = 20_000L
    }

    private var authTransaction: SipLoginAuthTransaction<AuthInfo>? = null

    val username = MutableLiveData<String>()

    val authId = MutableLiveData<String>()

    val password = MutableLiveData<String>()

    val domain = MutableLiveData<String>()

    val displayName = MutableLiveData<String>()

    val transport = MutableLiveData<String>()

    val internationalPrefix = MutableLiveData<String>()

    val internationalPrefixIsoCountryCode = MutableLiveData<String>()

    val showPassword = MutableLiveData<Boolean>()

    val expandAdvancedSettings = MutableLiveData<Boolean>()

    val proxy = MutableLiveData<String>()

    val outboundProxy = MutableLiveData<String>()

    val loginEnabled = MediatorLiveData<Boolean>()

    val registrationInProgress = MutableLiveData<Boolean>()

    val readingClientCertificate = MutableLiveData(false)

    val clientCertificateSelected = MutableLiveData(false)

    private var clientIdentity: SipClientIdentity? = null

    val connectionFeedback = MutableLiveData("")

    val localNetworkSettingsAvailable = MutableLiveData(false)

    val localNetworkPreflightEvent = MutableLiveData<Event<String>>()

    private val localNetworkGate = SipLoginLanGate()
    private var optionalLocalNetworkHint = false
    private var localNetworkSettingsHint = false
    // Memory only, never SavedState/Bundle/Event: the exact Connect intent must
    // retain its validated routes while an OS permission dialog is displayed.
    private data class LoginInput(
        val username: String, val authId: String, val password: String, val server: String,
        val displayName: String, val outbound: String, val proxy: String, val transport: String,
        val prefix: String, val prefixIso: String, val clientIdentity: SipClientIdentity?
    ) {
        val routes: List<String> get() = listOf(server, outbound, proxy)
    }
    private var pendingLoginInput: LoginInput? = null

    val accountLoggedInEvent: MutableLiveData<Event<Boolean>> by lazy {
        MutableLiveData()
    }

    val accountLoginErrorEvent: MutableLiveData<Event<String>> by lazy {
        MutableLiveData()
    }

    val defaultTransportIndexEvent: MutableLiveData<Event<Int>> by lazy {
        MutableLiveData()
    }

    val availableTransports = arrayListOf<String>()

    private val attempts = SipLoginAttempt()
    // These SDK objects are read and released only on the core thread.
    private var pendingAttempt: Long? = null
    private var newlyCreatedAuthInfo: AuthInfo? = null
    private var newlyCreatedAccount: Account? = null
    private var installedClientIdentity = false

    private val coreListener = object : CoreListenerStub() {
        @WorkerThread
        override fun onAccountRegistrationStateChanged(
            core: Core,
            account: Account,
            state: RegistrationState?,
            message: String
        ) {
            val attempt = pendingAttempt ?: return
            if (account.nativePointer == newlyCreatedAccount?.nativePointer && attempts.isCurrent(attempt)) {
                Log.i("$TAG Newly created account registration state is [$state] ($message)")

                if (state == RegistrationState.Ok) {
                    if (!attempts.finish(attempt)) return
                    registrationInProgress.postValue(false)
                    core.defaultAccount = account
                    releasePendingAccount(core, attempt, removeAccount = false)
                    coreContext.postOnMainThread { clearSelectedClientCertificate() }
                    accountLoggedInEvent.postValue(Event(true))
                } else if (state == RegistrationState.Failed) {
                    val error = when (account.error) {
                        Reason.Forbidden -> {
                            AppUtils.getString(R.string.assistant_account_login_forbidden_error)
                        }
                        else -> {
                            AppUtils.getFormattedString(
                                R.string.assistant_account_login_error,
                                account.error.toString()
                            )
                        }
                    }
                    failLogin(core, attempt, error)
                }
            }
        }
    }

    init {
        showPassword.value = false
        expandAdvancedSettings.value = false
        registrationInProgress.value = false

        loginEnabled.addSource(username) {
            loginEnabled.value = isLoginButtonEnabled()
        }
        loginEnabled.addSource(domain) {
            loginEnabled.value = isLoginButtonEnabled()
        }

        // TODO: handle formatting errors ?

        availableTransports.add(AppUtils.getString(R.string.mytel_ux_transport_automatic))
        availableTransports.add(TransportType.Udp.name.uppercase(Locale.getDefault()))
        availableTransports.add(TransportType.Tcp.name.uppercase(Locale.getDefault()))
        availableTransports.add(TransportType.Tls.name.uppercase(Locale.getDefault()))

        coreContext.postOnCoreThread { core ->
            domain.postValue(corePreferences.thirdPartySipAccountDefaultDomain)

            // Respect an administrator-provided default. An unconfigured fresh
            // account starts in Automatic, instead of silently forcing TLS.
            val defaultTransport = core.config.getString(
                "ui", "assistant_third_party_sip_account_transport", ""
            ).orEmpty().uppercase(Locale.ROOT)
            val index = availableTransports.indexOf(defaultTransport).coerceAtLeast(0)
            // Advanced controls are initially hidden, so their first layout
            // cannot be relied on to initialize the effective transport.
            transport.postValue(availableTransports[index])
            defaultTransportIndexEvent.postValue(Event(index.coerceAtLeast(0)))
        }
    }

    @UiThread
    fun login() {
        if (registrationInProgress.value == true || readingClientCertificate.value == true ||
            localNetworkGate.current() != null
        ) return
        localNetworkSettingsAvailable.value = false
        localNetworkSettingsHint = false
        if (!coreContext.isReady()) return
        if (!isLoginButtonEnabled()) {
            connectionFeedback.value = AppUtils.getString(R.string.mytel_ux_required_fields)
            return
        }
        val input = LoginInput(
            username.value.orEmpty().trim(), authId.value.orEmpty().trim(), password.value.orEmpty(),
            domain.value.orEmpty().trim(), displayName.value.orEmpty().trim(), outboundProxy.value.orEmpty().trim(),
            proxy.value.orEmpty().trim(), transport.value.orEmpty(), internationalPrefix.value.orEmpty().trim(),
            internationalPrefixIsoCountryCode.value.orEmpty(), clientIdentity
        )
        val routes = input.routes
        val need = SipLocalNetworkPolicy.routes(routes)
        if (need == SipLocalNetworkPolicy.Need.INVALID) {
            connectionFeedback.value = AppUtils.getString(R.string.assistant_login_cant_parse_address_toast)
            return
        }
        val token = localNetworkGate.begin(need) ?: return
        pendingLoginInput = input
        registrationInProgress.value = true
        connectionFeedback.value = ""
        localNetworkSettingsAvailable.value = false
        localNetworkSettingsHint = false
        val selection = input.transport
        // Validate SDK URI syntax without config, auth, accounts, registration
        // attempts or a timeout. A delayed result cannot revive a cancelled intent.
        coreContext.postOnCoreThread {
            val valid = runCatching {
                routes.filter { it.isNotBlank() }.all {
                    createServerAddress(it, selection, routes)?.let { address ->
                        !address.domain.isNullOrBlank() && address.port in 0..65535
                    } == true
                }
            }.getOrDefault(false)
            coreContext.postOnMainThread {
                if (!localNetworkGate.isCurrent(token)) return@postOnMainThread
                if (!valid) {
                    localNetworkGate.cancel()
                    pendingLoginInput = null
                    registrationInProgress.value = false
                    connectionFeedback.value = AppUtils.getString(R.string.assistant_login_cant_parse_address_toast)
                    return@postOnMainThread
                }
                localNetworkGate.validated(
                    token, Build.VERSION.SDK_INT >= Build.VERSION_CODES.CINNAMON_BUN,
                    Compatibility.isAccessLocalNetworkPermissionGranted(coreContext.context)
                )
                localNetworkPreflightEvent.value = Event(token)
            }
        }
    }

    @UiThread
    fun pendingLocalNetworkToken(): String? = localNetworkGate.current()

    @UiThread
    fun claimLocalNetworkPermissionLaunch(token: String): Boolean = localNetworkGate.claimLaunch(token)

    @UiThread
    fun localNetworkPermissionResult(token: String, granted: Boolean, settingsHint: Boolean): Boolean {
        val accepted = localNetworkGate.result(token, granted)
        if (accepted) localNetworkSettingsHint = !granted && settingsHint
        return accepted
    }

    @UiThread
    fun continueLocalNetworkPreflight(token: String, activeContext: Boolean): SipLoginLanGate.Action {
        val action = localNetworkGate.action(token, activeContext)
        when (action) {
            SipLoginLanGate.Action.START, SipLoginLanGate.Action.START_WITH_HINT -> {
                optionalLocalNetworkHint = action == SipLoginLanGate.Action.START_WITH_HINT
                val input = pendingLoginInput
                pendingLoginInput = null
                if (input != null) startSdkLogin(input) else registrationInProgress.value = false
            }
            SipLoginLanGate.Action.DENIED -> {
                pendingLoginInput = null
                registrationInProgress.value = false
                localNetworkSettingsAvailable.value = localNetworkSettingsHint
                connectionFeedback.value = AppUtils.getString(
                    if (localNetworkSettingsHint) R.string.mytel_lan_setup_settings else R.string.mytel_lan_setup_denied
                )
            }
            else -> Unit
        }
        return action
    }

    @UiThread
    private fun startSdkLogin(input: LoginInput) {
        if (!coreContext.isReady()) {
            registrationInProgress.value = false
            return
        }
        if (!SipSetupFields.canConnect(input.username, input.server)) {
            registrationInProgress.value = false
            connectionFeedback.value = AppUtils.getString(R.string.mytel_ux_required_fields)
            return
        }
        val attempt = attempts.begin() ?: return
        // Reserve the attempt synchronously, before queueing SDK work, so two
        // taps in one frame cannot create two accounts or replace credentials.
        registrationInProgress.value = true
        connectionFeedback.value = ""
        val usernameValue = input.username
        val authIdValue = input.authId
        val passwordValue = input.password
        val serverValue = input.server
        val displayNameValue = input.displayName
        val outboundValue = input.outbound
        val proxyValue = input.proxy
        val transportSelection = input.transport
        val accountRoutes = listOf(serverValue, outboundValue, proxyValue)
        val prefixValue = input.prefix
        val prefixIsoValue = input.prefixIso
        coreContext.postOnCoreThread { core ->
            if (!attempts.isCurrent(attempt)) return@postOnCoreThread
            try {
                core.loadConfigFromXml(corePreferences.thirdPartyDefaultValuesPath)

                val domainValue = serverValue
                val domainAddress = createServerAddress(domainValue, transportSelection, accountRoutes)
                if (domainAddress == null || domainAddress.domain.isNullOrBlank()) {
                    failLogin(core, attempt, AppUtils.getString(R.string.assistant_login_cant_parse_address_toast))
                    return@postOnCoreThread
                }
                val port = domainAddress.port
                if (port > 0) {
                    Log.i("$TAG Server port is retained on the registration route only")
                }
                val domain = domainAddress.domain.orEmpty()

                // Allow to enter SIP identity instead of simply username
                // in case identity domain doesn't match proxy domain
                var user = usernameValue
                if (user.startsWith("sip:", ignoreCase = true)) {
                    user = user.substring("sip:".length)
                } else if (user.startsWith("sips:", ignoreCase = true)) {
                    user = user.substring("sips:".length)
                }
                if (user.contains("@")) {
                    user = user.split("@")[0]
                }

                val userId = authIdValue

                // A fresh template retains the existing port-free sip identity policy.
                // SDK setters serialize IPv6 literals and escape usernames correctly.
                val identityAddress = Factory.instance().createAddress("sip:placeholder@identity.invalid")
                if (identityAddress == null ||
                    identityAddress.setDomain(domain) != 0 ||
                    identityAddress.setUsername(user) != 0
                ) {
                    Log.e("$TAG Can't construct SIP identity")
                    failLogin(core, attempt, AppUtils.getString(R.string.assistant_login_cant_parse_address_toast))
                    return@postOnCoreThread
                }
                Log.i("$TAG SIP identity constructed")

                val accounts = core.accountList
                val found = accounts.find {
                    it.params.identityAddress?.weakEqual(identityAddress) == true
                }
                if (found != null) {
                    Log.w("$TAG An account with the same SIP identity already exists")
                    failLogin(core, attempt, AppUtils.getString(R.string.assistant_account_login_already_connected_error))
                    return@postOnCoreThread
                }

                newlyCreatedAuthInfo = Factory.instance().createAuthInfo(
                    user,
                    userId,
                    passwordValue,
                    null,
                    null,
                    domainAddress.domain ?: domainValue
                )

                val accountParams = core.createAccountParams()

                if (displayNameValue.isNotEmpty()) {
                    identityAddress.displayName = displayNameValue
                }
                accountParams.identityAddress = identityAddress

                val outboundProxyValue = outboundValue
                val outboundProxyAddress = if (outboundProxyValue.isNotEmpty()) {
                    createServerAddress(outboundProxyValue, transportSelection, accountRoutes)
                } else {
                    null
                }
                if (outboundProxyAddress != null) {
                    Log.i("$TAG Outbound proxy address constructed")
                    accountParams.setRoutesAddresses(arrayOf(outboundProxyAddress))
                }

                val proxyServerValue = proxyValue
                val proxyServerAddress = if (proxyServerValue.isNotEmpty()) {
                    createServerAddress(proxyServerValue, transportSelection, accountRoutes)
                } else {
                    outboundProxyAddress ?: domainAddress
                }
                if ((outboundProxyValue.isNotEmpty() && outboundProxyAddress == null) || proxyServerAddress == null) {
                    newlyCreatedAuthInfo = null
                    failLogin(core, attempt, AppUtils.getString(R.string.assistant_login_cant_parse_address_toast))
                    return@postOnCoreThread
                }
                Log.i("$TAG Registration server address constructed")
                accountParams.serverAddress = proxyServerAddress
                Log.i("$TAG Is outbound proxy enabled ? [${accountParams.isOutboundProxyEnabled}]")

                val prefix = prefixValue
                val isoCountryCode = prefixIsoValue
                if (prefix.isNotEmpty()) {
                    val prefixDigits = if (prefix.startsWith("+")) {
                        prefix.substring(1)
                    } else {
                        prefix
                    }
                    if (prefixDigits.isNotEmpty()) {
                        Log.i(
                            "$TAG Setting international prefix [$prefixDigits]($isoCountryCode) in account params"
                        )
                        accountParams.internationalPrefix = prefixDigits
                        accountParams.internationalPrefixIsoCountryCode = isoCountryCode
                    }
                }

                newlyCreatedAccount = core.createAccount(accountParams)
                pendingAttempt = attempt
                core.addListener(coreListener)
                newlyCreatedAuthInfo?.let { auth ->
                    authTransaction = SipLoginAuthTransaction(
                        current = { core.authInfoList.toList() },
                        add = { core.addAuthInfo(it) },
                        remove = { core.removeAuthInfo(it) },
                        pointer = { it.nativePointer },
                        sameIdentity = { left, right ->
                            left.isEqualButAlgorithms(right) &&
                                (left.accessToken != null) == (right.accessToken != null)
                        }
                    ).also { it.begin(auth) }
                }
                val created = requireNotNull(newlyCreatedAccount)
                if (input.clientIdentity != null) {
                    if (!coreContext.sipClientCertificates.status(core, created).eligible) {
                        failLogin(core, attempt, AppUtils.getString(R.string.mytel_client_certificate_ineligible))
                        return@postOnCoreThread
                    }
                    coreContext.sipClientCertificates.install(core, created, input.clientIdentity)
                    installedClientIdentity = true
                }
                core.addAccount(created)
                coreContext.postOnCoreThreadDelayed({ currentCore ->
                    failLogin(currentCore, attempt, AppUtils.getString(R.string.mytel_login_timeout))
                }, REGISTRATION_TIMEOUT_MS)
            } catch (error: Exception) {
                // Exception text from transports can include credential-bearing
                // URIs. Report a useful category without logging their contents.
                Log.e("$TAG Login setup failed (${error.javaClass.simpleName})")
                failLogin(core, attempt, AppUtils.getString(R.string.mytel_login_setup_failed))
            }
        }
    }

    @WorkerThread
    private fun createServerAddress(value: String, selection: String, accountRoutes: List<String>): Address? {
        val uri = SipSetupServerUri.normalize(value) ?: return null
        val address = Factory.instance().createAddress(uri) ?: return null
        if (address.domain.isNullOrBlank()) return null
        val selectedTransport = when (SipSetupTransport.resolveRoute(uri, selection, accountRoutes)) {
            "TCP" -> TransportType.Tcp
            "TLS" -> TransportType.Tls
            else -> TransportType.Udp
        }
        val secure = uri.startsWith("sips:")
        // The native SDK treats URI parameter names case-sensitively.
        SipSetupTransport.transportParameterNames(uri).forEach { address.removeUriParam(it) }
        address.transport = if (secure) TransportType.Tls else selectedTransport
        if (secure) address.setUriParam("transport", "tls")
        return address
    }

    @WorkerThread
    private fun failLogin(core: Core, attempt: Long, message: String) {
        if (!attempts.finish(attempt)) return
        releasePendingAccount(core, attempt, removeAccount = true)
        registrationInProgress.postValue(false)
        val feedback = if (optionalLocalNetworkHint) "$message\n${AppUtils.getString(R.string.mytel_lan_setup_optional_hint)}" else message
        connectionFeedback.postValue(feedback)
        accountLoginErrorEvent.postValue(Event(feedback))
    }

    @WorkerThread
    private fun releasePendingAccount(core: Core, attempt: Long, removeAccount: Boolean) {
        if (pendingAttempt != attempt) return
        core.removeListener(coreListener)
        if (removeAccount) {
            newlyCreatedAccount?.let {
                if (installedClientIdentity) {
                    runCatching { coreContext.sipClientCertificates.remove(it) }.onFailure {
                        Log.w("$TAG Could not clear the certificate from the cancelled account")
                    }
                }
                if (core.accountList.any { current -> current.nativePointer == it.nativePointer }) {
                    core.removeAccount(it)
                }
            }
            authTransaction?.rollback()
        } else {
            authTransaction?.commit()
        }
        authTransaction = null
        newlyCreatedAccount = null
        newlyCreatedAuthInfo = null
        pendingAttempt = null
        installedClientIdentity = false
    }

    @UiThread
    fun selectClientCertificate(uri: Uri, password: CharArray) {
        readingClientCertificate.value = true
        viewModelScope.launch {
            try {
                clientIdentity = withContext(Dispatchers.IO) {
                    SipClientIdentity.fromDocument(coreContext.context.contentResolver, uri, password)
                }
                clientCertificateSelected.value = true
            } catch (exception: CancellationException) {
                throw exception
            } catch (_: Exception) {
                showRedToast(R.string.mytel_client_certificate_import_failed, R.drawable.warning_circle)
            } finally {
                password.fill('\u0000')
                readingClientCertificate.value = false
            }
        }
    }

    @UiThread
    fun clearSelectedClientCertificate() {
        clientIdentity = null
        clientCertificateSelected.value = false
    }

    @UiThread
    fun cancelLogin() {
        val cancelledPreflight = localNetworkGate.cancel()
        pendingLoginInput = null
        localNetworkSettingsAvailable.value = false
        optionalLocalNetworkHint = false
        val attempt = attempts.cancel()
        if (!cancelledPreflight && attempt == null) return
        registrationInProgress.value = false
        connectionFeedback.value = AppUtils.getString(R.string.mytel_ux_connection_cancelled)
        if (attempt != null) coreContext.postOnCoreThread { core ->
            releasePendingAccount(core, attempt, removeAccount = true)
        }
    }

    override fun onCleared() {
        clientIdentity = null
        localNetworkGate.cancel()
        pendingLoginInput = null
        val attempt = attempts.cancel()
        if (attempt != null && coreContext.isReady()) {
            coreContext.postOnCoreThread { core ->
                releasePendingAccount(core, attempt, removeAccount = true)
            }
        }
        super.onCleared()
    }

    @UiThread
    fun toggleShowPassword() {
        showPassword.value = showPassword.value == false
    }

    @UiThread
    private fun isLoginButtonEnabled(): Boolean {
        // Password isn't mandatory as authentication could be Bearer
        return SipSetupFields.canConnect(username.value, domain.value)
    }

    @UiThread
    fun toggleAdvancedSettingsExpand() {
        expandAdvancedSettings.value = expandAdvancedSettings.value == false
    }
}
