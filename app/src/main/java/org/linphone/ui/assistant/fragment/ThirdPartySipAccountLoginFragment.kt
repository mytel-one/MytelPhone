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
package org.linphone.ui.assistant.fragment

import android.Manifest
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Build
import android.graphics.Rect
import android.telephony.TelephonyManager
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.AdapterView
import android.widget.ArrayAdapter
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.OnBackPressedCallback
import androidx.annotation.UiThread
import androidx.core.view.ViewCompat
import androidx.core.app.ActivityCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.doOnLayout
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.Lifecycle
import androidx.navigation.fragment.findNavController
import androidx.navigation.navGraphViewModels
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.linphone.LinphoneApplication.Companion.coreContext
import org.linphone.R
import org.linphone.compatibility.Compatibility
import org.linphone.core.tools.Log
import org.linphone.databinding.AssistantThirdPartySipAccountLoginFragmentBinding
import org.linphone.ui.GenericActivity
import org.linphone.ui.GenericFragment
import org.linphone.ui.assistant.viewmodel.ThirdPartySipAccountLoginViewModel
import org.linphone.ui.assistant.SipLoginLanGate
import org.linphone.ui.sso.SingleSignOnActivity
import org.linphone.utils.DialogUtils
import org.linphone.utils.PhoneNumberUtils
import org.linphone.utils.MyTelPermissionRequestHistory
import org.linphone.utils.MyTelPermissionRequestPolicy

@UiThread
class ThirdPartySipAccountLoginFragment : GenericFragment() {
    companion object {
        private const val TAG = "[Third Party SIP Account Login Fragment]"
    }

    private lateinit var binding: AssistantThirdPartySipAccountLoginFragmentBinding

    private val viewModel: ThirdPartySipAccountLoginViewModel by navGraphViewModels(
        R.id.assistant_nav_graph
    )

