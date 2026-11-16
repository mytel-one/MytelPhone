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
package org.linphone.ui

import android.annotation.SuppressLint
import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Intent
import android.content.res.Configuration
import android.content.res.Resources
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import android.provider.DocumentsContract
import android.view.ViewGroup
import android.view.WindowManager
import androidx.annotation.DrawableRes
import androidx.annotation.MainThread
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.os.LocaleListCompat
import androidx.core.view.children
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.linphone.LinphoneApplication.Companion.corePreferences
import org.linphone.R
import org.linphone.compatibility.Compatibility
import org.linphone.core.tools.Log
import org.linphone.utils.ToastUtils
import org.linphone.utils.FileUtils
import org.linphone.utils.MyTelDocumentExport
import org.linphone.utils.slideInToastFromTop
import org.linphone.utils.slideInToastFromTopForDuration
import androidx.core.net.toUri
import java.io.File

@MainThread
open class GenericActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "[Generic Activity]"
        private const val PENDING_DOCUMENT_EXPORT = "mytel.pending_document_export"
        private const val LEGACY_EXPORT_NOTICE_SHOWN = "mytel.legacy_export_notice_shown"
    }

    private lateinit var toastsArea: ViewGroup

    private var mainColor: String = "orange"

    private var documentExport = MyTelDocumentExport()
    private var legacyExportNoticeShown = false

    private val documentExportLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        val sourcePath = documentExport.takePendingPath() ?: return@registerForActivityResult
        if (result.resultCode != Activity.RESULT_OK) {
            Log.i("$TAG Document export cancelled")
            return@registerForActivityResult
        }
        val destination = result.data?.data
        if (destination == null || destination.scheme != "content" ||
            !DocumentsContract.isDocumentUri(this, destination)
        ) {
            showDocumentExportError()
            return@registerForActivityResult
        }
        lifecycleScope.launch {
            val copied = withContext(Dispatchers.IO) {
                var success = false
                try {
                    success = FileUtils.copyFile(sourcePath.toUri(), destination)
                    success
                } finally {
                    if (!success) {
                        try {
                            if (!DocumentsContract.deleteDocument(contentResolver, destination)) {
                                Log.w("$TAG Provider did not confirm removal of incomplete exported document")
                            }
                        } catch (e: Exception) {
                            Log.e("$TAG Failed to remove incomplete exported document: $e")
                        }
                    }
                }
            }
            if (copied) {
                showGreenToast(getString(R.string.file_successfully_exported_to_documents_toast), R.drawable.check)
            } else {
                showDocumentExportError()
            }
        }
    }

    override fun getTheme(): Resources.Theme {
        mainColor = corePreferences.themeMainColor
        val theme = super.theme
        when (mainColor) {
            "terracotta" -> theme.applyStyle(R.style.Theme_LinphoneTerracotta, true)
            "lavender" -> theme.applyStyle(R.style.Theme_LinphoneLavender, true)
            "honey" -> theme.applyStyle(R.style.Theme_LinphoneHoney, true)
            "burgundy" -> theme.applyStyle(R.style.Theme_LinphoneBurgundy, true)
            "mint" -> theme.applyStyle(R.style.Theme_LinphoneMint, true)
            "coral" -> theme.applyStyle(R.style.Theme_LinphoneCoral, true)
            "plum" -> theme.applyStyle(R.style.Theme_LinphonePlum, true)
            "titanium" -> theme.applyStyle(R.style.Theme_LinphoneTitanium, true)
            "mineral_blue" -> theme.applyStyle(R.style.Theme_LinphoneMineralBlue, true)
            else -> theme.applyStyle(R.style.Theme_Linphone, true)
        }
        return theme
    }

    @SuppressLint("SourceLockedOrientationActivity")
    override fun onCreate(savedInstanceState: Bundle?) {
        enableWindowSecureMode(corePreferences.enableSecureMode)

        val nightMode = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK
        val darkModeEnabled = corePreferences.darkMode
        Log.i(
            "$TAG Theme selected in config file is [${if (darkModeEnabled == -1) "auto" else if (darkModeEnabled == 0) "light" else "dark"}]"
        )
        when (nightMode) {
            Configuration.UI_MODE_NIGHT_NO, Configuration.UI_MODE_NIGHT_UNDEFINED -> {
                if (darkModeEnabled == 1) {
                    Compatibility.forceDarkMode(this)
                }
            }
            Configuration.UI_MODE_NIGHT_YES -> {
                if (darkModeEnabled == 0) {
                    Compatibility.forceLightMode(this)
                }
            }
        }

        super.onCreate(savedInstanceState)

        documentExport = MyTelDocumentExport(savedInstanceState?.getString(PENDING_DOCUMENT_EXPORT))
        legacyExportNoticeShown = savedInstanceState?.getBoolean(LEGACY_EXPORT_NOTICE_SHOWN) ?: false

        // MyTel product requirement: the app UI language is always English,
        // regardless of the device locale (owner order 2026-11-06). Applied
        // idempotently in the shared AppCompat activity lifecycle right after
        // super.onCreate so every entry activity is covered; chat content in
        // any language remains fully functional.
        val english = LocaleListCompat.forLanguageTags("en")
        if (AppCompatDelegate.getApplicationLocales() != english) {
            AppCompatDelegate.setApplicationLocales(english)
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putString(PENDING_DOCUMENT_EXPORT, documentExport.pendingPath)
        outState.putBoolean(LEGACY_EXPORT_NOTICE_SHOWN, legacyExportNoticeShown)
        super.onSaveInstanceState(outState)
    }

    /** Save a user-selected copy without requesting broad storage permissions. */
    fun exportFileToDocuments(filePath: String) {
        val sourceUri = FileUtils.getProperFilePath(filePath).toUri()
        val source = if (sourceUri.scheme == "file") sourceUri.path?.let { File(it) } else null
        if (source == null || !source.isFile || !source.canRead()) {
            Log.e("$TAG Document export source is not a readable file")
            showDocumentExportError()
            return
        }
        if (!documentExport.begin(source.toUri().toString())) {
            Log.w("$TAG A document export picker is already open")
            return
        }
        val name = MyTelDocumentExport.safeFileName(source.name)
        val mime = FileUtils.getMimeTypeFromExtension(FileUtils.getExtensionFromFileName(name))
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = mime.ifBlank { "application/octet-stream" }
            putExtra(Intent.EXTRA_TITLE, name)
        }
        try {
            documentExportLauncher.launch(intent)
        } catch (e: CancellationException) {
            documentExport.takePendingPath()
            throw e
        } catch (e: Exception) {
            documentExport.takePendingPath()
            Log.e("$TAG Document picker could not be opened: $e")
            showDocumentExportError()
        }
    }

    private fun showDocumentExportError() {
        showRedToast(getString(R.string.export_file_to_documents_error_toast), R.drawable.warning_circle)
    }

    fun showLegacyMediaExportGuidance() {
        if (legacyExportNoticeShown) return
        legacyExportNoticeShown = true
        showBlueToast(getString(R.string.mytel_legacy_media_export_manual_save), R.drawable.info)
    }

    protected fun checkMainColorTheme() {
        if (mainColor != corePreferences.themeMainColor) {
            Log.i("$TAG Main color setting has changed, re-creating activity")
            recreate()
        }
    }

    fun setUpToastsArea(viewGroup: ViewGroup) {
        toastsArea = viewGroup
    }

    fun showGreenToast(
        message: String,
        @DrawableRes icon: Int,
        duration: Long = 4000,
        doNotTint: Boolean = false
    ) {
        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                val greenToast = ToastUtils.getGreenToast(
                    this@GenericActivity,
                    toastsArea,
                    message,
                    icon,
                    doNotTint
                )
                toastsArea.addView(greenToast.root)

                greenToast.root.slideInToastFromTopForDuration(
                    toastsArea,
                    lifecycleScope,
                    duration
                )
            }
        }
    }

    fun showBlueToast(
        message: String,
        @DrawableRes icon: Int,
        duration: Long = 4000,
        doNotTint: Boolean = false
    ) {
        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                val blueToast = ToastUtils.getBlueToast(
                    this@GenericActivity,
                    toastsArea,
                    message,
                    icon,
                    doNotTint
                )
                toastsArea.addView(blueToast.root)

                blueToast.root.slideInToastFromTopForDuration(
                    toastsArea,
                    lifecycleScope,
                    duration
                )
            }
        }
    }

    fun showRedToast(
        message: String,
        @DrawableRes icon: Int,
        duration: Long = 4000,
        doNotTint: Boolean = false
    ) {
        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                val redToast = ToastUtils.getRedToast(
                    this@GenericActivity,
                    toastsArea,
                    message,
                    icon,
                    doNotTint
                )
                toastsArea.addView(redToast.root)

                redToast.root.slideInToastFromTopForDuration(
                    toastsArea,
                    lifecycleScope,
                    duration
                )
            }
        }
    }

    fun showPersistentRedToast(
        message: String,
        @DrawableRes icon: Int,
        tag: String,
        doNotTint: Boolean = false
    ) {
        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                val redToast =
                    ToastUtils.getRedToast(
                        this@GenericActivity,
                        toastsArea,
                        message,
                        icon,
                        doNotTint
                    )
                redToast.root.tag = tag
                toastsArea.addView(redToast.root)

                redToast.root.slideInToastFromTop(
                    toastsArea,
                    true
                )
            }
        }
    }

    fun removePersistentRedToast(tag: String) {
        lifecycleScope.launch {
            withContext(Dispatchers.Main) {
                for (child in toastsArea.children) {
                    if (child.tag == tag) {
                        toastsArea.removeView(child)
                    }
                }
            }
        }
    }

    fun goToAndroidPermissionSettings() {
        Log.i("$TAG Going into Android settings for our app")
        try {
            val intent = Intent(
                Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                Uri.fromParts(
                    "package",
                    packageName, null
                )
            )
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(intent)
        } catch (anfe: ActivityNotFoundException) {
            Log.e("$TAG Failed to go to android settings: $anfe")
        }
    }

    protected fun enableWindowSecureMode(enable: Boolean) {
        val flags: Int = window.attributes.flags
        if ((enable && flags and WindowManager.LayoutParams.FLAG_SECURE != 0) ||
            (!enable && flags and WindowManager.LayoutParams.FLAG_SECURE == 0)
        ) {
            Log.d(
                "$TAG Secure flag is already ${if (enable) "enabled" else "disabled"}, skipping..."
            )
            return
        }

        if (enable) {
            Log.i("$TAG Secure flag added to window")
            window.addFlags(WindowManager.LayoutParams.FLAG_SECURE)
        } else {
            Log.w("$TAG Secure flag cleared from window")
            window.clearFlags(WindowManager.LayoutParams.FLAG_SECURE)
        }

        if (window.decorView.isAttachedToWindow) {
            Log.d("$TAG Redrawing window decorView to apply flag")
            try {
                windowManager.updateViewLayout(window.decorView, window.attributes)
            } catch (ise: IllegalStateException) {
                Log.e("$TAG Failed to update window's decorView layout: $ise")
            }
        }
    }
}
