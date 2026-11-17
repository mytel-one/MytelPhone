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
package org.linphone.ui.main.help.fragment

import android.content.ActivityNotFoundException
import android.content.Intent
import android.os.Bundle
import android.text.method.LinkMovementMethod
import android.text.util.Linkify
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.annotation.UiThread
import androidx.appcompat.app.AlertDialog
import androidx.core.net.toUri
import androidx.lifecycle.ViewModelProvider
import androidx.navigation.fragment.findNavController
import org.linphone.R
import org.linphone.core.tools.Log
import org.linphone.databinding.HelpFragmentBinding
import org.linphone.ui.GenericActivity
import org.linphone.ui.main.fragment.GenericMainFragment
import org.linphone.ui.main.help.viewmodel.HelpViewModel

@UiThread
class HelpFragment : GenericMainFragment() {
    companion object {
        private const val TAG = "[Help Fragment]"
    }

    private lateinit var binding: HelpFragmentBinding
    private lateinit var viewModel: HelpViewModel

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        binding = HelpFragmentBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding.lifecycleOwner = viewLifecycleOwner
        viewModel = ViewModelProvider(this)[HelpViewModel::class.java]
        binding.viewModel = viewModel
        observeToastEvents(viewModel)

        binding.setBackClickListener { goBack() }
        binding.setWebsiteClickListener {
            openExternalLink(getString(R.string.mytel_official_website_url))
        }
        binding.setAboutClickListener {
            openExternalLink(getString(R.string.mytel_about_company_url))
        }
        binding.setUserGuideClickListener {
            openExternalLink(getString(R.string.website_user_guide_url))
        }
        binding.setSupportClickListener {
            openExternalLink(getString(R.string.website_contact_url))
        }
        binding.setPhoneClickListener {
            openExternalLink(getString(R.string.mytel_support_phone_uri), Intent.ACTION_DIAL)
        }
        binding.setEmailClickListener {
            openExternalLink(getString(R.string.mytel_support_email_uri), Intent.ACTION_SENDTO)
        }
        binding.setLicensesClickListener { showSoftwareLicenses() }
        binding.setDebugClickListener {
            if (findNavController().currentDestination?.id == R.id.helpFragment) {
                val action = HelpFragmentDirections.actionHelpFragmentToDebugFragment()
                findNavController().navigate(action)
            }
        }
    }

    private fun showSoftwareLicenses() {
        val notices = resources.openRawResource(R.raw.mytel_notices)
            .bufferedReader(Charsets.UTF_8).use { it.readText() }
        val license = resources.openRawResource(R.raw.mytel_gpl_v3)
            .bufferedReader(Charsets.UTF_8).use { it.readText() }
        val message = getString(R.string.mytel_software_licenses_intro) +
            "\n\n" + notices + "\n\n" + license
        val dialog = AlertDialog.Builder(requireContext())
            .setTitle(R.string.mytel_software_licenses_title)
            .setMessage(message)
            .setPositiveButton(android.R.string.ok, null)
            .setNeutralButton(R.string.mytel_third_party_notices_title) { _, _ ->
                openExternalLink(getString(R.string.website_open_source_licences_usage_url))
            }
            .create()
        dialog.show()
        dialog.findViewById<TextView>(android.R.id.message)?.let { textView ->
            textView.setTextIsSelectable(true)
            Linkify.addLinks(textView, Linkify.WEB_URLS)
            textView.movementMethod = LinkMovementMethod.getInstance()
        }
    }

    private fun openExternalLink(url: String, action: String = Intent.ACTION_VIEW) {
        try {
            startActivity(Intent(action, url.toUri()))
        } catch (error: ActivityNotFoundException) {
            Log.w("$TAG No application is available for this MyTel contact action: $error")
            (activity as? GenericActivity)?.showRedToast(
                getString(R.string.mytel_contact_action_unavailable),
                R.drawable.warning_circle
            )
        } catch (error: SecurityException) {
            Log.w("$TAG This device does not permit the MyTel contact action: $error")
            (activity as? GenericActivity)?.showRedToast(
                getString(R.string.mytel_contact_action_unavailable),
                R.drawable.warning_circle
            )
        } catch (error: IllegalStateException) {
            Log.w("$TAG Cannot open this MyTel contact action while the screen is closing: $error")
        }
    }
}
