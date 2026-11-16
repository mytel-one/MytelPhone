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
package org.linphone.ui.welcome

import android.content.Intent
import android.os.Bundle
import android.view.View
import androidx.activity.addCallback
import androidx.activity.enableEdgeToEdge
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import androidx.databinding.DataBindingUtil
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter
import org.linphone.R
import org.linphone.databinding.WelcomeActivityBinding
import org.linphone.ui.GenericActivity
import org.linphone.ui.assistant.AssistantActivity
import org.linphone.ui.welcome.fragment.WelcomePage1Fragment

/** A single, skippable introduction before the usable account setup screen. */
class WelcomeActivity : GenericActivity() {
    private lateinit var binding: WelcomeActivityBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        onBackPressedDispatcher.addCallback { finish() }
        binding = DataBindingUtil.setContentView(this, R.layout.welcome_activity)
        binding.lifecycleOwner = this
        ViewCompat.setOnApplyWindowInsetsListener(binding.root) { view, insets ->
            val bars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            view.updatePadding(bars.left, bars.top, bars.right, bars.bottom)
            WindowInsetsCompat.CONSUMED
        }
        binding.pager.adapter = WelcomeAdapter(this)
        binding.skip.visibility = View.GONE
        binding.dotsIndicator.visibility = View.GONE
        binding.setSkipClickListener { goToAssistant() }
        binding.setNextClickListener { goToAssistant() }
    }

    private fun goToAssistant() {
        // Background access is explained after connection and in Settings;
        // account setup never opens an unrelated battery permission dialog.
        startActivity(Intent(this, AssistantActivity::class.java).apply {
            putExtra(AssistantActivity.SKIP_LANDING_EXTRA, true)
        })
        finish()
    }

    private class WelcomeAdapter(activity: FragmentActivity) : FragmentStateAdapter(activity) {
        override fun getItemCount(): Int = 1
        override fun createFragment(position: Int): Fragment = WelcomePage1Fragment()
    }
}