    private val chooseClientIdentity = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri != null) {
            DialogUtils.showSipClientCertificatePassword(requireContext()) {
                viewModel.selectClientCertificate(uri, it)
            }
        }
    }

    private val dropdownListener = object : AdapterView.OnItemSelectedListener {
        override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
            val transport = viewModel.availableTransports[position]
            Log.i("$TAG Selected transport updated [$transport]")
            viewModel.transport.value = transport
        }

        override fun onNothingSelected(parent: AdapterView<*>?) {
        }
    }

    private lateinit var adapter: ArrayAdapter<String>

    private var launchedLocalNetworkToken: String? = null

    private val accessLocalNetworkPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        val token = launchedLocalNetworkToken ?: return@registerForActivityResult
        launchedLocalNetworkToken = null
        if (viewModel.pendingLocalNetworkToken() != token) return@registerForActivityResult
        val settingsHint = !isGranted && activity?.let {
            !ActivityCompat.shouldShowRequestPermissionRationale(it, Manifest.permission.ACCESS_LOCAL_NETWORK)
        } == true
        if (viewModel.localNetworkPermissionResult(token, isGranted, settingsHint)) resumeLocalNetworkConnect()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        launchedLocalNetworkToken = savedInstanceState?.getString("mytel_setup_lan_request")
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putString("mytel_setup_lan_request", launchedLocalNetworkToken)
        super.onSaveInstanceState(outState)
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        binding = AssistantThirdPartySipAccountLoginFragmentBinding.inflate(layoutInflater)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.lifecycleOwner = viewLifecycleOwner

        adapter = ArrayAdapter(
            requireContext(),
            R.layout.drop_down_item,
            viewModel.availableTransports
        )
        adapter.setDropDownViewResource(R.layout.generic_dropdown_cell)
        binding.transport.adapter = adapter
        binding.transport.onItemSelectedListener = dropdownListener

        binding.viewModel = viewModel
        observeToastEvents(viewModel)

        binding.importClientCertificate.setOnClickListener {
            chooseClientIdentity.launch(arrayOf("*/*"))
        }
        binding.removeClientCertificate.setOnClickListener {
            viewModel.clearSelectedClientCertificate()
        }

        binding.domain.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                if (viewModel.loginEnabled.value == true && viewModel.registrationInProgress.value != true) {
                    ViewCompat.getWindowInsetsController(binding.root)?.hide(WindowInsetsCompat.Type.ime())
                    viewModel.login()
                }
                true
            } else {
                false
            }
        }

        viewModel.registrationInProgress.observe(viewLifecycleOwner) { connecting ->
            if (connecting == true) {
                ViewCompat.getWindowInsetsController(binding.root)?.hide(WindowInsetsCompat.Type.ime())
                binding.cancelRegistration.post {
                    binding.cancelRegistration.requestFocus()
                    binding.cancelRegistration.requestRectangleOnScreen(
                        Rect(0, 0, binding.cancelRegistration.width, binding.cancelRegistration.height), false
                    )
                }
            }
        }

        viewModel.connectionFeedback.observe(viewLifecycleOwner) { message ->
            val feedbackView = binding.connectionFeedback
            if (!message.isNullOrBlank() && feedbackView != null) {
                binding.executePendingBindings()
                feedbackView.isClickable = viewModel.localNetworkSettingsAvailable.value == true
                feedbackView.isFocusable = feedbackView.isClickable
                feedbackView.doOnLayout { feedback ->
                    feedback.requestRectangleOnScreen(
                        Rect(0, 0, feedback.width, feedback.height), false
                    )
                }
            }
        }
        binding.connectionFeedback?.setOnClickListener {
            if (viewModel.localNetworkSettingsAvailable.value == true && isLoginContextActive()) {
                (requireActivity() as GenericActivity).goToAndroidPermissionSettings()
            }
        }

        viewModel.localNetworkPreflightEvent.observe(viewLifecycleOwner) { event ->
            if (isLoginContextActive()) event.consume { resumeLocalNetworkConnect() }
        }

        binding.setBackClickListener {
            viewModel.cancelLogin()
            goBack()
        }

        requireActivity().onBackPressedDispatcher.addCallback(
            viewLifecycleOwner,
            object : OnBackPressedCallback(true) {
                override fun handleOnBackPressed() {
                    if (viewModel.registrationInProgress.value == true) {
                        viewModel.cancelLogin()
                    } else {
                        isEnabled = false
                        requireActivity().onBackPressedDispatcher.onBackPressed()
                        isEnabled = true
                    }
                }
            }
        )

        binding.setOutboundProxyTooltipClickListener {
            showOutboundProxyInfoDialog()
        }

        binding.setOtherSetupClickListener {
            if (findNavController().currentDestination?.id == R.id.thirdPartySipAccountLoginFragment) {
                viewModel.cancelLogin()
                findNavController().navigate(R.id.landingFragment)
            }
        }

        viewModel.showPassword.observe(viewLifecycleOwner) {
            lifecycleScope.launch {
                delay(50)
                binding.password.setSelection(binding.password.text?.length ?: 0)
            }
        }

        viewModel.accountLoggedInEvent.observe(viewLifecycleOwner) {
            it.consume {
                Log.i("$TAG Account successfully logged-in, leaving assistant")
                requireActivity().finish()
            }
        }

        viewModel.accountLoginErrorEvent.observe(viewLifecycleOwner) {
            it.consume { message ->
                (requireActivity() as GenericActivity).showRedToast(
                    message,
                    R.drawable.warning_circle
                )
            }
        }

        viewModel.defaultTransportIndexEvent.observe(viewLifecycleOwner) {
            it.consume { index ->
                binding.transport.setSelection(index)
            }
        }

        coreContext.bearerAuthenticationRequestedEvent.observe(viewLifecycleOwner) {
            it.consume { pair ->
                val serverUrl = pair.first
                val username = pair.second

                Log.i(
                    "$TAG Bearer auth request, navigating to Single Sign On Fragment with server URL [$serverUrl] and username [$username]"
                )
                val intent = Intent(requireContext(), SingleSignOnActivity::class.java)
                intent.putExtra(SingleSignOnActivity.INTENT_EXTRA_USERNAME, username)
                intent.putExtra(SingleSignOnActivity.INTENT_EXTRA_SERVER_URL, serverUrl)
                startActivity(intent)
            }
        }

        val telephonyManager = requireContext().getSystemService(Context.TELEPHONY_SERVICE) as? TelephonyManager
        val countryIso = telephonyManager?.networkCountryIso.orEmpty()
        coreContext.postOnCoreThread {
            val dialPlan = PhoneNumberUtils.getDeviceDialPlan(countryIso)
            if (dialPlan != null) {
                viewModel.internationalPrefix.postValue(dialPlan.countryCallingCode)
                viewModel.internationalPrefixIsoCountryCode.postValue(dialPlan.isoCountryCode)
            }
        }
    }

    private fun goBack() {
        if (!findNavController().popBackStack()) requireActivity().finish()
    }

    override fun onResume() {
        super.onResume()
        resumeLocalNetworkConnect()
    }

    override fun onStop() {
        cancelLocalConnectIfLeaving()
        super.onStop()
    }

    override fun onDestroyView() {
        cancelLocalConnectIfLeaving()
        super.onDestroyView()
    }

    private fun cancelLocalConnectIfLeaving() {
        val destination = runCatching { findNavController().currentDestination?.id }.getOrNull()
        if (isRemoving || activity?.isFinishing == true ||
            (destination != null && destination != R.id.thirdPartySipAccountLoginFragment)
        ) viewModel.cancelLogin()
    }

    private fun isLoginContextActive(): Boolean = isAdded && view != null && activity?.isFinishing == false &&
        viewLifecycleOwner.lifecycle.currentState.isAtLeast(Lifecycle.State.STARTED) &&
        runCatching { findNavController().currentDestination?.id }.getOrNull() == R.id.thirdPartySipAccountLoginFragment

    private fun resumeLocalNetworkConnect() {
        if (!isLoginContextActive()) return
        val token = viewModel.pendingLocalNetworkToken() ?: return
        if (viewModel.continueLocalNetworkPreflight(token, true) != SipLoginLanGate.Action.REQUEST ||
            !viewModel.claimLocalNetworkPermissionLaunch(token)
        ) return
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.CINNAMON_BUN) {
            viewModel.localNetworkPermissionResult(token, true, false)
            resumeLocalNetworkConnect()
            return
        }
        val permission = Manifest.permission.ACCESS_LOCAL_NETWORK
        when (MyTelPermissionRequestPolicy.resolve(
            Compatibility.isAccessLocalNetworkPermissionGranted(requireContext()),
            MyTelPermissionRequestHistory.wasRequested(requireContext(), permission),
            ActivityCompat.shouldShowRequestPermissionRationale(requireActivity(), permission)
        )) {
            MyTelPermissionRequestPolicy.Action.NONE -> {
                viewModel.localNetworkPermissionResult(token, true, false)
                resumeLocalNetworkConnect()
            }
            MyTelPermissionRequestPolicy.Action.REQUEST -> {
                launchedLocalNetworkToken = token
                MyTelPermissionRequestHistory.record(requireContext(), permission)
                try {
                    accessLocalNetworkPermissionLauncher.launch(permission)
                } catch (_: RuntimeException) {
                    launchedLocalNetworkToken = null
                    viewModel.cancelLogin()
                }
            }
            MyTelPermissionRequestPolicy.Action.SETTINGS -> {
                viewModel.localNetworkPermissionResult(token, false, true)
                resumeLocalNetworkConnect()
            }
        }
    }

    private fun showOutboundProxyInfoDialog() {
        val dialog = DialogUtils.getAccountOutboundProxyHelpDialog(requireActivity())
        dialog.show()
    }
}
